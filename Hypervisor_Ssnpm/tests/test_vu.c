/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_vu.c - Group 3: H x Ssnpm - VU-mode pointer masking
 *
 * HZPM-VU-01 ~ HZPM-VU-07
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 3.
 *
 * Spec references:
 *   - norm:senvcfg_pmm_Ssnpm (supervisor.adoc): senvcfg.PMM controls
 *     PM for the next-lower privilege mode (U/VU).
 *   - norm:H_scsrs_nomatch (hypervisor.adoc): senvcfg has no VS CSR;
 *     it keeps its usual function and accessibility when V=1.
 *   - norm:pm_ignore_va / norm:pm_ignore_pa / norm:pm_per_mode_control.
 */

/* ----- HZPM-VU-01: PMLEN7 tagged load in VU-mode ----- */
TEST_REGISTER(test_hzpm_vu_01);
bool test_hzpm_vu_01(void) {
    TEST_BEGIN("HZPM-VU-01: PMLEN7 tagged load in VU-mode");
    SSNPM_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_u_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for U/VU-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    uintptr_t r = two_stage_run_in_vu(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged load returns correct value", r, HZPM_MAGIC1);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VU-02: PMLEN16 tagged load in VU-mode ----- */
TEST_REGISTER(test_hzpm_vu_02);
bool test_hzpm_vu_02(void) {
    TEST_BEGIN("HZPM-VU-02: PMLEN16 tagged load in VU-mode");
    SSNPM_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_u_pmm(PMM_PMLEN16))
        TEST_SKIP("PMLEN=16 not supported for U/VU-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC2;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(16), 16);

    uintptr_t r = two_stage_run_in_vu(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged load returns correct value", r, HZPM_MAGIC2);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VU-03: PMLEN7 tagged store in VU-mode ----- */
TEST_REGISTER(test_hzpm_vu_03);
bool test_hzpm_vu_03(void) {
    TEST_BEGIN("HZPM-VU-03: PMLEN7 tagged store in VU-mode");
    SSNPM_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_u_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for U/VU-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA2 = 0;
    uintptr_t tagged = pm_tag_address(HZPM_DATA2, pm_alt_tag(7), 7);

    two_stage_run_in_vu(&ctx, hzpm_store64, tagged);
    TEST_ASSERT_EQ("store via tagged VA reached untagged address",
                   *(volatile uint64_t *)HZPM_DATA2, HZPM_STORE_MAGIC);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VU-04: VU PM disabled -> tagged VA faults ----- */
TEST_REGISTER(test_hzpm_vu_04);
bool test_hzpm_vu_04(void) {
    TEST_BEGIN("HZPM-VU-04: PM disabled, tagged VA page-fault (VU)");
    SSNPM_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    pm_set_umode(PMM_DISABLED);

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    trap_expect_begin();
    two_stage_run_in_vu(&ctx, hzpm_load64, tagged);
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

/* ----- HZPM-VU-05: VS-mode writes senvcfg.PMM directly ----- */
TEST_REGISTER(test_hzpm_vu_05);
bool test_hzpm_vu_05(void) {
    TEST_BEGIN("HZPM-VU-05: VS-mode writes senvcfg.PMM (no VS CSR)");
    SSNPM_REQUIRED_OR_SKIP();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* norm:H_scsrs_nomatch: senvcfg has no matching VS CSR and keeps
     * its usual accessibility when V=1. If Smstateen is implemented,
     * open the ENVCFG gates so VS-mode may access senvcfg. */
    if (stateen_is_available()) {
        mstateen_set_bits(0, STATEEN0_ENVCFG);
        hstateen_set_bits(0, STATEEN0_ENVCFG);
    }

    /* Single context with vsatp=Bare: both VS-mode and VU-mode
     * fetch via G-stage (U=1) pages, so one setup serves both
     * privilege levels, and senvcfg.PMM persists across the two
     * runs (no ts2_finish in between). The tagged access uses the
     * GPA zero-extend path (norm:pm_ignore_pa). */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Guest OS (VS-mode) configures its own VU-mode PM */
    pm_set_umode(PMM_DISABLED);
    VS_EXPECT_NO_TRAP(
        two_stage_run_in_vs(&ctx, hzpm_write_senvcfg_pmm, PMM_PMLEN7));
    TEST_ASSERT_EQ("senvcfg.PMM written from VS-mode",
                   pm_get_umode(), PMM_PMLEN7);

    /* VU-mode tagged load must now succeed */
    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC3;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);
    uintptr_t r = two_stage_run_in_vu(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("VU tagged load after VS-side senvcfg write",
                   r, HZPM_MAGIC3);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-VU-06: VU PM independent of VS PM ----- */
TEST_REGISTER(test_hzpm_vu_06);
bool test_hzpm_vu_06(void) {
    TEST_BEGIN("HZPM-VU-06: VU PM independent of VS PM");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_u_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for U/VU-mode");

    /* henvcfg.PMM=0 (VS PM off), senvcfg.PMM=PMLEN7 (VU PM on) */
    pm_set_vsmode(PMM_DISABLED);
    TEST_ASSERT_EQ("henvcfg.PMM disabled", pm_get_vsmode(), PMM_DISABLED);

    /* Part 1: U=1 mappings, VU tagged load succeeds (VU PM on) */
    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    uintptr_t r = two_stage_run_in_vu(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("VU tagged load works with henvcfg.PMM=0",
                   r, HZPM_MAGIC1);
    ts2_finish(&ctx);

    /* Part 2: S-level mappings (VS-mode cannot fetch from U=1
     * pages), VS tagged load faults (VS PM off): invalid Sv39 VA. */
    two_stage_ctx_t sctx;
    ts2_setup_full(&sctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    trap_expect_begin();
    two_stage_run_in_vs(&sctx, hzpm_load64, tagged);
    TEST_ASSERT("VS tagged load trapped", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=13 (load page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    }
    trap_expect_end();

    ts2_finish(&sctx);
    HYP_TEST_END();
}

/* ----- HZPM-VU-07: VU-mode GPA path zero-extend ----- */
TEST_REGISTER(test_hzpm_vu_07);
bool test_hzpm_vu_07(void) {
    TEST_BEGIN("HZPM-VU-07: GPA zero-extend (vsatp=Bare, VU-mode)");
    SSNPM_REQUIRED_OR_SKIP();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_u_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for U/VU-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* norm:pm_ignore_pa: with vsatp.MODE=Bare the VU effective
     * address is a GPA; the upper PMLEN bits are replaced with 0. */
    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC2;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    uintptr_t r = two_stage_run_in_vu(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged GPA load returns correct value",
                   r, HZPM_MAGIC2);

    ts2_finish(&ctx);
    HYP_TEST_END();
}
