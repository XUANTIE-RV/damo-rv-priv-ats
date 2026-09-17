/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.3 - G-stage faults forced to HS-mode and trap context
 *             (HZLASR-12 ~ HZLASR-17)
 *
 * Spec basis:
 *   norm:H_vm_gpatrans / norm:hedeleg_acc - a G-stage fault raises a
 *     guest-page-fault that CANNOT be delegated (hedeleg[21]/[23] RO-0),
 *     so it is forced to HS-mode. load-acquire -> load guest-page-fault
 *     (21, record-type); store-release -> store/AMO guest-page-fault (23,
 *     forced).
 *   norm:hstatus_gva_op / norm:hstatus_spv_op - a guest Zalasr trap sets
 *     GVA=1 and SPV=1; an HLV.W/HSV.W explicit access sets GVA=1 but SPV=0
 *     (the sole NOTE exception), proving load-acquire/store-release are not
 *     virtual-machine load/store instructions.
 *   norm:htval_trapval - guest-page fault: htval = GPA>>2 or 0; a VS-stage
 *     (non-guest) fault writes htval = 0.
 * =================================================================== */

/* HLV.W / HSV.W probes executed in HS-mode (PRIV_S) for the SPV=0/GVA=1
 * contrast (HZLASR-17). */
static inline uintptr_t hzlasr_hs_hlv_w(uintptr_t addr)
{
    (void)hlv_w(addr);
    return 0;
}
static inline uintptr_t hzlasr_hs_hsv_w(uintptr_t addr)
{
    hsv_w(addr, 0x5Au);
    return 0;
}

/* ------------------------------------------------------------------
 * HZLASR-12: load-acquire G-stage fault -> forced HS-mode; the exact cause
 *            is RECORD-AND-COMPARE (functional expectation: load gpf 21).
 *            hedeleg[21] read-only-0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_12_load_acq_gstage_fault);
bool test_hzlasr_12_load_acq_gstage_fault(void)
{
    TEST_BEGIN("HZLASR-12: load-acquire G-stage fault -> HS-mode (record cause)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("load-acquire G-stage fault fired", fired);
    hzlasr_record_load_cause("load-acquire G-stage fault (expect gpf 21)",
                             cause);

    hedeleg_write(1UL << 21);
    TEST_ASSERT_BITS("hedeleg[21] read-only-0 (G-stage not delegable)",
                     hedeleg_read(), (1UL << 21), (uintptr_t)0);
    hedeleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-13: store-release G-stage fault -> store/AMO guest-page-fault (23);
 *            hedeleg[23] read-only-0. FORCED (store class unambiguous).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_13_store_rel_gstage_fault);
bool test_hzlasr_13_store_rel_gstage_fault(void)
{
    TEST_BEGIN("HZLASR-13: store-release G-stage fault -> guest-pf (23)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hz_st_val = 0x00009abcu;

    trap_expect_begin();
#if __riscv_xlen == 64
    (void)two_stage_run_in_vs(&ctx, hz_vs_sd_rl, va);
#else
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va);
#endif
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("store-release G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    hedeleg_write(1UL << 23);
    TEST_ASSERT_BITS("hedeleg[23] read-only-0 (G-stage not delegable)",
                     hedeleg_read(), (1UL << 23), (uintptr_t)0);
    hedeleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-14: guest load-acquire/store-release trap -> GVA=1 and SPV=1
 *            (VS + VU source), stval == faulting GVA.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_14_gva_spv);
bool test_hzlasr_14_gva_spv(void)
{
    TEST_BEGIN("HZLASR-14: guest Zalasr trap -> GVA=1 and SPV=1");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;

    /* VS source (store-release G-stage fault, cause 23). */
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hz_st_val = 0x00009abcu;
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va);
    bool fired = trap_was_triggered();
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    uintptr_t tval = trap_get_tval();
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] VS-source store-release fault: gva=%d spv=%d tval=0x%lx\n",
           (int)gva, (int)spv, (unsigned long)tval);
    TEST_ASSERT("VS-source store-release guest fault fired", fired);
    TEST_ASSERT_EQ("VS source: GVA=1", (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("VS source: SPV=1 (trap from V=1)", spv);
    TEST_ASSERT_EQ("VS source: stval == faulting GVA", tval, va);

    /* VU source. */
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    ts2_g_override_4k(&ctx, va, HZ_G_RU);
    hz_st_val = 0x00009abcu;
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)two_stage_run_in_vu(&ctx, hz_vs_sw_rl, va);
    bool fired2 = trap_was_triggered();
    bool gva2 = trap_get_gva();
    bool spv2 = trap_get_spv();
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] VU-source store-release fault: gva=%d spv=%d\n",
           (int)gva2, (int)spv2);
    TEST_ASSERT("VU-source store-release guest fault fired", fired2);
    TEST_ASSERT_EQ("VU source: GVA=1", (uintptr_t)gva2, (uintptr_t)1);
    TEST_ASSERT("VU source: SPV=1 (trap from V=1)", spv2);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-15: guest Zalasr G-stage fault -> htval == GPA>>2 or 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_15_htval_gpa);
