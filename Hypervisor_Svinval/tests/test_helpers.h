/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Hypervisor × Svinval cross tests
 *
 * Provides H/Svinval extension detection, HINVAL instruction wrappers,
 * VS/VU-mode trampolines, and PTE inspection utilities.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 */

#ifndef HYPERVISOR_SVINVAL_TEST_HELPERS_H
#define HYPERVISOR_SVINVAL_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_fence.h"
#include "hyp/hyp_ldst.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/test_vs_helpers.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/hyp_platform.h"
#include "hyp_svinval_insn.h"

/* ===================================================================
 * Linker-provided test-region symbols (see Hypervisor_Svinval/kernel.ld).
 * =================================================================== */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t test_exec_page[];
extern uint8_t test_exec_target[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

#define TEST_REGION_BASE   ((uintptr_t)__vm_test_region_start)

/* ===================================================================
 * Exception cause codes (from RISC-V Privileged Spec)
 *
 * Note: When hedeleg=0, VS-mode exceptions are routed to M-mode,
 * which uses S-mode cause codes (13/15) rather than VS-mode codes (5/7).
 * This test suite sets hedeleg=0 in hyp_reset.c, so we expect:
 *   - VS-mode load page fault = 13 (S-mode encoding)
 *   - VS-mode store/AMO page fault = 15 (S-mode encoding)
 * =================================================================== */
#define CAUSE_VIRTUAL_INSTRUCTION      22
#define CAUSE_VS_STORE_PAGE_FAULT      15  /* S-mode encoding when hedeleg=0 */
#define CAUSE_VS_LOAD_PAGE_FAULT       13  /* S-mode encoding when hedeleg=0 */
#define CAUSE_INST_GUEST_PAGE_FAULT    20
#define CAUSE_LOAD_GUEST_PAGE_FAULT    21
#define CAUSE_STORE_GUEST_PAGE_FAULT   23


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

/* Modify VS-stage PTE at the given VA and level */
static void vs_pte_modify(two_stage_ctx_t *ctx, uintptr_t va, int level, uintptr_t new_flags) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, level);
    if (pte) {
        /* Preserve PPN, update flags */
        *pte = (*pte & ~(PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)) | new_flags;
    }
}

/* Modify G-stage PTE at the given GPA and level */
static void g_pte_modify(two_stage_ctx_t *ctx, uintptr_t gpa, int level, uintptr_t new_flags) {
    uintptr_t *pte = gpt_get_pte(&ctx->g_ctx, gpa, level);
    if (pte) {
        /* Preserve PPN, update flags */
        *pte = (*pte & ~(PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)) | new_flags;
    }
}

/* ===================================================================
 * VS-mode test trampolines for HINVAL instructions
 * =================================================================== */

/* VS-mode: execute HINVAL.VVMA(arg, 0) */
static uintptr_t vs_exec_hinval_vvma(uintptr_t arg) {
    trap_expect_begin();
    HINVAL_VVMA(arg, 0);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: execute HINVAL.GVMA(arg, 0) */
static uintptr_t vs_exec_hinval_gvma(uintptr_t arg) {
    trap_expect_begin();
    HINVAL_GVMA(arg, 0);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: execute SFENCE.W.INVAL */
static uintptr_t vs_exec_sfence_w_inval(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    SFENCE_W_INVAL();
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: execute SFENCE.INVAL.IR */
static uintptr_t vs_exec_sfence_inval_ir(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    SFENCE_INVAL_IR();
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VU-mode test trampolines for HINVAL instructions
 * =================================================================== */

/* VU-mode: execute HINVAL.VVMA(arg, 0) */
static uintptr_t vu_exec_hinval_vvma(uintptr_t arg) {
    trap_expect_begin();
    HINVAL_VVMA(arg, 0);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: execute HINVAL.GVMA(arg, 0) */
static uintptr_t vu_exec_hinval_gvma(uintptr_t arg) {
    trap_expect_begin();
    HINVAL_GVMA(arg, 0);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: execute SFENCE.W.INVAL */
static uintptr_t vu_exec_sfence_w_inval(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    SFENCE_W_INVAL();
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: execute SFENCE.INVAL.IR */
static uintptr_t vu_exec_sfence_inval_ir(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    SFENCE_INVAL_IR();
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: execute SINVAL.VMA(arg, 0) */
static uintptr_t vu_exec_sinval_vma(uintptr_t arg) {
    trap_expect_begin();
    SINVAL_VMA(arg, 0);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode memory access trampolines
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

#endif /* HYPERVISOR_SVINVAL_TEST_HELPERS_H */
