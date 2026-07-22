/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hypervisor x Zicbom: henvcfg.CBCFE control tests (Group 2)
 *
 * Tests henvcfg.CBCFE control of VS/VU-mode cbo.clean and cbo.flush.
 *
 * Reference: Hypervisor_CMO_test_plan.md Group 2
 * Spec: norm:henvcfg_cbcfe, norm:cbxe_unaffected
 */

/* ===================================================================
 * HCBCFE-01: VS-mode henvcfg.CBCFE=0 cbo.clean virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hcbcfe_01);
bool test_hcbcfe_01(void)
{
    TEST_BEGIN("HCBCFE-01: VS-mode CBCFE=0 cbo.clean virtual-inst");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_clean, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBCFE-02: VS-mode henvcfg.CBCFE=0 cbo.flush virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hcbcfe_02);
bool test_hcbcfe_02(void)
{
    TEST_BEGIN("HCBCFE-02: VS-mode CBCFE=0 cbo.flush virtual-inst");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_flush, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBCFE-03: VS-mode henvcfg.CBCFE=1 cbo.clean executes
 * =================================================================== */
TEST_REGISTER(test_hcbcfe_03);
bool test_hcbcfe_03(void)
{
    TEST_BEGIN("HCBCFE-03: VS-mode CBCFE=1 cbo.clean executes");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(1);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_clean, addr);
    trap_expect_end();

    TEST_ASSERT("no trap", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBCFE-04: VS-mode henvcfg.CBCFE=1 cbo.flush executes
 * =================================================================== */
TEST_REGISTER(test_hcbcfe_04);
bool test_hcbcfe_04(void)
{
    TEST_BEGIN("HCBCFE-04: VS-mode CBCFE=1 cbo.flush executes");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(1);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_flush, addr);
    trap_expect_end();

    TEST_ASSERT("no trap", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBCFE-05: VU-mode henvcfg.CBCFE=0 virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hcbcfe_05);
bool test_hcbcfe_05(void)
{
    TEST_BEGIN("HCBCFE-05: VU-mode CBCFE=0 cbo.clean virtual-inst");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_clean, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBCFE-06: VU-mode senvcfg.CBCFE=0 virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hcbcfe_06);
bool test_hcbcfe_06(void)
{
    TEST_BEGIN("HCBCFE-06: VU-mode senvcfg.CBCFE=0 virtual-inst");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(1);
    senvcfg_set_cbcfe(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_clean, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBCFE-07: VU-mode two-level CBCFE=1 cbo.clean executes
 * =================================================================== */
TEST_REGISTER(test_hcbcfe_07);
bool test_hcbcfe_07(void)
{
    TEST_BEGIN("HCBCFE-07: VU-mode two-level CBCFE=1 cbo.clean");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(1);
    senvcfg_set_cbcfe(1);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_clean, addr);
    trap_expect_end();

    TEST_ASSERT("no trap", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBCFE-08: VU-mode two-level CBCFE=1 cbo.flush executes
 * =================================================================== */
TEST_REGISTER(test_hcbcfe_08);
bool test_hcbcfe_08(void)
{
    TEST_BEGIN("HCBCFE-08: VU-mode two-level CBCFE=1 cbo.flush");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(1);
    senvcfg_set_cbcfe(1);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_flush, addr);
    trap_expect_end();

    TEST_ASSERT("no trap", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBCFE-09: henvcfg.CBCFE read-only zero when Zicbom not implemented
 * =================================================================== */
TEST_REGISTER(test_hcbcfe_09);
bool test_hcbcfe_09(void)
{
    TEST_BEGIN("HCBCFE-09: henvcfg.CBCFE read-only zero (no Zicbom)");

    H_REQUIRED_OR_SKIP();

    if (check_zicbom_extension()) {
        TEST_SKIP("Zicbom implemented, cannot test read-only-zero");
    }

    uintptr_t val = henvcfg_get_cbcfe();
    TEST_ASSERT_EQ("henvcfg.CBCFE is read-only zero", val, (uintptr_t)0);

    hyp_reset_state();
    TEST_END();
}

/* ===================================================================
 * HCBCFE-10: henvcfg.CBCFE does not affect menvcfg/senvcfg.CBCFE
 * =================================================================== */
TEST_REGISTER(test_hcbcfe_10);
bool test_hcbcfe_10(void)
{
    TEST_BEGIN("HCBCFE-10: henvcfg.CBCFE does not affect others");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    /* Set menvcfg.CBCFE=1, senvcfg.CBCFE=1 first */
    menvcfg_set_cbcfe(1);
    senvcfg_set_cbcfe(1);

    /* Clear henvcfg.CBCFE */
    henvcfg_set_cbcfe(0);

    /* menvcfg and senvcfg should be unaffected */
    TEST_ASSERT_EQ("menvcfg.CBCFE unaffected",
                   menvcfg_get_cbcfe(), (uintptr_t)1);
    TEST_ASSERT_EQ("senvcfg.CBCFE unaffected",
                   senvcfg_get_cbcfe(), (uintptr_t)1);

    hyp_reset_state();
    TEST_END();
}
