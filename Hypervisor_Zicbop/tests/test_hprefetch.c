/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hypervisor x Zicbop: Prefetch VS/VU-mode behavior tests (Group 7)
 *
 * Tests that prefetch instructions in VS/VU-mode do not trigger
 * virtual-instruction exceptions regardless of henvcfg CMO fields,
 * and do not raise exceptions on G-stage faults.
 *
 * Reference: Hypervisor_CMO_test_plan.md Group 7
 * Spec: norm:cbp_unperm_noexcep
 */

/* ===================================================================
 * HPREFETCH-01: VS-mode prefetch.r no virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hprefetch_01);
bool test_hprefetch_01(void)
{
    TEST_BEGIN("HPREFETCH-01: VS-mode prefetch.r no virtual-inst");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* Disable all henvcfg CMO fields */
    henvcfg_set_cbie(CBIE_DISABLE);
    henvcfg_set_cbcfe(0);
    henvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_prefetch_r, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (prefetch.r)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HPREFETCH-02: VS-mode prefetch.w no virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hprefetch_02);
bool test_hprefetch_02(void)
{
    TEST_BEGIN("HPREFETCH-02: VS-mode prefetch.w no virtual-inst");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    henvcfg_set_cbie(CBIE_DISABLE);
    henvcfg_set_cbcfe(0);
    henvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_prefetch_w, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (prefetch.w)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HPREFETCH-03: VS-mode prefetch.i no virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hprefetch_03);
bool test_hprefetch_03(void)
{
    TEST_BEGIN("HPREFETCH-03: VS-mode prefetch.i no virtual-inst");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    henvcfg_set_cbie(CBIE_DISABLE);
    henvcfg_set_cbcfe(0);
    henvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_prefetch_i, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (prefetch.i)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HPREFETCH-04: VU-mode prefetch.r no virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hprefetch_04);
bool test_hprefetch_04(void)
{
    TEST_BEGIN("HPREFETCH-04: VU-mode prefetch.r no virtual-inst");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* Disable all CMO fields at all levels */
    henvcfg_set_cbie(CBIE_DISABLE);
    henvcfg_set_cbcfe(0);
    henvcfg_set_cbze(0);
    senvcfg_set_cbie(CBIE_DISABLE);
    senvcfg_set_cbcfe(0);
    senvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_prefetch_r, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (prefetch.r)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HPREFETCH-05: VU-mode prefetch.w no virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hprefetch_05);
bool test_hprefetch_05(void)
{
    TEST_BEGIN("HPREFETCH-05: VU-mode prefetch.w no virtual-inst");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    henvcfg_set_cbie(CBIE_DISABLE);
    henvcfg_set_cbcfe(0);
    henvcfg_set_cbze(0);
    senvcfg_set_cbie(CBIE_DISABLE);
    senvcfg_set_cbcfe(0);
    senvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_prefetch_w, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (prefetch.w)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HPREFETCH-06: VU-mode prefetch.i no virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_hprefetch_06);
bool test_hprefetch_06(void)
{
    TEST_BEGIN("HPREFETCH-06: VU-mode prefetch.i no virtual-inst");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    henvcfg_set_cbie(CBIE_DISABLE);
    henvcfg_set_cbcfe(0);
    henvcfg_set_cbze(0);
    senvcfg_set_cbie(CBIE_DISABLE);
    senvcfg_set_cbcfe(0);
    senvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_prefetch_i, addr);
    trap_expect_end();

    TEST_ASSERT("no trap (prefetch.i)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HPREFETCH-07: VS-mode prefetch G-stage no permission no exception
 * =================================================================== */
TEST_REGISTER(test_hprefetch_07);
bool test_hprefetch_07(void)
{
    TEST_BEGIN("HPREFETCH-07: VS-mode prefetch G-stage no-perm no exc");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    /* G-stage: victim page invalid */
    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim, 0 /* invalid */);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_prefetch_r, victim);
    trap_expect_end();

    /* Prefetch must NOT raise any exception */
    TEST_ASSERT("no trap (prefetch on invalid G-stage)",
                !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HPREFETCH-08: VS-mode prefetch does not check A/D bits
 * =================================================================== */
TEST_REGISTER(test_hprefetch_08);
bool test_hprefetch_08(void)
{
    TEST_BEGIN("HPREFETCH-08: VS-mode prefetch no A/D check");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    /* VS-stage: valid page with A=0, D=0 */
    ts2_setup_with_vs_victim(&ctx, SUITE_VSATP_MODE, SUITE_HGATP_MODE,
                             victim,
                             PTE_V | PTE_R | PTE_W | PTE_U);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_prefetch_w, victim);
    trap_expect_end();

    /* Prefetch should not trigger page-fault for A=0/D=0 */
    TEST_ASSERT("no trap (prefetch ignores A/D)", !trap_was_triggered());

    ts2_finish(&ctx);
    TEST_END();
}
