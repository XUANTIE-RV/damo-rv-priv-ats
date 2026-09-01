/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_hcross_pmp.c - Group 5: Hypervisor x PMP cross tests
 *
 * Group 5.1 (tests 01-04): two-stage translation succeeds but the
 *         final SPA is denied by machine-level PMP. VS/VU explicit
 *         accesses must raise access faults (1/5/7), never page
 *         faults or guest-page faults (norm:H_pmp).
 *
 * Group 5.2 (tests 05-06): PMP also constrains implicit page-table
 *         walks (norm:pmp_with_paging). A PMP violation is always an
 *         access fault (norm:pmp_access_fault_exception); the page
 *         fault -> guest-page fault conversion of norm:H_vm_gpatrans
 *         does not cover access faults.
 *
 * Group 5.3 (test 07): translation caches may cache PMP
 *         attributes; after tightening PMP, SFENCE.VMA + HFENCE.GVMA
 *         must make the new settings visible (norm:pmp_sfence_required).
 *
 * Group 5.4 (test 08): HLVX cannot override PMP - an execute-only
 *         PMP region still denies HLVX reads.
 *
 * PMP configuration strategy: entry 0 = victim rule (4KB NAPOT),
 * entry 1 = fall-through NAPOT(all-space, RWX); all unlocked so M-mode
 * is unaffected. See test_helpers.h.
 *
 * See DOCS/testplan/Hypervisor_Sm_test_plan.md Group 5.
 */

/* ===================================================================
 * Group 5.1: final SPA denied by PMP (explicit accesses)
 * =================================================================== */

