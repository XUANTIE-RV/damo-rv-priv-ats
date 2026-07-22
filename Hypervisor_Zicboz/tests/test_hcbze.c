/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hypervisor x Zicboz: henvcfg.CBZE control tests (Group 3)
 *
 * Tests henvcfg.CBZE control of VS/VU-mode cbo.zero execution.
 *
 * Reference: Hypervisor_CMO_test_plan.md Group 3
 * Spec: norm:henvcfg_cbze, norm:cbxe_unaffected
 */

/* ===================================================================
 * HCBZE-01: VS-mode henvcfg.CBZE=0 triggers virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hcbze_01);
bool test_hcbze_01(void)
{
    TEST_BEGIN("HCBZE-01: VS-mode CBZE=0 virtual-instruction");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_zero, addr);
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
 * HCBZE-02: VS-mode henvcfg.CBZE=1 executes normally
 * =================================================================== */
TEST_REGISTER(test_hcbze_02);
bool test_hcbze_02(void)
{
    TEST_BEGIN("HCBZE-02: VS-mode CBZE=1 executes normally");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(1);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_zero, addr);
    trap_expect_end();

    TEST_ASSERT("no trap", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBZE-03: VU-mode henvcfg.CBZE=0 triggers virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hcbze_03);
bool test_hcbze_03(void)
{
    TEST_BEGIN("HCBZE-03: VU-mode henvcfg.CBZE=0 virtual-instruction");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_zero, addr);
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
 * HCBZE-04: VU-mode senvcfg.CBZE=0 triggers virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hcbze_04);
bool test_hcbze_04(void)
{
    TEST_BEGIN("HCBZE-04: VU-mode senvcfg.CBZE=0 virtual-instruction");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(1);
    senvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_zero, addr);
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
 * HCBZE-05: VU-mode two-level CBZE=1 executes normally
 * =================================================================== */
TEST_REGISTER(test_hcbze_05);
bool test_hcbze_05(void)
{
    TEST_BEGIN("HCBZE-05: VU-mode two-level CBZE=1 executes");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(1);
    senvcfg_set_cbze(1);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_zero, addr);
    trap_expect_end();

    TEST_ASSERT("no trap", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBZE-06: henvcfg.CBZE read-only zero when Zicboz not implemented
 * =================================================================== */
TEST_REGISTER(test_hcbze_06);
bool test_hcbze_06(void)
{
    TEST_BEGIN("HCBZE-06: henvcfg.CBZE read-only zero (no Zicboz)");

    H_REQUIRED_OR_SKIP();

    if (check_zicboz_extension()) {
        TEST_SKIP("Zicboz implemented, cannot test read-only-zero");
    }

    uintptr_t val = henvcfg_get_cbze();
    TEST_ASSERT_EQ("henvcfg.CBZE is read-only zero", val, (uintptr_t)0);

    hyp_reset_state();
    TEST_END();
}

/* ===================================================================
 * HCBZE-07: henvcfg.CBZE does not affect menvcfg/senvcfg.CBZE
 * =================================================================== */
TEST_REGISTER(test_hcbze_07);
bool test_hcbze_07(void)
{
    TEST_BEGIN("HCBZE-07: henvcfg.CBZE does not affect others");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    /* Set menvcfg.CBZE=1, senvcfg.CBZE=1 first */
    menvcfg_set_cbze(1);
    senvcfg_set_cbze(1);

    /* Clear henvcfg.CBZE */
    henvcfg_set_cbze(0);

    /* menvcfg and senvcfg should be unaffected */
    TEST_ASSERT_EQ("menvcfg.CBZE unaffected",
                   menvcfg_get_cbze(), (uintptr_t)1);
    TEST_ASSERT_EQ("senvcfg.CBZE unaffected",
                   senvcfg_get_cbze(), (uintptr_t)1);

    hyp_reset_state();
    TEST_END();
}
