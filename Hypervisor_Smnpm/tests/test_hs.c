/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hs.c - Group 6: H x Smnpm - HS-mode pointer masking
 *
 * HZPM-HS-01 ~ HZPM-HS-06
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 6.
 *
 * Spec references:
 *   - norm:menvcfg_pmm_op (machine.adoc): menvcfg.PMM controls PM
 *     for the next-lower privilege mode (S-/HS-mode).
 *   - norm:smnpm_definition (zpm.adoc).
 *   - norm:pm_per_mode_control / norm:pm_mode_only_dependency:
 *     menvcfg.PMM must NOT affect VS/VU-mode; henvcfg.PMM and
 *     senvcfg.PMM must NOT affect HS-mode.
 *   - norm:pm_ignore_va / norm:pm_ignore_pa.
 */

static volatile uint64_t hs_data __attribute__((aligned(8)));
static volatile uint64_t hs_amo  __attribute__((aligned(8)));

/* ----- HZPM-HS-01: PMLEN7 HS-mode tagged load (VA path) ----- */
TEST_REGISTER(test_hzpm_hs_01);
bool test_hzpm_hs_01(void) {
    TEST_BEGIN("HZPM-HS-01: PMLEN7 tagged load in HS-mode (Sv39)");
    SMNPM_REQUIRED_OR_SKIP();
    REQUIRE_SATP_SV39();
    if (!hzpm_try_set_s_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for S/HS-mode");

    pt_context_t ctx;
    hzpm_setup_hs_vm(&ctx);

    hs_data = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address((uintptr_t)&hs_data,
                                      pm_max_tag(7), 7);

    uintptr_t r = vm_run_in_smode(&ctx, hzpm_hs_load64, tagged);
    TEST_ASSERT_EQ("HS tagged load correct (sign-extend)",
                   r, HZPM_MAGIC1);

    pt_pool_reset();
    HYP_TEST_END();
}

/* ----- HZPM-HS-02: HS-mode Bare tagged load (PA path) ----- */
TEST_REGISTER(test_hzpm_hs_02);
bool test_hzpm_hs_02(void) {
    TEST_BEGIN("HZPM-HS-02: PMLEN7 tagged load in HS-mode (Bare)");
    SMNPM_REQUIRED_OR_SKIP();
    if (!hzpm_try_set_s_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for S/HS-mode");

    /* satp=Bare (reset baseline): HS effective address is a physical
     * address -> zero-extend transformation (norm:pm_ignore_pa). */
    hs_data = HZPM_MAGIC2;
    uintptr_t tagged = pm_tag_address((uintptr_t)&hs_data,
                                      pm_max_tag(7), 7);

    goto_priv(PRIV_S);
    uint64_t val = *(volatile uint64_t *)tagged;
    goto_priv(PRIV_M);

    TEST_ASSERT_EQ("HS Bare tagged load correct (zero-extend)",
                   val, HZPM_MAGIC2);
    HYP_TEST_END();
}

/* ----- HZPM-HS-03: HS PM independent of VS PM ----- */
TEST_REGISTER(test_hzpm_hs_03);
bool test_hzpm_hs_03(void) {
    TEST_BEGIN("HZPM-HS-03: HS PM independent of VS PM");
    SMNPM_REQUIRED_OR_SKIP();
    REQUIRE_SATP_SV39();
    REQUIRE_VSATP_SV39();
    if (!detect_ssnpm_hyp())
        TEST_SKIP("Ssnpm hyp controls not implemented");
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    /* menvcfg.PMM=0 (HS PM off), henvcfg.PMM=PMLEN7 (VS PM on) */
    pm_set_smode(PMM_DISABLED);
    TEST_ASSERT_EQ("menvcfg.PMM disabled", pm_get_smode(), PMM_DISABLED);

    /* (a) HS-mode tagged load must fault (HS PM off): the tagged VA
     * is invalid for Sv39 -> load page-fault (13). */
    pt_context_t sctx;
    hzpm_setup_hs_vm(&sctx);

    hs_data = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address((uintptr_t)&hs_data,
                                      pm_max_tag(7), 7);

    trap_expect_begin();
    vm_run_in_smode(&sctx, hzpm_hs_load64, tagged);
    TEST_ASSERT("HS tagged load trapped", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=13 (load page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    }
    trap_expect_end();
    pt_pool_reset();

    /* (b) VS-mode tagged load must succeed (VS PM on) */
    two_stage_ctx_t vctx;
    ts2_setup_full(&vctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC1;
    uintptr_t vtagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);
    uintptr_t r = two_stage_run_in_vs(&vctx, hzpm_load64, vtagged);
    TEST_ASSERT_EQ("VS tagged load works with menvcfg.PMM=0",
                   r, HZPM_MAGIC1);

    ts2_finish(&vctx);
    HYP_TEST_END();
}

/* ----- HZPM-HS-04: menvcfg.PMM does not affect VS-mode ----- */
TEST_REGISTER(test_hzpm_hs_04);
bool test_hzpm_hs_04(void) {
    TEST_BEGIN("HZPM-HS-04: menvcfg.PMM does not affect VS-mode");
    SMNPM_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!detect_ssnpm_hyp())
        TEST_SKIP("Ssnpm hyp controls not implemented");
    if (!hzpm_try_set_s_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for S/HS-mode");

    /* menvcfg.PMM=PMLEN7 (HS PM on), henvcfg.PMM=0 (VS PM off) */
    pm_set_vsmode(PMM_DISABLED);
    TEST_ASSERT_EQ("henvcfg.PMM disabled", pm_get_vsmode(), PMM_DISABLED);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    /* VS-mode tagged load must fault: menvcfg.PMM must not enable
     * PM for VS-mode (norm:pm_per_mode_control). */
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT("VS tagged load trapped", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=13 (load page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    }
    trap_expect_end();

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HS-05: menvcfg.PMM does not affect VU-mode ----- */
TEST_REGISTER(test_hzpm_hs_05);
bool test_hzpm_hs_05(void) {
    TEST_BEGIN("HZPM-HS-05: menvcfg.PMM does not affect VU-mode");
    SMNPM_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!detect_ssnpm())
        TEST_SKIP("Ssnpm (senvcfg.PMM) not implemented");
    if (!hzpm_try_set_s_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for S/HS-mode");

    /* menvcfg.PMM=PMLEN7 (HS PM on), senvcfg.PMM=0 (VU PM off) */
    pm_set_umode(PMM_DISABLED);
    TEST_ASSERT_EQ("senvcfg.PMM disabled", pm_get_umode(), PMM_DISABLED);

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    /* VU-mode tagged load must fault: menvcfg.PMM must not enable
     * PM for VU-mode. */
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, hzpm_load64, tagged);
    TEST_ASSERT("VU tagged load trapped", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=13 (load page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    }
    trap_expect_end();

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-HS-06: PMLEN7 HS-mode AMO ----- */
TEST_REGISTER(test_hzpm_hs_06);
bool test_hzpm_hs_06(void) {
    TEST_BEGIN("HZPM-HS-06: PMLEN7 amoadd.d in HS-mode (Sv39)");
    SMNPM_REQUIRED_OR_SKIP();
    REQUIRE_SATP_SV39();
    if (!hzpm_try_set_s_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for S/HS-mode");

    pt_context_t ctx;
    hzpm_setup_hs_vm(&ctx);

    hs_amo = 100;
    uintptr_t tagged = pm_tag_address((uintptr_t)&hs_amo,
                                      pm_max_tag(7), 7);

    uintptr_t old = vm_run_in_smode(&ctx, hzpm_hs_amoadd64, tagged);
    TEST_ASSERT_EQ("AMO old value", old, 100);
    TEST_ASSERT_EQ("AMO updated value", hs_amo, 100 + HZPM_AMO_ADDEND);

    pt_pool_reset();
    HYP_TEST_END();
}
