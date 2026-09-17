/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4.3 - G-stage faults forced to HS-mode and trap context
 *             (HZABHA-10 ~ HZABHA-14)
 *
 * Spec basis:
 *   norm:H_vm_gpatrans / norm:hedeleg_acc - a byte/half AMO G-stage fault
 *     raises a store/AMO guest-page-fault (cause 23); hedeleg[23] RO-0.
 *   norm:hstatus_gva_op / norm:hstatus_spv_op - a guest byte/half AMO trap
 *     sets GVA=1 and SPV=1; an HSV.B explicit access sets GVA=1 but SPV=0.
 *   norm:htval_trapval - guest-page fault: htval = GPA>>2 or 0; other
 *     traps write htval = 0.
 * =================================================================== */

/* HSV.B probe executed in HS-mode (PRIV_S) for the SPV=0/GVA=1 contrast. */
static uintptr_t hzabha_hs_hsv_b(uintptr_t addr)
{
    hsv_b(addr, 0x5Au);
    return 0;
}

/* ------------------------------------------------------------------
 * HZABHA-10: byte/half AMO G-stage fault -> store/AMO guest-page-fault
 *            (23); hedeleg[23] read-only-0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_10_bh_amo_gstage_fault);
bool test_hzabha_10_bh_amo_gstage_fault(void)
{
    TEST_BEGIN("HZABHA-10: byte/half AMO G-stage fault -> guest-pf (23)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_b, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("byte AMO G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    hedeleg_write(1UL << 23);
    TEST_ASSERT_BITS("hedeleg[23] read-only-0",
                     hedeleg_read(), (1UL << 23), (uintptr_t)0);
    hedeleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-11: guest byte/half AMO trap -> GVA=1 and SPV=1 (VS + VU).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_11_gva_spv);
bool test_hzabha_11_gva_spv(void)
{
    TEST_BEGIN("HZABHA-11: guest byte/half AMO trap -> GVA=1 and SPV=1");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;

    /* VS source. */
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint64_t *)va = 0x0011223344556677ULL;
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_b, va);
    bool fired = trap_was_triggered();
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    uintptr_t tval = trap_get_tval();
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] VS-source byte AMO fault: gva=%d spv=%d tval=0x%lx\n",
           (int)gva, (int)spv, (unsigned long)tval);
    TEST_ASSERT("VS-source byte AMO guest fault fired", fired);
    TEST_ASSERT_EQ("VS source: GVA=1", (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("VS source: SPV=1 (trap from V=1)", spv);
    TEST_ASSERT_EQ("VS source: stval == faulting GVA", tval, va);

    /* VU source. */
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    ts2_g_override_4k(&ctx, va, HZ_G_RU);
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)two_stage_run_in_vu(&ctx, hz_vs_amo_add_b, va);
    bool fired2 = trap_was_triggered();
    bool gva2 = trap_get_gva();
    bool spv2 = trap_get_spv();
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] VU-source byte AMO fault: gva=%d spv=%d\n",
           (int)gva2, (int)spv2);
    TEST_ASSERT("VU-source byte AMO guest fault fired", fired2);
    TEST_ASSERT_EQ("VU source: GVA=1", (uintptr_t)gva2, (uintptr_t)1);
    TEST_ASSERT("VU source: SPV=1 (trap from V=1)", spv2);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-12: guest byte/half AMO G-stage fault -> htval = GPA>>2 (or 0).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_12_htval_gpa);
bool test_hzabha_12_htval_gpa(void)
{
    TEST_BEGIN("HZABHA-12: guest byte/half AMO fault -> htval == GPA>>2 or 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_b, va);
    bool fired = trap_was_triggered();
    uintptr_t htval = trap_get_htval();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("byte AMO guest fault fired", fired);
    TEST_ASSERT("htval == 0 or GPA>>2", htval == 0 || htval == (va >> 2));
    printf("  [INFO] byte AMO guest fault htval=0x%lx (GPA>>2=0x%lx)\n",
           (unsigned long)htval, (unsigned long)(va >> 2));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-13: VS-stage byte/half AMO fault (not guest-page fault) ->
 *            htval = 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_13_vs_stage_htval_zero);
bool test_hzabha_13_vs_stage_htval_zero(void)
{
    TEST_BEGIN("HZABHA-13: VS-stage byte/half AMO fault -> htval == 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_b, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = trap_get_htval();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("VS-stage byte AMO fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15, VS-stage)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT_EQ("htval == 0 (not a guest-page fault)",
                   htval, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-14: HSV.B explicit access -> SPV=0 but GVA=1 (contrast with the
 *            guest byte AMO's SPV=1/GVA=1). HSV runs in HS-mode (V=0).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_14_hsv_b_spv0_gva1);
bool test_hzabha_14_hsv_b_spv0_gva1(void)
{
    TEST_BEGIN("HZABHA-14: HSV.B explicit access -> SPV=0 but GVA=1");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);

    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hzabha_hs_hsv_b, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    trap_expect_end();

    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] HSV.B fault: cause=%lu gva=%d spv=%d\n",
           (unsigned long)cause, (int)gva, (int)spv);
    TEST_ASSERT("HSV.B guest-page fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    TEST_ASSERT_EQ("HSV.B explicit access: GVA=1",
                   (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("HSV.B explicit access: SPV=0 (trap not from V=1)", !spv);

    HYP_TEST_END();
}
