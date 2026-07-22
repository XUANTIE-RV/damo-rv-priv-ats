/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_vs.c - Group 2: H x Ssnpm - VS-mode pointer masking
 *
 * HZPM-VS-01 ~ HZPM-VS-10
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 2.
 *
 * Spec references:
 *   - norm:henvcfg_pmm_op: henvcfg.PMM controls VS-mode PM.
 *   - norm:pm_ignore_va: vsatp.MODE != Bare -> VA sign-extend.
 *   - norm:pm_ignore_pa: vsatp.MODE == Bare -> GPA zero-extend.
 *   - norm:pm_per_mode_control / norm:pm_mode_only_dependency.
 */

/* ===================================================================
 * VA path (vsatp=Sv39, hgatp=Bare): sign-extend transformation
 * =================================================================== */

/* ----- HZPM-VS-01: PMLEN7 tagged load in VS-mode (VA path) ----- */
TEST_REGISTER(test_hzpm_vs_01);
bool test_hzpm_vs_01(void) {
    TEST_BEGIN("HZPM-VS-01: PMLEN7 tagged load in VS-mode");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);
    TEST_ASSERT("tagged differs from base", tagged != HZPM_DATA1);

    uintptr_t r = two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged load returns correct value", r, HZPM_MAGIC1);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VS-02: PMLEN16 tagged load in VS-mode (VA path) ----- */
