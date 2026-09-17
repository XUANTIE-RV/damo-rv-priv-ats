/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.3 - G-stage faults forced to HS-mode and trap context
 *             (HZLRSC-09 ~ HZLRSC-14)
 *
 * Spec basis:
 *   norm:H_vm_gpatrans - G-stage faults raise guest-page-fault
 *     (cause 20/21/23) and, since hedeleg[21/23] are read-only-0
 *     (norm:hedeleg_acc), can never be delegated to VS-mode.
 *   norm:hstatus_gva_op / norm:hstatus_spv_op - hstatus.GVA/SPV are
 *     written when a trap is TAKEN INTO HS-mode: a guest LR/SC trap sets
 *     GVA=1 and SPV=1; an HLV/HSV explicit access sets GVA=1 but SPV=0.
 *     (These bits are NOT written for traps taken into M-mode, so the
 *     GVA/SPV cases route the fault into HS-mode via medeleg and read the
 *     values captured at trap entry BEFORE ts2_finish clears the record.)
 *   norm:htval_trapval - guest-page fault: htval = faulting GPA>>2 or 0;
 *     every other trap writes htval = 0.
 * =================================================================== */

/* HLV.W probe executed in HS-mode (PRIV_S) for the SPV=0/GVA=1 contrast. */
static uintptr_t hz_hs_hlv_w(uintptr_t addr)
{
    return (uintptr_t)(intptr_t)hlv_w(addr);
}

/* Clear hstatus.GVA/SPV so a stale value cannot fake a positive result. */
static inline void hz_clear_gva_spv(void)
{
    hstatus_write(hstatus_read() & ~(HSTATUS_GVA | HSTATUS_SPV));
}

/* ------------------------------------------------------------------
 * HZLRSC-09: LR G-stage fault -> load guest-page-fault (21); hedeleg[21]
 *            is read-only-0 (cannot be delegated to VS-mode).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_09_lr_gstage_fault);
bool test_hzlrsc_09_lr_gstage_fault(void)
{
    TEST_BEGIN("HZLRSC-09: LR G-stage fault -> load guest-page-fault (21)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("LR G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == load guest-page-fault (21)",
                   cause, (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);

    hedeleg_write(1UL << 21);
    TEST_ASSERT_BITS("hedeleg[21] read-only-0",
                     hedeleg_read(), (1UL << 21), (uintptr_t)0);
    hedeleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-10: SC G-stage fault -> store/AMO guest-page-fault (23);
 *            hedeleg[23] read-only-0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_10_sc_gstage_fault);
bool test_hzlrsc_10_sc_gstage_fault(void)
{
    TEST_BEGIN("HZLRSC-10: SC G-stage fault -> store guest-page-fault (23)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lrsc_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("SC G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    hedeleg_write(1UL << 23);
    TEST_ASSERT_BITS("hedeleg[23] read-only-0",
                     hedeleg_read(), (1UL << 23), (uintptr_t)0);
    hedeleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-11: guest LR/SC trap sets GVA=1 and SPV=1; stval = faulting VA.
 *            Routed into HS-mode so hardware writes hstatus.GVA/SPV.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_11_gva_spv);
bool test_hzlrsc_11_gva_spv(void)
{
    TEST_BEGIN("HZLRSC-11: guest LR/SC trap -> GVA=1 and SPV=1");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;

    /* LR (cause 21). */
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_LOAD_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired = trap_was_triggered();
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    uintptr_t tval = trap_get_tval();
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_LOAD_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] LR guest fault: gva=%d spv=%d tval=0x%lx\n",
           (int)gva, (int)spv, (unsigned long)tval);
    TEST_ASSERT("LR guest fault fired", fired);
    TEST_ASSERT_EQ("LR guest fault: hstatus.GVA=1",
                   (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("LR guest fault: hstatus.SPV=1 (trap from V=1)", spv);
    TEST_ASSERT_EQ("LR guest fault: stval == faulting GVA", tval, va);

    /* SC (cause 23). */
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lrsc_w, va);
    bool fired2 = trap_was_triggered();
    bool gva2 = trap_get_gva();
    bool spv2 = trap_get_spv();
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] SC guest fault: gva=%d spv=%d\n",
           (int)gva2, (int)spv2);
    TEST_ASSERT("SC guest fault fired", fired2);
    TEST_ASSERT_EQ("SC guest fault: hstatus.GVA=1",
                   (uintptr_t)gva2, (uintptr_t)1);
    TEST_ASSERT("SC guest fault: hstatus.SPV=1 (trap from V=1)", spv2);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-12: guest LR/SC fault -> htval = faulting GPA>>2 (or 0).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_12_htval_gpa);
bool test_hzlrsc_12_htval_gpa(void)
{
    TEST_BEGIN("HZLRSC-12: guest LR/SC fault -> htval == GPA>>2 or 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;   /* GPA == VA (identity) */

    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired = trap_was_triggered();
    uintptr_t htval = trap_get_htval();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("LR guest fault fired", fired);
    TEST_ASSERT("htval == 0 or GPA>>2",
                htval == 0 || htval == (va >> 2));
    printf("  [INFO] LR guest fault htval=0x%lx (GPA>>2=0x%lx)\n",
           (unsigned long)htval, (unsigned long)(va >> 2));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-13: VS-stage fault (not a guest-page fault) -> htval = 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_13_vs_stage_htval_zero);
bool test_hzlrsc_13_vs_stage_htval_zero(void)
{
    TEST_BEGIN("HZLRSC-13: VS-stage LR fault -> htval == 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_XONLY);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = trap_get_htval();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("VS-stage LR fault fired", fired);
    TEST_ASSERT_EQ("cause == load page-fault (13, VS-stage)",
                   cause, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT_EQ("htval == 0 (not a guest-page fault)",
                   htval, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-14: HLV.W explicit access -> SPV=0 but GVA=1 (contrast with
 *            HZLRSC-11's SPV=1/GVA=1 for guest LR/SC). The HLV runs in
 *            HS-mode (V=0); its guest-page fault is routed into HS-mode
 *            so hardware writes hstatus.GVA/SPV.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_14_hlv_spv0_gva1);
bool test_hzlrsc_14_hlv_spv0_gva1(void)
{
    TEST_BEGIN("HZLRSC-14: HLV.W explicit access -> SPV=0 but GVA=1");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);
    two_stage_enable(&ctx, 0);        /* activate vsatp/hgatp, stay in HS */
    hstatus_set_spvp(PRIV_S);         /* effective privilege = VS */
    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_LOAD_GUEST_PAGE_FAULT);

    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hz_hs_hlv_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    trap_expect_end();

    hz_unroute_from_hs(1UL << CAUSE_LOAD_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [INFO] HLV.W fault: cause=%lu gva=%d spv=%d\n",
           (unsigned long)cause, (int)gva, (int)spv);
    TEST_ASSERT("HLV.W guest-page fault fired", fired);
    TEST_ASSERT_EQ("cause == load guest-page-fault (21)",
                   cause, (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    TEST_ASSERT_EQ("HLV.W explicit access: GVA=1",
                   (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("HLV.W explicit access: SPV=0 (trap not from V=1)", !spv);

    HYP_TEST_END();
}
