/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 3.3 - G-stage faults forced to HS-mode and trap context
 *             (HZACAS-11 ~ HZACAS-16)
 *
 * Spec basis:
 *   norm:H_vm_gpatrans / norm:hedeleg_acc - an amocas G-stage fault raises
 *     a store/AMO guest-page-fault (cause 23); hedeleg[23] is read-only-0.
 *   norm:Zacas_amocas_w_permission - a FAILED CAS still undergoes the
 *     G-stage write permission check (HZACAS-12, analogue of HZLRSC-24).
 *   norm:hstatus_gva_op / norm:hstatus_spv_op - a guest amocas trap sets
 *     GVA=1 and SPV=1; an HSV explicit access sets GVA=1 but SPV=0.
 *   norm:htval_trapval - guest-page fault: htval = GPA>>2 or 0; other
 *     traps write htval = 0.
 * =================================================================== */

/* HSV.D probe executed in HS-mode (PRIV_S) for the SPV=0/GVA=1 contrast. */
static uintptr_t hzacas_hs_hsv_d(uintptr_t addr)
{
    hsv_d(addr, 0x123456789ABCDEF0ULL);
    return 0;
}

/* ------------------------------------------------------------------
 * HZACAS-11: SUCCESS-path amocas G-stage fault -> store/AMO guest-page
 *            fault (23); hedeleg[23] read-only-0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_11_cas_gstage_fault);
bool test_hzacas_11_cas_gstage_fault(void)
{
    TEST_BEGIN("HZACAS-11: amocas G-stage fault -> store guest-pf (23)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("amocas G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    hedeleg_write(1UL << 23);
    TEST_ASSERT_BITS("hedeleg[23] read-only-0",
                     hedeleg_read(), (1UL << 23), (uintptr_t)0);
    hedeleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-12: FAILED CAS G-stage write check -> store guest-page-fault (23).
 *            norm:Zacas_amocas_w_permission applies unconditionally.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_12_cas_failed_gstage);
bool test_hzacas_12_cas_failed_gstage(void)
{
    TEST_BEGIN("HZACAS-12: FAILED amocas G-stage W=0 -> store guest-pf (23)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00009999u;   /* mismatch -> guaranteed-failed CAS */
    hz_cas_swap = 0x00002000u;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    printf("  [INFO] FAILED amocas G-stage: fired=%d cause=%lu\n",
           (int)fired, (unsigned long)cause);
    TEST_ASSERT("FAILED amocas G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23) "
                   "[norm:Zacas_amocas_w_permission]",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-13: guest amocas trap -> GVA=1 and SPV=1 (VS + VU sources).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_13_gva_spv);
bool test_hzacas_13_gva_spv(void)
{
    TEST_BEGIN("HZACAS-13: guest amocas trap -> GVA=1 and SPV=1");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;

    /* VS source. */
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    uintptr_t tval = trap_get_tval();
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] VS-source amocas fault: gva=%d spv=%d tval=0x%lx\n",
           (int)gva, (int)spv, (unsigned long)tval);
    TEST_ASSERT("VS-source amocas guest fault fired", fired);
    TEST_ASSERT_EQ("VS source: GVA=1", (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("VS source: SPV=1 (trap from V=1)", spv);
    TEST_ASSERT_EQ("VS source: stval == faulting GVA", tval, va);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-14: guest amocas G-stage fault -> htval = GPA>>2 (or 0).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_14_htval_gpa);
bool test_hzacas_14_htval_gpa(void)
{
    TEST_BEGIN("HZACAS-14: guest amocas fault -> htval == GPA>>2 or 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t htval = trap_get_htval();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("amocas guest fault fired", fired);
    TEST_ASSERT("htval == 0 or GPA>>2", htval == 0 || htval == (va >> 2));
    printf("  [INFO] amocas guest fault htval=0x%lx (GPA>>2=0x%lx)\n",
           (unsigned long)htval, (unsigned long)(va >> 2));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-15: VS-stage amocas fault (not a guest-page fault) -> htval = 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_15_vs_stage_htval_zero);
bool test_hzacas_15_vs_stage_htval_zero(void)
{
    TEST_BEGIN("HZACAS-15: VS-stage amocas fault -> htval == 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = trap_get_htval();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("VS-stage amocas fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15, VS-stage)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT_EQ("htval == 0 (not a guest-page fault)",
                   htval, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-16: HSV.D explicit access -> SPV=0 but GVA=1 (contrast with the
 *            guest amocas's SPV=1/GVA=1). HSV runs in HS-mode (V=0).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_16_hsv_spv0_gva1);
bool test_hzacas_16_hsv_spv0_gva1(void)
{
    TEST_BEGIN("HZACAS-16: HSV.D explicit access -> SPV=0 but GVA=1");
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
    (void)run_in_priv(PRIV_S, hzacas_hs_hsv_d, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    trap_expect_end();

    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] HSV.D fault: cause=%lu gva=%d spv=%d\n",
           (unsigned long)cause, (int)gva, (int)spv);
    TEST_ASSERT("HSV.D guest-page fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    TEST_ASSERT_EQ("HSV.D explicit access: GVA=1",
                   (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("HSV.D explicit access: SPV=0 (trap not from V=1)", !spv);

    HYP_TEST_END();
}
