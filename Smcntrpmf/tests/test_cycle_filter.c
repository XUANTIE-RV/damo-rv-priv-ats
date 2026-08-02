/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_cycle_filter.c - Group 2: Cycle Counter Privilege Mode Filtering
 *
 * Tests PMF-CYC-01 through PMF-CYC-11
 * Validates mcyclecfg xINH bits inhibit/allow cycle counting per mode.
 */

/* ------------------------------------------------------------------ */
/* PMF-CYC-01: All xINH=0, all modes count                           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_01_all_zero);
bool test_pmf_cyc_01_all_zero(void)
{
    TEST_BEGIN("PMF-CYC-01: All xINH=0, all modes count");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Clear all inhibit bits */
    mcyclecfg_write(0);

    /* M-mode should count */
    uint64_t start = read_mcycle();
    execute_nops(100);
    uint64_t end = read_mcycle();

    TEST_ASSERT("M-mode cycle increments", end > start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CYC-02: MINH=1 inhibits M-mode cycle counting                 */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_02_minh_inhibit);
bool test_pmf_cyc_02_minh_inhibit(void)
{
    TEST_BEGIN("PMF-CYC-02: MINH=1 inhibits M-mode cycle counting");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Set MINH=1 */
    mcyclecfg_write(CYCLECFG_MINH);

    uint64_t start = read_mcycle();
    execute_nops(200);
    uint64_t end = read_mcycle();

    /* With MINH=1, cycle should not increment in M-mode */
    TEST_ASSERT("M-mode cycle does not increment with MINH=1", end == start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CYC-03: MINH=1 does not affect S-mode cycle counting          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_03_minh_no_smode_effect);
bool test_pmf_cyc_03_minh_no_smode_effect(void)
{
    TEST_BEGIN("PMF-CYC-03: MINH=1 does not affect S-mode");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Set MINH=1, SINH=0 (S-mode should count) */
    mcyclecfg_write(CYCLECFG_MINH);

    goto_priv(PRIV_S);
    uint64_t start = read_cycle();
    execute_nops(200);
    uint64_t end = read_cycle();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode cycle increments with MINH=1", end > start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CYC-04: SINH=1 inhibits S-mode cycle counting                 */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_04_sinh_inhibit);
bool test_pmf_cyc_04_sinh_inhibit(void)
{
    TEST_BEGIN("PMF-CYC-04: SINH=1 inhibits S-mode cycle counting");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Set SINH=1, MINH=1 (inhibit both M and S to isolate) */
    mcyclecfg_write(CYCLECFG_SINH | CYCLECFG_MINH);

    goto_priv(PRIV_S);
    uint64_t start = read_cycle();
    execute_nops(200);
    uint64_t end = read_cycle();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode cycle does not increment with SINH=1", end == start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CYC-05: SINH=1 does not affect M-mode cycle counting          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_05_sinh_no_mmode_effect);
bool test_pmf_cyc_05_sinh_no_mmode_effect(void)
{
    TEST_BEGIN("PMF-CYC-05: SINH=1 does not affect M-mode");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Set SINH=1, MINH=0 (M-mode should count) */
    mcyclecfg_write(CYCLECFG_SINH);

    uint64_t start = read_mcycle();
    execute_nops(200);
    uint64_t end = read_mcycle();

    TEST_ASSERT("M-mode cycle increments with SINH=1", end > start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CYC-06: UINH=1 inhibits U-mode cycle counting                 */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_06_uinh_inhibit);
bool test_pmf_cyc_06_uinh_inhibit(void)
{
    TEST_BEGIN("PMF-CYC-06: UINH=1 inhibits U-mode cycle counting");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_umode())
        TEST_SKIP("U-mode not supported");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Set UINH=1, MINH=1 (inhibit U and M, so only U-mode matters) */
    mcyclecfg_write(CYCLECFG_UINH | CYCLECFG_MINH);

    /* Read counter in M-mode before U-mode execution */
    uint64_t start = read_mcycle();

    /* Execute in U-mode (should not count with UINH=1) */
    goto_priv(PRIV_U);
    execute_nops(200);
    goto_priv(PRIV_M);

    /* Read counter in M-mode after */
    uint64_t end = read_mcycle();

    /* With UINH=1 and MINH=1, counter should not increment */
    TEST_ASSERT("U-mode cycle does not increment with UINH=1", end == start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CYC-07: UINH=1 does not affect M-mode cycle counting          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_07_uinh_no_mmode_effect);
bool test_pmf_cyc_07_uinh_no_mmode_effect(void)
{
    TEST_BEGIN("PMF-CYC-07: UINH=1 does not affect M-mode");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Set UINH=1, MINH=0 */
    mcyclecfg_write(CYCLECFG_UINH);

    uint64_t start = read_mcycle();
    execute_nops(200);
    uint64_t end = read_mcycle();

    TEST_ASSERT("M-mode cycle increments with UINH=1", end > start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CYC-10: Multiple modes inhibited simultaneously               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_10_multi_inhibit);
bool test_pmf_cyc_10_multi_inhibit(void)
{
    TEST_BEGIN("PMF-CYC-10: Multiple modes inhibited simultaneously");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Set MINH=1, SINH=1 (both M and S inhibited) */
    mcyclecfg_write(CYCLECFG_MINH | CYCLECFG_SINH);

    /* M-mode should not count */
    uint64_t m_start = read_mcycle();
    execute_nops(100);
    uint64_t m_end = read_mcycle();

    TEST_ASSERT("M-mode cycle inhibited", m_end == m_start);

    /* S-mode should not count */
    goto_priv(PRIV_S);
    uint64_t s_start = read_cycle();
    execute_nops(100);
    uint64_t s_end = read_cycle();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode cycle inhibited", s_end == s_start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CYC-11: Mode transition cycle counting behavior               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_11_transition);
bool test_pmf_cyc_11_transition(void)
{
    TEST_BEGIN("PMF-CYC-11: Mode transition cycle counting");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_umode())
        TEST_SKIP("U-mode not supported");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* MINH=1 (M inhibited), UINH=0 (U not inhibited) */
    mcyclecfg_write(CYCLECFG_MINH);

    /* Read counter before U-mode execution */
    uint64_t start = read_mcycle();

    /* Execute in U-mode (should count) */
    goto_priv(PRIV_U);
    execute_nops(200);
    goto_priv(PRIV_M);

    /* Read counter after */
    uint64_t end = read_mcycle();

    /* U-mode portion should have counted */
    TEST_ASSERT("cycle incremented from U-mode execution", end > start);

    mcyclecfg_write(0);
    TEST_END();
}
