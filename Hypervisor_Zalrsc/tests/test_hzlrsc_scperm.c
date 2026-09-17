/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.5 - SC retire permission check (HZLRSC-22 ~ HZLRSC-26)
 *
 * Spec basis:
 *   norm:sc_retire_permission - "No sc.w shall retire unless it passes
 *     memory permission checks" (mandatory, SHALL). A failed SC still
 *     retires (it writes its nonzero failure code to rd), so it must
 *     still pass the store permission check.
 *   norm:sc_failed_as_store - "for the purposes of memory protection, a
 *     failed sc.w may be treated like a store".
 *   norm:store_page_fault_no_w - SC without write permission -> store
 *     page-fault (15); an unreadable page is also unwritable, so SC to
 *     an unreadable page reports 15, never load page-fault (13).
 *   norm:load_page_fault_no_r - LR needs only read permission, so LR to
 *     an R=1/W=0 page executes normally.
 *
 * Following the validated Zalrsc precedent (ZLRSC-26), each failed-SC
 * case exercises BOTH a no-reservation SC (plan literal) and an SC that
 * holds a reservation on a DIFFERENT writable page (so it must fail on
 * address mismatch yet still undergo the target's permission check).
 * =================================================================== */

/* Run a failed-SC probe against a VS-stage victim and report whether it
 * faulted with the given cause. Returns 1 on the expected fault. */
static int hzlrsc_failed_sc_vs(uintptr_t victim_va, uintptr_t resv_va,
                               uintptr_t vs_flags, uintptr_t (*probe)(uintptr_t),
                               uintptr_t exp_cause, const char *tag)
{
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, victim_va, vs_flags);
    hz_resv_other_va = resv_va;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, probe, victim_va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);
    printf("  [INFO] %s: fired=%d cause=%lu (expected %lu)\n",
           tag, (int)fired, (unsigned long)cause, (unsigned long)exp_cause);
    return fired && cause == exp_cause;
}

/* ------------------------------------------------------------------
 * HZLRSC-22: SC with a valid reservation to an R=1/W=0 page -> 15.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_22_sc_valid_res_no_write);
bool test_hzlrsc_22_sc_valid_res_no_write(void)
{
    TEST_BEGIN("HZLRSC-22: SC (valid reservation) to W=0 -> store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lrsc_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("SC write fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-23: FAILED SC (never writes memory) to W=0 page -> still 15.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_23_failed_sc_no_write);
bool test_hzlrsc_23_failed_sc_no_write(void)
{
    TEST_BEGIN("HZLRSC-23: failed SC to W=0 -> still store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    uintptr_t rw = (uintptr_t)test_data_area;

    int ok_none = hzlrsc_failed_sc_vs(va, rw, HZ_VS_R, hz_vs_sc_w,
                                      CAUSE_STORE_PAGE_FAULT,
                                      "failed SC (no reservation)");
    int ok_other = hzlrsc_failed_sc_vs(va, rw, HZ_VS_R,
                                       hz_vs_sc_w_holding_other,
                                       CAUSE_STORE_PAGE_FAULT,
                                       "failed SC (reservation elsewhere)");

    TEST_ASSERT("failed SC (no reservation) to W=0 raises store pf (15) "
                "[norm:sc_retire_permission]", ok_none);
    TEST_ASSERT("failed SC (reservation elsewhere) to W=0 raises store pf "
                "(15) [norm:sc_retire_permission]", ok_other);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-24: failed SC G-stage write check -> store guest-pf (23),
 *            GVA=1, SPV=1, htval = GPA>>2 (or 0).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_24_failed_sc_gstage);
bool test_hzlrsc_24_failed_sc_gstage(void)
{
    TEST_BEGIN("HZLRSC-24: failed SC G-stage W=0 -> store guest-pf (23)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    uintptr_t rw = (uintptr_t)test_data_area;

    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hz_resv_other_va = rw;
    /* Route the guest-page fault into HS-mode so hstatus.GVA/SPV are
     * written by hardware (norm:hstatus_gva_op / spv_op). */
    hz_route_to_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sc_w_holding_other, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    uintptr_t htval = fired ? trap_get_htval() : 0;
    trap_expect_end();

    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] failed-SC G-stage: fired=%d cause=%lu gva=%d spv=%d "
           "htval=0x%lx\n", (int)fired, (unsigned long)cause,
           (int)gva, (int)spv, (unsigned long)htval);

    TEST_ASSERT("failed-SC G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    TEST_ASSERT_EQ("failed-SC guest fault: GVA=1",
                   (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("failed-SC guest fault: SPV=1", spv);
    TEST_ASSERT("htval == 0 or GPA>>2",
                htval == 0 || htval == (va >> 2));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-25: LR needs only read permission -> R=1/W=0 page executes
 *            normally (contrast with the SC cases above).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_25_lr_read_only_ok);
bool test_hzlrsc_25_lr_read_only_ok(void)
{
    TEST_BEGIN("HZLRSC-25: LR to R=1/W=0 page executes normally");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu (LR needs only read)\n",
               (unsigned long)cause);
    TEST_ASSERT("LR to R=1/W=0 page took no trap (norm:load_page_fault_no_r)",
                !fired);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-26: SC to an unreadable page -> store class (15), never load
 *            class (13) (norm:store_page_fault_no_w NOTE).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_26_sc_unreadable_store_class);
bool test_hzlrsc_26_sc_unreadable_store_class(void)
{
    TEST_BEGIN("HZLRSC-26: SC to R=0 page -> store pf (15), not load (13)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    uintptr_t rw = (uintptr_t)test_data_area;

    /* Execute-only leaf: R=0 (hence W=0). SC must report store-class. */
    int ok_other = hzlrsc_failed_sc_vs(va, rw, HZ_VS_XONLY,
                                       hz_vs_sc_w_holding_other,
                                       CAUSE_STORE_PAGE_FAULT,
                                       "SC (reservation elsewhere) to R=0");

    TEST_ASSERT("SC to unreadable page raises store pf (15), not load (13) "
                "[norm:store_page_fault_no_w]", ok_other);

    HYP_TEST_END();
}
