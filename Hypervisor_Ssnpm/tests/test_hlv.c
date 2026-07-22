/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hlv.c - Group 4: H x Ssnpm - HLV/HSV pointer-mask selection
 *
 * HZPM-HLV-01 ~ HZPM-HLV-10
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 4.
 *
 * Spec references (hypervisor.adoc, sec:hstatus HUPMM paragraph):
 *   - HLV/HSV explicit accesses always act as though V=1 with the
 *     nominal privilege mode = hstatus.SPVP (norm:mstatus_mprv_hlsv).
 *   - Access as though in VS-mode (SPVP=1): PM controlled by
 *     henvcfg.PMM (any execution mode).
 *   - Access as though in VU-mode (SPVP=0), executed in HS/M-mode:
 *     PM controlled by senvcfg.PMM.
 *   - Access as though in VU-mode (SPVP=0), executed in U-mode
 *     (hstatus.HU=1): PM controlled by hstatus.HUPMM.
 */

/* Common two-stage setup: VS-stage Sv39 + G-stage Sv39x4, then
 * activate both stages without entering VS-mode (for HLV/HSV). */
static void hlv_setup(two_stage_ctx_t *ctx) {
    ts2_setup_full(ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    two_stage_enable(ctx, /*vmid=*/0);
}

/* Same, but with U=1 VS-stage mappings: required for HLV/HSV
 * accesses performed as though in VU-mode (SPVP=0). */
static void hlv_setup_u(two_stage_ctx_t *ctx) {
    ts2_setup_full_u(ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    two_stage_enable(ctx, /*vmid=*/0);
}

/* ----- HZPM-HLV-01: HS-mode, SPVP=1 -> henvcfg.PMM ----- */
TEST_REGISTER(test_hzpm_hlv_01);
bool test_hzpm_hlv_01(void) {
    TEST_BEGIN("HZPM-HLV-01: HLV in HS-mode, SPVP=1 uses henvcfg.PMM");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    hlv_setup(&ctx);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    hstatus_set_spvp(PRIV_S);   /* access as though in VS-mode */
    goto_priv(PRIV_S);          /* execute in HS-mode */
    uint64_t val = hlv_d(tagged);
    goto_priv(PRIV_M);

    TEST_ASSERT_EQ("HLV.D via tagged GVA correct", val, HZPM_MAGIC1);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HLV-02: HS-mode, SPVP=1, henvcfg.PMM=0 -> fault ----- */
TEST_REGISTER(test_hzpm_hlv_02);
bool test_hzpm_hlv_02(void) {
    TEST_BEGIN("HZPM-HLV-02: HLV in HS-mode, SPVP=1, PM off -> fault");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    pm_set_vsmode(PMM_DISABLED);

    two_stage_ctx_t ctx;
    hlv_setup(&ctx);

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    /* With VS PM disabled the tagged GVA is not transformed; it is
     * an invalid Sv39 VA -> VS-stage error -> load page-fault (13). */
    hstatus_set_spvp(PRIV_S);
    goto_priv(PRIV_S);
    trap_expect_begin();
    (void)hlv_d(tagged);
    TEST_ASSERT("HLV trapped", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=13 (load page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    }
    trap_expect_end();
    goto_priv(PRIV_M);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HLV-03: HS-mode, SPVP=0 -> senvcfg.PMM ----- */
TEST_REGISTER(test_hzpm_hlv_03);
bool test_hzpm_hlv_03(void) {
    TEST_BEGIN("HZPM-HLV-03: HLV in HS-mode, SPVP=0 uses senvcfg.PMM");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_u_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for U/VU-mode");

    two_stage_ctx_t ctx;
    hlv_setup_u(&ctx);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC2;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    hstatus_set_spvp(PRIV_U);   /* access as though in VU-mode */
    goto_priv(PRIV_S);
    uint64_t val = hlv_d(tagged);
    goto_priv(PRIV_M);

    TEST_ASSERT_EQ("HLV.D via tagged GVA correct", val, HZPM_MAGIC2);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HLV-04: HS-mode, SPVP=0 -> HUPMM ineffective ----- */
TEST_REGISTER(test_hzpm_hlv_04);
bool test_hzpm_hlv_04(void) {
    TEST_BEGIN("HZPM-HLV-04: HUPMM ineffective for HLV in HS-mode");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* sec:hstatus HUPMM paragraph: in HS- and M-modes, PM for
     * HLV/HSV performed as though in VU-mode is controlled by
     * senvcfg.PMM -- NOT by hstatus.HUPMM. With senvcfg.PMM=0 and
     * HUPMM=PMLEN7, the tagged access must fault. */
    pm_set_umode(PMM_DISABLED);
    if (!hzpm_try_set_hupmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for HUPMM");

    two_stage_ctx_t ctx;
    hlv_setup_u(&ctx);

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    hstatus_set_spvp(PRIV_U);
    goto_priv(PRIV_S);
    trap_expect_begin();
    (void)hlv_d(tagged);
    TEST_ASSERT("HLV trapped (HUPMM must not apply in HS-mode)",
                trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=13 (load page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    }
    trap_expect_end();
    goto_priv(PRIV_M);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HLV-05: M-mode, SPVP=0 -> senvcfg.PMM ----- */
TEST_REGISTER(test_hzpm_hlv_05);
bool test_hzpm_hlv_05(void) {
    TEST_BEGIN("HZPM-HLV-05: HLV in M-mode, SPVP=0 uses senvcfg.PMM");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_u_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for U/VU-mode");

    /* M-mode's own PM (mseccfg.PMM) must not interfere */
    if (detect_smmpm())
        pm_set_mmode(PMM_DISABLED);

    two_stage_ctx_t ctx;
    hlv_setup_u(&ctx);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC3;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    hstatus_set_spvp(PRIV_U);
    uint64_t val = hlv_d(tagged);   /* already in M-mode */

    TEST_ASSERT_EQ("HLV.D via tagged GVA correct", val, HZPM_MAGIC3);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HLV-06: M-mode, SPVP=1 -> henvcfg.PMM ----- */
TEST_REGISTER(test_hzpm_hlv_06);
bool test_hzpm_hlv_06(void) {
    TEST_BEGIN("HZPM-HLV-06: HLV in M-mode, SPVP=1 uses henvcfg.PMM");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    hlv_setup(&ctx);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    hstatus_set_spvp(PRIV_S);
    uint64_t val = hlv_d(tagged);

    TEST_ASSERT_EQ("HLV.D via tagged GVA correct", val, HZPM_MAGIC1);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HLV-07: U-mode, SPVP=0 -> hstatus.HUPMM ----- */
TEST_REGISTER(test_hzpm_hlv_07);
bool test_hzpm_hlv_07(void) {
    TEST_BEGIN("HZPM-HLV-07: HLV in U-mode, SPVP=0 uses HUPMM");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_hupmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for HUPMM");

    two_stage_ctx_t ctx;
    hlv_setup_u(&ctx);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC2;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    hstatus_set_spvp(PRIV_U);
    hstatus_set_hu(true);           /* allow HLV/HSV in U-mode */
    goto_priv(PRIV_U);
    uint64_t val = hlv_d(tagged);
    goto_priv(PRIV_M);

    TEST_ASSERT_EQ("HLV.D via tagged GVA correct", val, HZPM_MAGIC2);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HLV-08: U-mode, SPVP=0 -> senvcfg.PMM ineffective ----- */
TEST_REGISTER(test_hzpm_hlv_08);
bool test_hzpm_hlv_08(void) {
    TEST_BEGIN("HZPM-HLV-08: senvcfg.PMM ineffective for HLV in U-mode");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* sec:hstatus HUPMM paragraph: in U-mode, PM for HLV/HSV
     * performed as though in VU-mode is controlled by hstatus.HUPMM
     * -- NOT by senvcfg.PMM. With HUPMM=0 and senvcfg.PMM=PMLEN7,
     * the tagged access must fault. */
    pm_set_hupmm(PMM_DISABLED);
    if (!hzpm_try_set_u_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for U/VU-mode");

    two_stage_ctx_t ctx;
    hlv_setup_u(&ctx);

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    hstatus_set_spvp(PRIV_U);
    hstatus_set_hu(true);
    goto_priv(PRIV_U);
    trap_expect_begin();
    (void)hlv_d(tagged);
    TEST_ASSERT("HLV trapped (senvcfg.PMM must not apply in U-mode)",
                trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=13 (load page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    }
    trap_expect_end();
    goto_priv(PRIV_M);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HLV-09: HSV.D tagged store ----- */
TEST_REGISTER(test_hzpm_hlv_09);
bool test_hzpm_hlv_09(void) {
    TEST_BEGIN("HZPM-HLV-09: HSV in HS-mode, SPVP=1 tagged store");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    hlv_setup(&ctx);

    *(volatile uint64_t *)HZPM_DATA2 = 0;
    uintptr_t tagged = pm_tag_address(HZPM_DATA2, pm_alt_tag(7), 7);

    hstatus_set_spvp(PRIV_S);
    goto_priv(PRIV_S);
    hsv_d(tagged, HZPM_STORE_MAGIC);
    goto_priv(PRIV_M);

    TEST_ASSERT_EQ("HSV.D via tagged GVA reached untagged address",
                   *(volatile uint64_t *)HZPM_DATA2, HZPM_STORE_MAGIC);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HLV-10: MPRV does not change HLV PM selection ----- */
TEST_REGISTER(test_hzpm_hlv_10);
bool test_hzpm_hlv_10(void) {
    TEST_BEGIN("HZPM-HLV-10: MPRV does not affect HLV PM selection");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    hlv_setup(&ctx);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC3;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    /* norm:mstatus_mprv_hlsv: MPRV does not affect HLV/HSV; they
     * always act as though V=1 and privilege = hstatus.SPVP,
     * overriding MPRV. Set MPRV=1/MPV=0/MPP=U (which would select
     * U-mode PM for ordinary loads) -- HLV must still use
     * henvcfg.PMM via SPVP=1. */
    hstatus_set_spvp(PRIV_S);
    ts2_set_mprv_mpv(/*mpv=*/0, PRIV_U);
    uint64_t val = hlv_d(tagged);
    ts2_clear_mprv();

    TEST_ASSERT_EQ("HLV.D uses SPVP/henvcfg.PMM despite MPRV",
                   val, HZPM_MAGIC3);

    ts2_finish(&ctx);
    HYP_TEST_END();
}
