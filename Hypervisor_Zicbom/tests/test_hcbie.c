/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hypervisor x Zicbom: henvcfg.CBIE control tests (Group 1)
 *
 * Tests henvcfg.CBIE control of VS/VU-mode cbo.inval execution,
 * including flush override semantics.
 *
 * Reference: Hypervisor_CMO_test_plan.md Group 1
 * Spec: norm:henvcfg_cbie, norm:cbo-inval_h-mode_veq1_op,
 *       norm:cbo-inval_h-mode_op0/op1/op2
 */

/* ===================================================================
 * HCBIE-01: VS-mode henvcfg.CBIE=00 triggers virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hcbie_01);
bool test_hcbie_01(void)
{
    TEST_BEGIN("HCBIE-01: VS-mode henvcfg.CBIE=00 virtual-instruction");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* menvcfg.CBIE=11 (HS-mode enabled), henvcfg.CBIE=00 */
    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_DISABLE);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_inval, addr);
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
 * HCBIE-02: VS-mode henvcfg.CBIE=01 executes flush
 * =================================================================== */
TEST_REGISTER(test_hcbie_02);
bool test_hcbie_02(void)
{
    TEST_BEGIN("HCBIE-02: VS-mode henvcfg.CBIE=01 executes flush");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* menvcfg.CBIE=11, henvcfg.CBIE=01 (flush) */
    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_FLUSH);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (flush executed)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBIE-03: VS-mode henvcfg.CBIE=11 executes invalidate
 * =================================================================== */
TEST_REGISTER(test_hcbie_03);
bool test_hcbie_03(void)
{
    TEST_BEGIN("HCBIE-03: VS-mode henvcfg.CBIE=11 executes invalidate");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* menvcfg.CBIE=11, henvcfg.CBIE=11 (invalidate) */
    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_INVAL);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (invalidate executed)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBIE-04: VU-mode henvcfg.CBIE=00 triggers virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hcbie_04);
bool test_hcbie_04(void)
{
    TEST_BEGIN("HCBIE-04: VU-mode henvcfg.CBIE=00 virtual-instruction");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* menvcfg.CBIE=11, henvcfg.CBIE=00 */
    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_DISABLE);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_inval, addr);
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
 * HCBIE-05: VU-mode senvcfg.CBIE=00 triggers virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hcbie_05);
bool test_hcbie_05(void)
{
    TEST_BEGIN("HCBIE-05: VU-mode senvcfg.CBIE=00 virtual-instruction");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* menvcfg.CBIE=11, henvcfg.CBIE=11, senvcfg.CBIE=00 */
    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_INVAL);
    senvcfg_set_cbie(CBIE_DISABLE);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_inval, addr);
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
 * HCBIE-06: VU-mode both CBIE non-zero executes
 * =================================================================== */
TEST_REGISTER(test_hcbie_06);
bool test_hcbie_06(void)
{
    TEST_BEGIN("HCBIE-06: VU-mode two-level CBIE non-zero executes");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* menvcfg.CBIE=11, henvcfg.CBIE=11, senvcfg.CBIE=11 */
    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_INVAL);
    senvcfg_set_cbie(CBIE_INVAL);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (invalidate executed)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBIE-07: VU-mode henvcfg.CBIE=01 executes flush
 * =================================================================== */
TEST_REGISTER(test_hcbie_07);
bool test_hcbie_07(void)
{
    TEST_BEGIN("HCBIE-07: VU-mode henvcfg.CBIE=01 executes flush");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* menvcfg.CBIE=11, henvcfg.CBIE=01, senvcfg.CBIE=11 */
    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_FLUSH);
    senvcfg_set_cbie(CBIE_INVAL);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (flush executed)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBIE-08: VU-mode senvcfg.CBIE=01 executes flush
 * =================================================================== */
