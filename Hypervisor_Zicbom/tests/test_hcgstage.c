/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hypervisor x Zicbom: G-stage address translation fault tests (Group 5)
 *
 * Tests CBO management instruction behavior under G-stage translation
 * when access is not permitted.
 *
 * Reference: Hypervisor_CMO_test_plan.md Group 5
 * Spec: norm:cbm_unperm_fault, norm:fault_excep_csr
 */

/* ===================================================================
 * HCGSTAGE-01: cbo.clean G-stage no permission guest-page-fault
 * =================================================================== */
TEST_REGISTER(test_hcgstage_01);
bool test_hcgstage_01(void)
{
    TEST_BEGIN("HCGSTAGE-01: cbo.clean G-stage no-perm GPF");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim, 0 /* invalid */);

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
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCGSTAGE-02: cbo.flush G-stage no permission guest-page-fault
 * =================================================================== */
TEST_REGISTER(test_hcgstage_02);
bool test_hcgstage_02(void)
{
    TEST_BEGIN("HCGSTAGE-02: cbo.flush G-stage no-perm GPF");

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
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCGSTAGE-03: cbo.inval G-stage no permission guest-page-fault
 * =================================================================== */
TEST_REGISTER(test_hcgstage_03);
bool test_hcgstage_03(void)
{
    TEST_BEGIN("HCGSTAGE-03: cbo.inval G-stage no-perm GPF");

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
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCGSTAGE-05: cbo.clean G-stage GPF stval=rs1
 * =================================================================== */
TEST_REGISTER(test_hcgstage_05);
bool test_hcgstage_05(void)
{
    TEST_BEGIN("HCGSTAGE-05: cbo.clean G-stage GPF stval=rs1");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim, 0);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(1);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_clean, victim);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=23",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
        /* stval should be the faulting address (rs1 value) */
        TEST_ASSERT_EQ("stval=rs1 (victim address)",
                       trap_get_tval(), victim);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCGSTAGE-08: cbo.clean read-only G-stage executes (management
 *              only needs any access permitted)
 * =================================================================== */
TEST_REGISTER(test_hcgstage_08);
bool test_hcgstage_08(void)
{
    TEST_BEGIN("HCGSTAGE-08: cbo.clean read-only G-stage executes");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    /* G-stage: read-only (R=1, W=0, U=1, A=1, D=1) */
    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim,
                            PTE_V | PTE_R | PTE_U | PTE_A | PTE_D);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(1);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_clean, victim);
    trap_expect_end();

    /* Per spec: management instruction succeeds if any access permitted */
    TEST_ASSERT("no trap (read-only is sufficient)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCGSTAGE-09: cbo.inval read-only G-stage executes
 * =================================================================== */
TEST_REGISTER(test_hcgstage_09);
bool test_hcgstage_09(void)
{
    TEST_BEGIN("HCGSTAGE-09: cbo.inval read-only G-stage executes");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    /* G-stage: read-only */
    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim,
                            PTE_V | PTE_R | PTE_U | PTE_A | PTE_D);

    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_INVAL);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_inval, victim);
    trap_expect_end();

    TEST_ASSERT("no trap (read-only is sufficient)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCGSTAGE-12: cbo.clean VS-stage page-fault (not G-stage)
 * =================================================================== */
TEST_REGISTER(test_hcgstage_12);
bool test_hcgstage_12(void)
{
    TEST_BEGIN("HCGSTAGE-12: cbo.clean VS-stage page-fault");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    /* VS-stage: victim page invalid; G-stage: full access */
    ts2_setup_with_vs_victim(&ctx, SUITE_VSATP_MODE, SUITE_HGATP_MODE,
                             victim, 0 /* invalid VS-stage PTE */);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(1);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_clean, victim);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        /* Should be store page-fault (cause=15), NOT guest-page-fault */
        TEST_ASSERT_EQ("cause=15 (store page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    }

    ts2_finish(&ctx);
    TEST_END();
}
