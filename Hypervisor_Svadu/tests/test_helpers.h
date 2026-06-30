/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Hypervisor × Svadu cross tests
 *
 * Provides H/Svadu extension detection, henvcfg/menvcfg ADUE access,
 * PTE inspection utilities, and VS-mode test trampolines.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 */

#ifndef HYPERVISOR_SVADU_TEST_HELPERS_H
#define HYPERVISOR_SVADU_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_ldst.h"
#include "hyp/hyp_fence.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/test_vs_helpers.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/hyp_platform.h"

/* ===================================================================
 * Linker-provided test-region symbols (see Hypervisor_Svadu/kernel.ld).
 * =================================================================== */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t test_exec_page[];
extern uint8_t test_exec_target[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

#define TEST_REGION_BASE   ((uintptr_t)__vm_test_region_start)

/* ===================================================================
 * Guest-page-fault cause codes (from RISC-V Privileged Spec)
 * =================================================================== */
#define CAUSE_INST_GUEST_PAGE_FAULT    20
#define CAUSE_LOAD_GUEST_PAGE_FAULT    21
#define CAUSE_STORE_GUEST_PAGE_FAULT   23

/* ===================================================================
 * H extension detection
 * =================================================================== */
static bool check_h_extension(void) {
    uint64_t misa = CSRR(misa);
    if (!(misa & (1UL << ('H' - 'A')))) {
        return false;
    }
    return true;
}

#define H_REQUIRED_OR_SKIP() do { \
    if (!check_h_extension()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * Svadu detection via henvcfg.ADUE writability
 *
 * Per spec norm:svadu_henvcfg_adue_writable: when Svadu is implemented,
 * henvcfg.ADUE must be writable. If ADUE is read-only zero, Svadu is
 * not implemented.
 * =================================================================== */
static bool svadu_detected = false;
static bool svadu_detection_done = false;

static bool check_svadu_via_adue(void) {
    if (svadu_detection_done)
        return svadu_detected;

    /* Try to write henvcfg.ADUE=1 and read back */
    uintptr_t old_henvcfg = henvcfg_read();
    henvcfg_set_adue(true);
    uintptr_t new_henvcfg = henvcfg_read();
    henvcfg_write(old_henvcfg);  /* restore */

    /* henvcfg.ADUE is bit 61, same as menvcfg.ADUE */
    svadu_detected = ((new_henvcfg & MENVCFG_ADUE) != 0);
    svadu_detection_done = true;
    return svadu_detected;
}

#define SVADU_REQUIRED_OR_SKIP() do { \
    if (!check_svadu_via_adue()) { \
        TEST_SKIP("Platform does not implement Svadu"); \
    } \
} while (0)

/* ===================================================================
 * henvcfg.ADUE access helpers
 * =================================================================== */
static inline int henvcfg_adue_read(void) {
    /* henvcfg.ADUE is bit 61, same as menvcfg.ADUE */
    return (henvcfg_read() & MENVCFG_ADUE) ? 1 : 0;
}

static inline void henvcfg_adue_set(int enable) {
    henvcfg_set_adue(enable);
}

/* ===================================================================
 * menvcfg.ADUE access helpers
 * =================================================================== */
static inline void menvcfg_adue_set(int enable) {
    uintptr_t val = menvcfg_read();
    if (enable)
        val |= MENVCFG_ADUE;
    else
        val &= ~MENVCFG_ADUE;
    menvcfg_write(val);
}

static inline int menvcfg_adue_read(void) {
    return (menvcfg_read() & MENVCFG_ADUE) ? 1 : 0;
}

/* ===================================================================
 * PTE inspection / modification helpers
 * =================================================================== */

/* Read VS-stage PTE at the given VA and level */
static uintptr_t vs_pte_read(two_stage_ctx_t *ctx, uintptr_t va, int level) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, level);
    return pte ? *pte : 0;
}

/* Read G-stage PTE at the given GPA and level */
static uintptr_t g_pte_read(two_stage_ctx_t *ctx, uintptr_t gpa, int level) {
    uintptr_t *pte = gpt_get_pte(&ctx->g_ctx, gpa, level);
    return pte ? *pte : 0;
}

/* Clear A/D bits in VS-stage PTE */
static void vs_pte_clear_ad(two_stage_ctx_t *ctx, uintptr_t va, int level) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, level);
    if (pte) {
        *pte &= ~(PTE_A | PTE_D);
        hfence_vvma_all();
    }
}

/* Clear A/D bits in G-stage PTE */
static void g_pte_clear_ad(two_stage_ctx_t *ctx, uintptr_t gpa, int level) {
    uintptr_t *pte = gpt_get_pte(&ctx->g_ctx, gpa, level);
    if (pte) {
        *pte &= ~(PTE_A | PTE_D);
        hfence_gvma_all();
    }
}

/* Set A bit in G-stage PTE, clear D bit */
static void g_pte_set_a_clear_d(two_stage_ctx_t *ctx, uintptr_t gpa, int level) {
    uintptr_t *pte = gpt_get_pte(&ctx->g_ctx, gpa, level);
    if (pte) {
        *pte = (*pte | PTE_A) & ~PTE_D;
        hfence_gvma_all();
    }
}

/* ===================================================================
 * VS-mode test trampolines
 * =================================================================== */

/* VS-mode load: returns 0 on success, cause on trap */
static uintptr_t vs_load(uintptr_t arg) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode store: returns 0 on success, cause on trap */
static uintptr_t vs_store(uintptr_t arg) {
    trap_expect_begin();
    *(volatile uintptr_t *)arg = 0xDEADBEEF;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

#endif /* HYPERVISOR_SVADU_TEST_HELPERS_H */
