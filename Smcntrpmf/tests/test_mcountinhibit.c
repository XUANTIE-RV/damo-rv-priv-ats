/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mcountinhibit.c - Group 6: Interaction with mcountinhibit
 *
 * Tests PMF-INH-01 through PMF-INH-04
 * Validates mcountinhibit global disable vs mcyclecfg/minstretcfg mode filtering.
 */

/* ------------------------------------------------------------------ */
/* PMF-INH-01: mcountinhibit global disable takes priority           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_inh_01_global_disable);
bool test_pmf_inh_01_global_disable(void)
{
    TEST_BEGIN("PMF-INH-01: mcountinhibit.CY=1 global disable");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    /* Check if mcountinhibit is implemented */
    trap_expect_begin();
    uintptr_t orig_inhibit = mcountinhibit_read();
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("mcountinhibit not implemented");
    }
    trap_expect_end();

    /* Set mcountinhibit.CY=1 (global cycle disable) */
    mcountinhibit_write(orig_inhibit | MCOUNTINHIBIT_CY);

    /* mcyclecfg all zeros (no mode filtering) */
    mcyclecfg_write(0);

    uint64_t start = read_mcycle();
    execute_nops(200);
    uint64_t end = read_mcycle();

    TEST_ASSERT("cycle globally disabled by mcountinhibit", end == start);

    /* Restore */
    mcountinhibit_write(orig_inhibit);
    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INH-02: mcountinhibit.CY=0, mcyclecfg inhibits M-mode         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_inh_02_mode_filter);
bool test_pmf_inh_02_mode_filter(void)
{
    TEST_BEGIN("PMF-INH-02: mcountinhibit.CY=0, mcyclecfg.MINH=1");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Check if mcountinhibit is implemented */
    trap_expect_begin();
    uintptr_t orig_inhibit = mcountinhibit_read();
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("mcountinhibit not implemented");
    }
    trap_expect_end();

    /* Clear mcountinhibit.CY (cycle enabled globally) */
    mcountinhibit_write(orig_inhibit & ~MCOUNTINHIBIT_CY);

    /* Set MINH=1 (M-mode inhibited by mcyclecfg) */
    mcyclecfg_write(CYCLECFG_MINH);

    uint64_t start = read_mcycle();
    execute_nops(200);
    uint64_t end = read_mcycle();

    TEST_ASSERT("M-mode cycle inhibited by mcyclecfg", end == start);

    /* Restore */
    mcountinhibit_write(orig_inhibit);
    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INH-03: mcountinhibit.IR=1 global instret disable             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_inh_03_instret_global_disable);
bool test_pmf_inh_03_instret_global_disable(void)
{
    TEST_BEGIN("PMF-INH-03: mcountinhibit.IR=1 global instret disable");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    /* Check if mcountinhibit is implemented */
    trap_expect_begin();
    uintptr_t orig_inhibit = mcountinhibit_read();
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("mcountinhibit not implemented");
    }
    trap_expect_end();

    /* Set mcountinhibit.IR=1 (global instret disable) */
    mcountinhibit_write(orig_inhibit | MCOUNTINHIBIT_IR);

    /* minstretcfg all zeros */
    minstretcfg_write(0);

    uint64_t start = read_minstret();
    execute_nops(200);
    uint64_t end = read_minstret();

    TEST_ASSERT("instret globally disabled by mcountinhibit", end == start);

    /* Restore */
    mcountinhibit_write(orig_inhibit);
    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INH-04: mcountinhibit.IR=0, minstretcfg inhibits S-mode       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_inh_04_instret_mode_filter);
bool test_pmf_inh_04_instret_mode_filter(void)
{
    TEST_BEGIN("PMF-INH-04: mcountinhibit.IR=0, minstretcfg.SINH=1");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* Check if mcountinhibit is implemented */
    trap_expect_begin();
    uintptr_t orig_inhibit = mcountinhibit_read();
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("mcountinhibit not implemented");
    }
    trap_expect_end();

    /* Clear mcountinhibit.IR */
    mcountinhibit_write(orig_inhibit & ~MCOUNTINHIBIT_IR);

    /* Set SINH=1, MINH=1 (S and M inhibited) */
    minstretcfg_write(CYCLECFG_SINH | CYCLECFG_MINH);

    goto_priv(PRIV_S);
    uint64_t start = read_instret();
    execute_nops(200);
    uint64_t end = read_instret();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode instret inhibited by minstretcfg", end == start);

    /* Restore */
    mcountinhibit_write(orig_inhibit);
    minstretcfg_write(0);
    TEST_END();
}
