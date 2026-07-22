/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hypervisor x Zicbom: htinst standard transformation tests (Group 4)
 *
 * Tests htinst/mtinst standard transformation for cbo.inval, cbo.clean,
 * cbo.flush when they trigger traps.
 *
 * Reference: Hypervisor_CMO_test_plan.md Group 4
 * Spec: norm:h_trans_cache
 */

/* ===================================================================
 * HCXINST-01: cbo.clean page-fault htinst standard transformation
 * =================================================================== */
TEST_REGISTER(test_hcxinst_01);
bool test_hcxinst_01(void)
{
    TEST_BEGIN("HCXINST-01: cbo.clean page-fault htinst");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    /* G-stage: victim page has no permission */
    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim, 0 /* invalid */);

    /* Enable cbo.clean for VS-mode */
    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(1);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_clean, victim);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=23 (store guest-page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
        /* htinst should be standard transformation or 0 */
        uintptr_t htinst = trap_get_htinst();
        bool ok = (htinst == HTINST_CBO_CLEAN) || (htinst == 0);
        TEST_ASSERT("htinst=0x0010200F or 0", ok);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCXINST-02: cbo.flush page-fault htinst standard transformation
 * =================================================================== */
TEST_REGISTER(test_hcxinst_02);
bool test_hcxinst_02(void)
{
    TEST_BEGIN("HCXINST-02: cbo.flush page-fault htinst");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim, 0);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(1);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_flush, victim);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=23 (store guest-page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
        uintptr_t htinst = trap_get_htinst();
        bool ok = (htinst == HTINST_CBO_FLUSH) || (htinst == 0);
        TEST_ASSERT("htinst=0x0020200F or 0", ok);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCXINST-03: cbo.inval page-fault htinst standard transformation
 * =================================================================== */
TEST_REGISTER(test_hcxinst_03);
bool test_hcxinst_03(void)
{
    TEST_BEGIN("HCXINST-03: cbo.inval page-fault htinst");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim, 0);

    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_INVAL);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_inval, victim);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=23 (store guest-page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
        uintptr_t htinst = trap_get_htinst();
        bool ok = (htinst == HTINST_CBO_INVAL) || (htinst == 0);
        TEST_ASSERT("htinst=0x0000200F or 0", ok);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCXINST-07: cbo.inval virtual-instruction htinst
 * =================================================================== */
TEST_REGISTER(test_hcxinst_07);
bool test_hcxinst_07(void)
{
    TEST_BEGIN("HCXINST-07: cbo.inval virtual-inst htinst");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* henvcfg.CBIE=00 -> virtual-instruction */
    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_DISABLE);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        uintptr_t htinst = trap_get_htinst();
        bool ok = (htinst == HTINST_CBO_INVAL) || (htinst == 0);
        TEST_ASSERT("htinst=0x0000200F or 0", ok);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCXINST-08: cbo.clean virtual-instruction htinst
 * =================================================================== */
TEST_REGISTER(test_hcxinst_08);
bool test_hcxinst_08(void)
{
    TEST_BEGIN("HCXINST-08: cbo.clean virtual-inst htinst");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* henvcfg.CBCFE=0 -> virtual-instruction */
    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_clean, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        uintptr_t htinst = trap_get_htinst();
        bool ok = (htinst == HTINST_CBO_CLEAN) || (htinst == 0);
        TEST_ASSERT("htinst=0x0010200F or 0", ok);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCXINST-10: VU-mode cbo.inval virtual-instruction htinst
 * =================================================================== */
TEST_REGISTER(test_hcxinst_10);
bool test_hcxinst_10(void)
{
    TEST_BEGIN("HCXINST-10: VU-mode cbo.inval virtual-inst htinst");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_DISABLE);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        uintptr_t htinst = trap_get_htinst();
        bool ok = (htinst == HTINST_CBO_INVAL) || (htinst == 0);
        TEST_ASSERT("htinst=0x0000200F or 0", ok);
    }

    ts2_finish(&ctx);
    TEST_END();
}