TEST_REGISTER(test_hzpm_vs_02);
bool test_hzpm_vs_02(void) {
    TEST_BEGIN("HZPM-VS-02: PMLEN16 tagged load in VS-mode");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN16))
        TEST_SKIP("PMLEN=16 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC2;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(16), 16);

    uintptr_t r = two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged load returns correct value", r, HZPM_MAGIC2);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VS-03: PMLEN7 tagged store in VS-mode ----- */
TEST_REGISTER(test_hzpm_vs_03);
bool test_hzpm_vs_03(void) {
    TEST_BEGIN("HZPM-VS-03: PMLEN7 tagged store in VS-mode");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA2 = 0;
    uintptr_t tagged = pm_tag_address(HZPM_DATA2, pm_alt_tag(7), 7);

    two_stage_run_in_vs(&ctx, hzpm_store64, tagged);
    TEST_ASSERT_EQ("store via tagged VA reached untagged address",
                   *(volatile uint64_t *)HZPM_DATA2, HZPM_STORE_MAGIC);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VS-04: PMLEN7 AMO in VS-mode ----- */
TEST_REGISTER(test_hzpm_vs_04);
bool test_hzpm_vs_04(void) {
    TEST_BEGIN("HZPM-VS-04: PMLEN7 amoadd.d via tagged VA in VS-mode");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA2 = 100;
    uintptr_t tagged = pm_tag_address(HZPM_DATA2, pm_max_tag(7), 7);

    uintptr_t old = two_stage_run_in_vs(&ctx, hzpm_amoadd64, tagged);
    TEST_ASSERT_EQ("AMO old value", old, 100);
    TEST_ASSERT_EQ("AMO updated value",
                   *(volatile uint64_t *)HZPM_DATA2, 100 + HZPM_AMO_ADDEND);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VS-05: different tags reach the same location ----- */
TEST_REGISTER(test_hzpm_vs_05);
bool test_hzpm_vs_05(void) {
    TEST_BEGIN("HZPM-VS-05: different tags, same location (VS-mode)");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC3;
    uintptr_t tagged_a = pm_tag_address(HZPM_DATA1, 0x55, 7);
    uintptr_t tagged_b = pm_tag_address(HZPM_DATA1, 0x7F, 7);

    uintptr_t ra = two_stage_run_in_vs(&ctx, hzpm_load64, tagged_a);
    uintptr_t rb = two_stage_run_in_vs(&ctx, hzpm_load64, tagged_b);
    TEST_ASSERT_EQ("tag 0x55 reads correct value", ra, HZPM_MAGIC3);
    TEST_ASSERT_EQ("tag 0x7F reads same value", rb, HZPM_MAGIC3);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VS-06: PM disabled -> tagged VA faults ----- */
TEST_REGISTER(test_hzpm_vs_06);
bool test_hzpm_vs_06(void) {
    TEST_BEGIN("HZPM-VS-06: PM disabled, tagged VA page-fault (VS)");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    pm_set_vsmode(PMM_DISABLED);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    /* With PM disabled the tagged VA is not transformed; bits[63:39]
     * are no longer the sign-extension of bit 38, so the VA is
     * invalid for Sv39 -> load page-fault (cause=13). */
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=13 (load page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    }
    trap_expect_end();

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VS-07: VA sign-extend correctness (bit 56 = 1) ----- */
TEST_REGISTER(test_hzpm_vs_07);
bool test_hzpm_vs_07(void) {
    TEST_BEGIN("HZPM-VS-07: VA sign-extend correctness in VS-mode");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    /* Map a high-half canonical Sv39 VA (bits[63:39] all ones,
     * bit 56 = 1) to the scratch GPA. After tagging bits[63:57],
     * a correct sign-extend from bit 56 (=1) restores the high-half
     * VA; a wrong zero-extend would produce an unmapped low VA. */
    uintptr_t high_va = 0xFFFFFFC000001000ULL;
    *(volatile uint64_t *)HZPM_DATA3 = HZPM_MAGIC1;
    int rc = two_stage_vs_map(&ctx, high_va, HZPM_DATA3,
                              VS_FLAGS_RWX_S_AD, PT_LEVEL_4K);
    TEST_ASSERT("high-half VA mapped", rc == 0);

    uintptr_t tagged = pm_tag_address(high_va, 0x55, 7);
    TEST_ASSERT_EQ("software model: sign-extend restores high VA",
                   pm_transform_va(tagged, 7), high_va);

    uintptr_t r = two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged high-half load correct (sign-extend)",
                   r, HZPM_MAGIC1);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VS-08: VS PM independent of HS PM (menvcfg.PMM) ----- */
TEST_REGISTER(test_hzpm_vs_08);
bool test_hzpm_vs_08(void) {
    TEST_BEGIN("HZPM-VS-08: VS PM independent of menvcfg.PMM");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    /* norm:pm_per_mode_control / norm:pm_mode_only_dependency:
     * VS-mode PM depends only on henvcfg.PMM, not on menvcfg.PMM. */
    if (detect_smnpm())
        pm_set_smode(PMM_DISABLED);
    TEST_ASSERT_EQ("menvcfg.PMM disabled", pm_get_smode(), PMM_DISABLED);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC2;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    uintptr_t r = two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("VS tagged load works with menvcfg.PMM=0",
                   r, HZPM_MAGIC2);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * GPA path (vsatp=Bare, hgatp=Sv39x4): zero-extend transformation
 * =================================================================== */

/* ----- HZPM-VS-09: GPA path zero-extend transformation ----- */
TEST_REGISTER(test_hzpm_vs_09);
bool test_hzpm_vs_09(void) {
    TEST_BEGIN("HZPM-VS-09: GPA zero-extend (vsatp=Bare, VS-mode)");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* norm:pm_ignore_pa: with vsatp.MODE=Bare the effective address
     * is a GPA and the upper PMLEN bits are replaced with 0.
     * (On this platform bit(XLEN-PMLEN-1)=0 for all mapped GPAs, so
     * zero-extend and sign-extend coincide; the test still proves PM
     * applies to GPA accesses and the tag is discarded.) */
    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    uintptr_t r = two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged GPA load returns correct value",
                   r, HZPM_MAGIC1);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VS-10: GPA path with PM disabled -> guest-page-fault ----- */
TEST_REGISTER(test_hzpm_vs_10);
bool test_hzpm_vs_10(void) {
    TEST_BEGIN("HZPM-VS-10: PM disabled, tagged GPA guest-page-fault");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    pm_set_vsmode(PMM_DISABLED);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* With PM disabled the tagged GPA keeps bits[63:57] set, which
     * exceeds the Sv39x4 GPA width -> guest-page-fault (cause=21). */
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=21 (load guest-page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    }
    trap_expect_end();

    ts2_finish(&ctx);
    HYP_TEST_END();
}