TEST_REGISTER(test_hcbie_08);
bool test_hcbie_08(void)
{
    TEST_BEGIN("HCBIE-08: VU-mode senvcfg.CBIE=01 executes flush");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* menvcfg.CBIE=11, henvcfg.CBIE=11, senvcfg.CBIE=01 */
    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_INVAL);
    senvcfg_set_cbie(CBIE_FLUSH);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (flush executed)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBIE-09: henvcfg.CBIE WARL reserved encoding 10
 * =================================================================== */
TEST_REGISTER(test_hcbie_09);
bool test_hcbie_09(void)
{
    TEST_BEGIN("HCBIE-09: henvcfg.CBIE WARL reserved encoding 10");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    /* Write CBIE=10 (reserved), read back - should not retain 10 */
    henvcfg_set_cbie(CBIE_RESERVED);
    uintptr_t readback = henvcfg_get_cbie();

    TEST_ASSERT("CBIE=10 not retained (WARL)", readback != CBIE_RESERVED);

    hyp_reset_state();
    TEST_END();
}

/* ===================================================================
 * HCBIE-10: henvcfg.CBIE read-only zero when Zicbom not implemented
 *
 * Note: This test can only verify on platforms without Zicbom.
 * If Zicbom IS implemented, we skip.
 * =================================================================== */
TEST_REGISTER(test_hcbie_10);
bool test_hcbie_10(void)
{
    TEST_BEGIN("HCBIE-10: henvcfg.CBIE read-only zero (no Zicbom)");

    H_REQUIRED_OR_SKIP();

    if (check_zicbom_extension()) {
        TEST_SKIP("Zicbom implemented, cannot test read-only-zero");
    }

    uintptr_t val = henvcfg_get_cbie();
    TEST_ASSERT_EQ("henvcfg.CBIE is read-only zero", val, (uintptr_t)0);

    hyp_reset_state();
    TEST_END();
}

/* ===================================================================
 * HCBIE-11: menvcfg.CBIE=01 forces flush in VS-mode (overrides
 *           henvcfg.CBIE=11)
 * =================================================================== */
TEST_REGISTER(test_hcbie_11);
bool test_hcbie_11(void)
{
    TEST_BEGIN("HCBIE-11: menvcfg.CBIE=01 forces VS-mode flush");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* menvcfg.CBIE=01 (HS-mode flush), henvcfg.CBIE=11 */
    menvcfg_set_cbie(CBIE_FLUSH);
    henvcfg_set_cbie(CBIE_INVAL);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    /* Should execute (as flush), no trap */
    TEST_ASSERT("no trap (forced flush)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBIE-12: menvcfg.CBIE=01 forces flush in VU-mode
 * =================================================================== */
TEST_REGISTER(test_hcbie_12);
bool test_hcbie_12(void)
{
    TEST_BEGIN("HCBIE-12: menvcfg.CBIE=01 forces VU-mode flush");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* menvcfg.CBIE=01, henvcfg.CBIE=11, senvcfg.CBIE=11 */
    menvcfg_set_cbie(CBIE_FLUSH);
    henvcfg_set_cbie(CBIE_INVAL);
    senvcfg_set_cbie(CBIE_INVAL);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (forced flush)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCBIE-13: henvcfg.CBIE does not affect menvcfg/senvcfg.CBIE
 * =================================================================== */
TEST_REGISTER(test_hcbie_13);
bool test_hcbie_13(void)
{
    TEST_BEGIN("HCBIE-13: henvcfg.CBIE does not affect others");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    /* Set menvcfg.CBIE=11, senvcfg.CBIE=11 first */
    menvcfg_set_cbie(CBIE_INVAL);
    senvcfg_set_cbie(CBIE_INVAL);

    /* Write henvcfg.CBIE=00 */
    henvcfg_set_cbie(CBIE_DISABLE);

    /* menvcfg.CBIE and senvcfg.CBIE should be unaffected */
    TEST_ASSERT_EQ("menvcfg.CBIE unaffected",
                   menvcfg_get_cbie(), (uintptr_t)CBIE_INVAL);
    TEST_ASSERT_EQ("senvcfg.CBIE unaffected",
                   senvcfg_get_cbie(), (uintptr_t)CBIE_INVAL);

    hyp_reset_state();
    TEST_END();
}
