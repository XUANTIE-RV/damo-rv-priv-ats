/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Hypervisor x Svpbmt cross tests
 *
 * Provides H/Svpbmt extension detection, PBMT PTE modification helpers
 * for G-stage and VS-stage, and VS-mode test trampolines.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 */

#ifndef HYPERVISOR_SVPBMT_TEST_HELPERS_H
#define HYPERVISOR_SVPBMT_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_fence.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/test_vs_helpers.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/hyp_platform.h"

/* ===================================================================
 * Linker-provided test-region symbols (see kernel.ld).
 * =================================================================== */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

#define TEST_REGION_BASE   ((uintptr_t)__vm_test_region_start)

/* ===================================================================
 * H extension detection
 * =================================================================== */
static bool check_h_extension(void) {
    uint64_t misa = CSRR(misa);
    return (misa & (1UL << ('H' - 'A'))) != 0;
}

#define H_REQUIRED_OR_SKIP() do { \
    if (!check_h_extension()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * Svpbmt detection via menvcfg.PBMTE writability
 *
 * Per Svpbmt spec: when Svpbmt is implemented, menvcfg.PBMTE must be
 * writable. If PBMTE is read-only zero, Svpbmt is not implemented.
 * =================================================================== */
static bool svpbmt_detected = false;
static bool svpbmt_detection_done = false;

static bool check_svpbmt_extension(void) {
    if (svpbmt_detection_done)
        return svpbmt_detected;

    uintptr_t old = menvcfg_read();
    menvcfg_write(old | MENVCFG_PBMTE);
    uintptr_t new_val = menvcfg_read();
    menvcfg_write(old);  /* restore */

    svpbmt_detected = ((new_val & MENVCFG_PBMTE) != 0);
    svpbmt_detection_done = true;
    return svpbmt_detected;
}

#define SVPBMT_REQUIRED_OR_SKIP() do { \
    if (!check_svpbmt_extension()) { \
        TEST_SKIP("Svpbmt not available"); \
    } \
} while (0)

/* ===================================================================
 * PBMT PTE modification helpers
 * =================================================================== */

/*
 * g_pte_set_pbmt - Override PBMT field in G-stage leaf PTE
 * @ctx:  two_stage context
 * @gpa:  Guest physical address of the mapped page
 * @pbmt: PBMT value (PBMT_PMA, PBMT_NC, PBMT_IO)
 */
static void g_pte_set_pbmt(two_stage_ctx_t *ctx, uintptr_t gpa,
                            uintptr_t pbmt) {
    uintptr_t *pte = gpt_get_pte(&ctx->g_ctx, gpa, PT_LEVEL_4K);
    if (pte) {
        *pte = (*pte & ~PTE_PBMT_MASK) | (pbmt & PTE_PBMT_MASK);
        hfence_gvma_all();
    }
}

/*
 * vs_pte_set_pbmt - Override PBMT field in VS-stage leaf PTE
 * @ctx:  two_stage context
 * @va:   Virtual address of the mapped page
 * @pbmt: PBMT value (PBMT_PMA, PBMT_NC, PBMT_IO)
 */
static void vs_pte_set_pbmt(two_stage_ctx_t *ctx, uintptr_t va,
                             uintptr_t pbmt) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, PT_LEVEL_4K);
    if (pte) {
        *pte = (*pte & ~PTE_PBMT_MASK) | (pbmt & PTE_PBMT_MASK);
        hfence_vvma_all();
    }
}

/* ===================================================================
 * PBMT verification helpers
 * =================================================================== */

/* Read back PBMT from G-stage leaf PTE */
static uintptr_t g_pte_read_pbmt(two_stage_ctx_t *ctx, uintptr_t gpa) {
    uintptr_t *pte = gpt_get_pte(&ctx->g_ctx, gpa, PT_LEVEL_4K);
    if (pte)
        return PTE_PBMT(*pte);
    return 0;
}

/* Read back PBMT from VS-stage leaf PTE */
static uintptr_t vs_pte_read_pbmt(two_stage_ctx_t *ctx, uintptr_t va) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, PT_LEVEL_4K);
    if (pte)
        return PTE_PBMT(*pte);
    return 0;
}

#endif /* HYPERVISOR_SVPBMT_TEST_HELPERS_H */