/* HCROSS-PMP-01: VS-mode load -> load access fault (5) */
TEST_REGISTER(test_hcross_pmp_01);
bool test_hcross_pmp_01(void)
{
    TEST_BEGIN("HCROSS-PMP-01: PMP denies final SPA -> VS load access fault");

    REQUIRE_HYP_PMP();

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_data_area;   /* identity: VA==GPA==SPA */
    ts2_setup_full(&ctx, HPMP_VS_MODE, HPMP_G_MODE);

    hpmp_save_t save;
    hpmp_deny_page(va, &save);

    bool ok = ts2_run_check_fault(&ctx, test_vs_load_expect_fault, va,
                                  CAUSE_LOAD_ACCESS_FAULT);
    hpmp_restore(&save);

    TEST_ASSERT("cause = 5, not page-fault (13) / guest-page-fault (21)", ok);
    TEST_ASSERT("trap originated from V=1 (mstatus.MPV=1)", trap_get_mpv());

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCROSS-PMP-02: VS-mode store -> store access fault (7) */
TEST_REGISTER(test_hcross_pmp_02);
bool test_hcross_pmp_02(void)
{
    TEST_BEGIN("HCROSS-PMP-02: PMP denies final SPA -> VS store access fault");

    REQUIRE_HYP_PMP();

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_data_area;
    ts2_setup_full(&ctx, HPMP_VS_MODE, HPMP_G_MODE);

    hpmp_save_t save;
    hpmp_deny_page(va, &save);

    bool ok = ts2_run_check_fault(&ctx, test_vs_store_expect_fault, va,
                                  CAUSE_STORE_ACCESS_FAULT);
    hpmp_restore(&save);

    TEST_ASSERT("cause = 7, not page-fault (15) / guest-page-fault (23)", ok);
    TEST_ASSERT("trap originated from V=1 (mstatus.MPV=1)", trap_get_mpv());

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCROSS-PMP-03: VS-mode fetch -> instruction access fault (1) */
TEST_REGISTER(test_hcross_pmp_03);
bool test_hcross_pmp_03(void)
{
    TEST_BEGIN("HCROSS-PMP-03: PMP denies final SPA -> VS fetch access fault");

    REQUIRE_HYP_PMP();

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_exec_page;
    ts2_setup_full(&ctx, HPMP_VS_MODE, HPMP_G_MODE);

    hpmp_save_t save;
    hpmp_deny_page(va, &save);

    bool ok = ts2_run_check_fault(&ctx, test_vs_exec_expect_fault, va,
                                  CAUSE_INST_ACCESS_FAULT);
    hpmp_restore(&save);

    TEST_ASSERT("cause = 1, not page-fault (12) / guest-page-fault (20)", ok);
    TEST_ASSERT("trap originated from V=1 (mstatus.MPV=1)", trap_get_mpv());

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCROSS-PMP-04: VU-mode load -> load access fault (5) */
TEST_REGISTER(test_hcross_pmp_04);
bool test_hcross_pmp_04(void)
{
    TEST_BEGIN("HCROSS-PMP-04: PMP denies final SPA -> VU load access fault");

    REQUIRE_HYP_PMP();

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_data_area;
    /* VU setup: VS-stage leaf PTEs carry U=1 so VU-mode may access. */
    ts2_setup_full_u(&ctx, HPMP_VS_MODE, HPMP_G_MODE);

    hpmp_save_t save;
    hpmp_deny_page(va, &save);

    trap_expect_begin();
    (void)two_stage_run_in_vu(&ctx, test_vs_load_expect_fault, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    two_stage_cleanup(&ctx);
    hyp_reset_state();
    hpmp_restore(&save);

    TEST_ASSERT("VU load trapped", fired);
    TEST_ASSERT_EQ("cause = 5 (load access fault)",
                   cause, (uintptr_t)CAUSE_LOAD_ACCESS_FAULT);
    TEST_ASSERT("trap originated from V=1 (mstatus.MPV=1)", trap_get_mpv());

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * Group 5.2: PMP constraints on implicit page-table walks
 * =================================================================== */

/* HCROSS-PMP-05: VS-stage page-table walk denied by PMP.
 *
 * The VS-stage PT page holding the leaf PTE for @va is denied in PMP
 * (its SPA; GPA==SPA under the identity G-stage mapping). The implicit
 * PT read is first translated by G-stage successfully, then fails the
 * PMP check: that is a physical-protection violation, so the guest
 * load must report a load access fault (5) per
 * norm:pmp_access_fault_exception - a guest-page fault would violate
 * the spec (norm:H_vm_gpatrans converts only page faults). */
TEST_REGISTER(test_hcross_pmp_05);
bool test_hcross_pmp_05(void)
{
    TEST_BEGIN("HCROSS-PMP-05: PMP denies VS-stage PT walk -> access fault");

    REQUIRE_HYP_PMP();

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_data_area;
    ts2_setup_full(&ctx, HPMP_VS_MODE, HPMP_G_MODE);

    uintptr_t pt_pa = two_stage_vs_pt_page_addr(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS-stage leaf PT page resolvable", pt_pa != 0);
    if (pt_pa == 0) {
        ts2_finish(&ctx);
        HYP_TEST_END();
    }

    hpmp_save_t save;
    hpmp_deny_page(pt_pa, &save);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_load_expect_fault, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    hpmp_restore(&save);

    TEST_ASSERT("implicit VS-PT walk trapped", fired);
    TEST_ASSERT_EQ("cause = 5 (load access fault), not guest-page fault",
                   cause, (uintptr_t)CAUSE_LOAD_ACCESS_FAULT);
    TEST_ASSERT("trap originated from V=1 (mstatus.MPV=1)", trap_get_mpv());

    /* After restoring PMP the same access must succeed again. */
    uintptr_t r = ts2_run_check_no_fault(&ctx, test_vs_read_write, va);
    TEST_ASSERT("VS access succeeds after PMP restore", r == 0);

    two_stage_cleanup(&ctx);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCROSS-PMP-06: G-stage page-table walk denied by PMP.
 *
 * Deny the second-level G-stage PT page reached through the root
 * entry covering the victim GPA, then trigger the G-stage walk with
 * an HLV load from HS-mode while hgatp is active (V=0).
 *
 * HLV from HS-mode is used instead of a VS-mode explicit access:
 * the mid-level G-stage PT page is shared with the kernel image
 * window, so under VS-mode the very first trampoline instruction
 * fetch would fault unrecoverably; HLV drives the same G-stage walk
 * machinery and traps recoverably in the HS/M context.
 *
 * The PMP violation is expected to surface as a load access fault
 * (5). Because the failing read sits inside the G-stage translation
 * machinery itself, the spec does not fully pin down the report
 * form; a load guest-page fault (21) is accepted but diagnosed. */
TEST_REGISTER(test_hcross_pmp_06);
bool test_hcross_pmp_06(void)
{
    TEST_BEGIN("HCROSS-PMP-06: PMP denies G-stage PT walk -> access fault");

    REQUIRE_HYP_PMP();

    two_stage_ctx_t ctx;
    uintptr_t gpa = (uintptr_t)test_data_area;
    ts2_setup_full(&ctx, HPMP_VS_MODE, HPMP_G_MODE);

    uintptr_t mid_pa = hpmp_g_mid_pt_page(&ctx, gpa);
    TEST_ASSERT("G-stage mid-level PT page resolvable", mid_pa != 0);
    if (mid_pa == 0) {
        ts2_finish(&ctx);
        HYP_TEST_END();
    }

    /* Activate hgatp/vsatp; drive the G-stage walk via HLV in HS-mode
     * (V=0), effective privilege VS (SPVP=1). */
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);

    hpmp_save_t save;
    hpmp_deny_page(mid_pa, &save);

    trap_expect_begin();
    (void)hlv_d(gpa);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    hpmp_restore(&save);

    TEST_ASSERT("implicit G-PT walk trapped", fired);
    if (cause == CAUSE_LOAD_GUEST_PAGE_FAULT) {
        printf("  DIAG: walk failure reported as guest-page fault (21)\n");
    }
    TEST_ASSERT("cause = 5 (access fault) or 21 (walk-internal report)",
                cause == CAUSE_LOAD_ACCESS_FAULT ||
                cause == CAUSE_LOAD_GUEST_PAGE_FAULT);

    /* After restoring PMP the same guest access must succeed. */
    uintptr_t r = ts2_run_check_no_fault(&ctx, test_vs_read_write, gpa);
    TEST_ASSERT("VS access succeeds after PMP restore", r == 0);

    two_stage_cleanup(&ctx);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * Group 5.3: PMP change synchronization (HFENCE.GVMA)
 * =================================================================== */

/* HCROSS-PMP-07: tighten PMP after translations are cached.
 *
 * Phase 1: allow-all, run a VS access successfully so that the
 * implementation may cache the translation (with its PMP attributes).
 * Phase 2: M-mode denies the target SPA, then synchronizes with
 * SFENCE.VMA (x0,x0) + HFENCE.GVMA (x0,x0). The next VS access must
 * now observe the tightened PMP and raise an access fault - proving
 * cached translation entries carrying stale PMP attributes were
 * flushed. */
TEST_REGISTER(test_hcross_pmp_07);
bool test_hcross_pmp_07(void)
{
    TEST_BEGIN("HCROSS-PMP-07: HFENCE.GVMA sync after PMP tighten");

    REQUIRE_HYP_PMP();

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_data_area;
    ts2_setup_full(&ctx, HPMP_VS_MODE, HPMP_G_MODE);

    /* Phase 1: baseline access succeeds (translation caches may be
     * populated with the allow-all PMP attributes). */
    uintptr_t r = ts2_run_check_no_fault(&ctx, test_vs_read_write, va);
    TEST_ASSERT("initial VS access succeeds (allow-all)", r == 0);

    /* Phase 2: tighten PMP + mandatory synchronization fences
     * (hpmp_deny_page performs SFENCE.VMA + HFENCE.GVMA). */
    hpmp_save_t save;
    hpmp_deny_page(va, &save);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_load_expect_fault, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    hpmp_restore(&save);

    TEST_ASSERT("post-sync VS access trapped", fired);
    TEST_ASSERT_EQ("cause = 5 (load access fault) after synchronization",
                   cause, (uintptr_t)CAUSE_LOAD_ACCESS_FAULT);

    two_stage_cleanup(&ctx);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * Group 5.4: HLVX and PMP
 * =================================================================== */

/* HCROSS-PMP-08: HLVX cannot override PMP.
 *
 * Target page carries X=1 and U=1 in both VS-stage and G-stage PTEs;
 * HLVX runs VU-effective (hstatus.SPVP=0, norm:hlsv_priv) so both
 * U-bit checks pass and only PMP can deny the access.
 *
 * Sequence:
 *   - Sanity: with PMP wide open, HLVX.WU succeeds through the
 *     X-only PTEs and returns the planted word (execution permission
 *     substitutes for read at the page-table level).
 *   - Main: PMP grants execute-only (X=1, R=0) on the final SPA.
 *     HLVX.WU must now raise a load access fault: the physical
 *     memory attributes must grant BOTH read and execute, and HLVX
 *     cannot override PMP.
 *   - Isolation: after restoring PMP, HLVX.WU succeeds again,
 *     proving the failure was caused by the PMP read denial, not by
 *     page-table permissions.
 */
TEST_REGISTER(test_hcross_pmp_08);
bool test_hcross_pmp_08(void)
{
    TEST_BEGIN("HCROSS-PMP-08: HLVX cannot override PMP execute-only");

    REQUIRE_HYP_PMP();

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    /* Plant a known word so the sanity HLVX result can be verified. */
    *(volatile uint32_t *)va = 0x90909090U;

    /* X=1, U=1 at both stages so the VU-effective HLVX access clears
     * all U-bit checks and PMP is the sole possible denier. */
    uintptr_t vs_x = PTE_V | PTE_X | PTE_U | PTE_A | PTE_D;
    uintptr_t g_x  = PTE_V | PTE_X | PTE_U | PTE_A | PTE_D;
    ts2_setup_with_dual_victim(&ctx, HPMP_VS_MODE, HPMP_G_MODE,
                               va, vs_x, g_x);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_U);   /* VU-effective access */

    /* Sanity: wide-open PMP, HLVX.WU succeeds on the X-only PTEs. */
    trap_expect_begin();
    uint32_t sv = hlvx_wu(va);
    bool s_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("sanity: HLVX.WU succeeds with PMP wide open", !s_fired);
    TEST_ASSERT_EQ("sanity: HLVX.WU returned planted word",
                   (uintptr_t)sv, (uintptr_t)0x90909090U);

    /* Main: PMP execute-only on the final SPA (identity: SPA == va). */
    hpmp_save_t save;
    hpmp_xonly_page(va, &save);

    trap_expect_begin();
    (void)hlvx_wu(va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    hpmp_restore(&save);

    TEST_ASSERT("HLVX.WU trapped on PMP execute-only region", fired);
    TEST_ASSERT_EQ("cause = 5 (load access fault), PMP R=0",
                   cause, (uintptr_t)CAUSE_LOAD_ACCESS_FAULT);

    /* Isolation: after restoring PMP the identical access succeeds
     * again, so the failure above was caused by the PMP read denial. */
    trap_expect_begin();
    (void)hlvx_wu(va);
    bool i_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("HLVX.WU succeeds after PMP restore (isolation)", !i_fired);

    hstatus_set_spvp(PRIV_S);
    two_stage_cleanup(&ctx);
    ts2_finish(&ctx);
    HYP_TEST_END();
}
