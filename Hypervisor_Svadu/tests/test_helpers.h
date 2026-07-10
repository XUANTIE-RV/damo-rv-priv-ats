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
 * Exception cause codes for two-stage translation faults
 * (from RISC-V Privileged Spec, hypervisor extension)
 *
 * VS-stage translation faults produce regular page-faults (cause 13/15).
 * G-stage translation faults produce guest-page-faults (cause 21/23).
 * Per norm:H_vm_gpatrans: "guest-page-fault exceptions are raised
 * instead of regular page-fault exceptions" — but only for G-stage.
 * =================================================================== */
#define CAUSE_LOAD_PAGE_FAULT           13   /* VS-stage translation fault */
#define CAUSE_STORE_PAGE_FAULT          15   /* VS-stage store/AMO fault */
#define CAUSE_INST_GUEST_PAGE_FAULT    20   /* G-stage instruction fault */
#define CAUSE_LOAD_GUEST_PAGE_FAULT    21   /* G-stage load fault */
#define CAUSE_STORE_GUEST_PAGE_FAULT   23   /* G-stage store/AMO fault */

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
 * Svadu detection via menvcfg.ADUE writability
 *
 * Per spec norm:Svadu_hw_update_a_d_bits: "If the Svadu extension is
 * implemented, the menvcfg.ADUE field is writable."
 * Per spec norm:menvcfg_adue_rdonly0: "If Svadu is not implemented,
 * ADUE is read-only zero."
 *
 * We must test menvcfg.ADUE (not henvcfg.ADUE) because
 * norm:menvcfg_adue_henvcfg_adue_rdonly0 states that
 * henvcfg.ADUE is read-only zero when menvcfg.ADUE is zero.
 * Testing henvcfg.ADUE writability would give a false negative
 * when menvcfg.ADUE happens to be zero.
 * =================================================================== */
static bool svadu_detected = false;
static bool svadu_detection_done = false;

static bool check_svadu_via_adue(void) {
    if (svadu_detection_done)
        return svadu_detected;

    /* Try to write menvcfg.ADUE=1 and read back */
    uintptr_t old_menvcfg = menvcfg_read();
    menvcfg_write(old_menvcfg | MENVCFG_ADUE);
    svadu_detected = ((menvcfg_read() & MENVCFG_ADUE) != 0);
    menvcfg_write(old_menvcfg);  /* restore */

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

/* Clear A/D bits in VS-stage PTE WITHOUT flushing TLB.
 * Used when the caller needs to preserve cached TLB entries
 * (e.g., cross-VMID ADUE synchronization tests where flushing
 * would destroy TLB state for other VMIDs). */
static void vs_pte_clear_ad_nofence(two_stage_ctx_t *ctx, uintptr_t va, int level) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, level);
    if (pte) {
        *pte &= ~(PTE_A | PTE_D);
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