bool test_hzlasr_15_htval_gpa(void)
{
    TEST_BEGIN("HZLASR-15: guest Zalasr G-stage fault -> htval == GPA>>2 or 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hz_st_val = 0x00009abcu;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va);
    bool fired = trap_was_triggered();
    uintptr_t htval = trap_get_htval();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("store-release guest fault fired", fired);
    TEST_ASSERT("htval == 0 or GPA>>2 (baseline norm:htval_trapval)",
                htval == 0 || htval == (va >> 2));
    printf("  [INFO] store-release guest fault htval=0x%lx (GPA>>2=0x%lx)\n",
           (unsigned long)htval, (unsigned long)(va >> 2));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-16: VS-stage store-release fault (not a guest-page fault) ->
 *            htval == 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_16_vs_stage_htval_zero);
bool test_hzlasr_16_vs_stage_htval_zero(void)
{
    TEST_BEGIN("HZLASR-16: VS-stage store-release fault -> htval == 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);
    hz_st_val = 0x00009abcu;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = trap_get_htval();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("VS-stage store-release fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15, VS-stage)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT_EQ("htval == 0 (not a guest-page fault)",
                   htval, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-17: HLV.W / HSV.W explicit access -> SPV=0 but GVA=1 (contrast
 *            with the guest Zalasr trap's SPV=1/GVA=1). Runs in HS-mode.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_17_hlv_hsv_spv0_gva1);
bool test_hzlasr_17_hlv_hsv_spv0_gva1(void)
{
    TEST_BEGIN("HZLASR-17: HLV.W/HSV.W explicit access -> SPV=0 but GVA=1");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    /* HSV.W (store-release contrast) -> store/AMO guest-page-fault (23). */
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hzlasr_hs_hsv_w, va);
    bool st_fired = trap_was_triggered();
    uintptr_t st_cause = st_fired ? trap_get_cause() : 0;
    bool st_gva = trap_get_gva();
    bool st_spv = trap_get_spv();
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] HSV.W fault: cause=%lu gva=%d spv=%d\n",
           (unsigned long)st_cause, (int)st_gva, (int)st_spv);
    TEST_ASSERT("HSV.W guest-page fault fired", st_fired);
    TEST_ASSERT_EQ("HSV.W cause == store/AMO guest-page-fault (23)",
                   st_cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    TEST_ASSERT_EQ("HSV.W explicit access: GVA=1",
                   (uintptr_t)st_gva, (uintptr_t)1);
    TEST_ASSERT("HSV.W explicit access: SPV=0 (trap not from V=1)", !st_spv);

    /* HLV.W (load-acquire contrast) -> load guest-page-fault (21). */
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_LOAD_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hzlasr_hs_hlv_w, va);
    bool ld_fired = trap_was_triggered();
    uintptr_t ld_cause = ld_fired ? trap_get_cause() : 0;
    bool ld_gva = trap_get_gva();
    bool ld_spv = trap_get_spv();
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_LOAD_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] HLV.W fault: cause=%lu gva=%d spv=%d\n",
           (unsigned long)ld_cause, (int)ld_gva, (int)ld_spv);
    TEST_ASSERT("HLV.W guest-page fault fired", ld_fired);
    TEST_ASSERT_EQ("HLV.W cause == load guest-page-fault (21)",
                   ld_cause, (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    TEST_ASSERT_EQ("HLV.W explicit access: GVA=1",
                   (uintptr_t)ld_gva, (uintptr_t)1);
    TEST_ASSERT("HLV.W explicit access: SPV=0 (trap not from V=1)", !ld_spv);

    HYP_TEST_END();
}
