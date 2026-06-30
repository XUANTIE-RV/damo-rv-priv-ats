/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/tests_2stage/test_group19_pmp.c
 *
 * Group 19 - PMP interaction with two-stage translation
 * (TS-PMP-01).
 *
 * NOTE: TS-PMP-02 removed (framework cannot isolate G-stage root PMP deny).
 *
 * Spec basis (norm:H_pmp): Machine-level PMP applies to supervisor
 * physical addresses regardless of virtualization mode. Once both
 * VS-stage and G-stage succeed, the resulting SPA is still subject
 * to PMP; failure surfaces as access-fault (cause=5/7), NOT as a
 * page-fault or guest-page-fault.
 *
 * Strategy:
 *   - PMP entry 0 is the highest-priority entry (lowest index wins).
 *     The framework reset configures entry 0 = NAPOT covering the
 *     whole address space with RWX. We override entry 0 with a deny
 *     rule that matches the target page only, then add entry 1 as a
 *     fall-through NAPOT(all-space, RWX). VS-mode access to the
 *     target page hits entry 0 first -> deny.
 * =================================================================== */

#ifndef SUITE_HGATP_MODE
#error "SUITE_HGATP_MODE must be defined before including this file"
#endif
#ifndef SUITE_VSATP_MODE
#error "SUITE_VSATP_MODE must be defined before including this file"
#endif

#include "pmp/pmp_cfg.h"

#define G19_GMODE   SUITE_HGATP_MODE
#define G19_VSMODE  SUITE_VSATP_MODE

/* Save/restore PMP entries 0 and 1 around a deny window. */
typedef struct {
    pmp_entry_t e0;
    pmp_entry_t e1;
} g19_pmp_save_t;

static void g19_pmp_deny_page(uintptr_t pa, g19_pmp_save_t *save) {
    pmp_get_entry(0, &save->e0);
    pmp_get_entry(1, &save->e1);

    /* Entry 0: deny target 4KB. */
    pmp_entry_t deny = PMP_ENTRY_NAPOT(pa & ~0xfffUL, 0x1000UL, 0);
    pmp_set_entry(0, &deny);
    /* Entry 1: allow all RWX (NAPOT spanning low 54 bits). */
    pmp_entry_t allow = PMP_ENTRY_NAPOT(0, (uintptr_t)1UL << 54, PMP_RWX);
    pmp_set_entry(1, &allow);
}

static void g19_pmp_restore(const g19_pmp_save_t *save) {
    pmp_set_entry(0, &save->e0);
    pmp_set_entry(1, &save->e1);
}

/* ===================================================================
 * TS-PMP-01: Two-stage translation succeeds, but the final SPA
 * is denied by PMP. Expect cause=5 (load-access-fault).
 * =================================================================== */
TEST_REGISTER(test_ts_pmp_01_spa_denied);
bool test_ts_pmp_01_spa_denied(void) {
    TEST_BEGIN("TS-PMP-01: PMP denies final SPA -> load-access-fault (5)");
    REQUIRE_VSATP_MODE(G19_VSMODE);
    REQUIRE_HGATP_MODE(G19_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_data_area;     /* identity == SPA */

    ts2_setup_full(&ctx, G19_VSMODE, G19_GMODE);

    g19_pmp_save_t save;
    g19_pmp_deny_page(va, &save);

    bool ok = ts2_run_check_fault(&ctx, test_vs_load_expect_fault, va,
                                  CAUSE_LOAD_ACCESS_FAULT);
    g19_pmp_restore(&save);
    TEST_ASSERT("cause = load-access-fault (5)", ok);
    HYP_TEST_END();
}

/* TS-PMP-02: removed — denying G-stage root PMP also breaks the M->VS
 * trampoline path, making it impossible to isolate the fault. */
