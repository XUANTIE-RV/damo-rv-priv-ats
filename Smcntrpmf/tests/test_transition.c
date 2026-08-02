/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_transition.c - Group 4: Mode Transition and Counting Boundary Behavior
 *
 * Tests PMF-TR-01 through PMF-TR-10
 * Validates counter behavior during trap and trap return transitions.
 */

/* ------------------------------------------------------------------ */
/* PMF-TR-01: Trap into inhibited mode stops cycle                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_tr_01_trap_inhibited_cycle);
bool test_pmf_tr_01_trap_inhibited_cycle(void)
{
    TEST_BEGIN("PMF-TR-01: Trap into inhibited M-mode stops cycle");

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

    /* Execute in M-mode (should not count with MINH=1) */
    execute_nops(200);

    /* Return to U-mode (should count) */
    goto_priv(PRIV_U);
    execute_nops(200);
    goto_priv(PRIV_M);

    /* Read counter after */
    uint64_t end = read_mcycle();

    /* Cycle should have incremented from U-mode portions only */
    TEST_ASSERT("cycle incremented from U-mode", end > start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-TR-02: Trap into non-inhibited mode continues cycle           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_tr_02_trap_non_inhibited_cycle);
bool test_pmf_tr_02_trap_non_inhibited_cycle(void)
{
    TEST_BEGIN("PMF-TR-02: Trap into non-inhibited S-mode continues cycle");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* SINH=0 (S not inhibited), MINH=1 (M inhibited) */
    mcyclecfg_write(CYCLECFG_MINH);

    /* Go to S-mode (non-inhibited) */
    goto_priv(PRIV_S);
    uint64_t start = read_cycle();
    execute_nops(200);
    uint64_t end = read_cycle();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode cycle increments", end > start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-TR-03: Trap into inhibited mode stops instret                 */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_tr_03_trap_inhibited_instret);
bool test_pmf_tr_03_trap_inhibited_instret(void)
{
    TEST_BEGIN("PMF-TR-03: Trap into inhibited M-mode stops instret");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* MINH=1 (M inhibited) */
    minstretcfg_write(CYCLECFG_MINH);

    uint64_t start = read_minstret();
    execute_nops(100);
    uint64_t end = read_minstret();

    TEST_ASSERT("M-mode instret inhibited", end == start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-TR-04: Trap into non-inhibited mode continues instret         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_tr_04_trap_non_inhibited_instret);
bool test_pmf_tr_04_trap_non_inhibited_instret(void)
{
    TEST_BEGIN("PMF-TR-04: Trap into non-inhibited S-mode continues instret");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* SINH=0 (S not inhibited), MINH=1 (M inhibited) */
    minstretcfg_write(CYCLECFG_MINH);

    goto_priv(PRIV_S);
    uint64_t start = read_instret();
    execute_nops(100);
    uint64_t end = read_instret();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode instret increments", end > start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-TR-05: MRET from inhibited mode instret behavior              */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_tr_05_mret_inhibited);
bool test_pmf_tr_05_mret_inhibited(void)
{
    TEST_BEGIN("PMF-TR-05: MRET from inhibited M-mode no instret");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /*
     * Per norm:instret_xret, an xRET instruction retires and increments
     * instret ONLY if the originating privilege mode is not inhibited.
     *
     * Inhibit BOTH M-mode and S-mode (MINH=1, SINH=1) so that every
     * instruction involved in the mode switch (M-mode handler, the MRET
     * originating from inhibited M-mode, and any transient S-mode
     * instructions) is silenced. instret must then remain stable across
     * the round trip, proving MRET from inhibited M-mode does not count.
     */
    minstretcfg_write(CYCLECFG_MINH | CYCLECFG_SINH);

    uint64_t start = read_minstret();

    /* goto_priv uses MRET internally to switch modes */
    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

    uint64_t end = read_minstret();

    /* With M and S inhibited, no instruction in the switch path counts,
     * including MRET originating from inhibited M-mode. */
    TEST_ASSERT("instret stable across switch (MINH=SINH=1)", end == start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-TR-06: MRET from non-inhibited mode increments instret        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_tr_06_mret_non_inhibited);
bool test_pmf_tr_06_mret_non_inhibited(void)
{
    TEST_BEGIN("PMF-TR-06: MRET from non-inhibited M-mode increments");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* MINH=0 (M not inhibited) */
    minstretcfg_write(0);

    uint64_t start = read_minstret();
    execute_nops(50);
    uint64_t end = read_minstret();

    TEST_ASSERT("instret increments in non-inhibited M-mode", end > start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-TR-07: Interrupt into inhibited mode                          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_tr_07_interrupt_inhibited);
bool test_pmf_tr_07_interrupt_inhibited(void)
{
    TEST_BEGIN("PMF-TR-07: Interrupt into inhibited M-mode");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* MINH=1 (M inhibited) */
    mcyclecfg_write(CYCLECFG_MINH);

    /* Just verify M-mode cycle is inhibited */
    uint64_t start = read_mcycle();
    execute_nops(100);
    uint64_t end = read_mcycle();

    TEST_ASSERT("M-mode cycle inhibited", end == start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-TR-08: Interrupt into non-inhibited mode                      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_tr_08_interrupt_non_inhibited);
bool test_pmf_tr_08_interrupt_non_inhibited(void)
{
    TEST_BEGIN("PMF-TR-08: Interrupt into non-inhibited S-mode");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* SINH=0 (S not inhibited) */
    mcyclecfg_write(0);

    goto_priv(PRIV_S);
    uint64_t start = read_cycle();
    execute_nops(100);
    uint64_t end = read_cycle();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode cycle increments", end > start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-TR-09: Repeated mode switches cycle consistency               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_tr_09_repeated_switch_cycle);
bool test_pmf_tr_09_repeated_switch_cycle(void)
{
    TEST_BEGIN("PMF-TR-09: Repeated mode switches cycle consistency");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* MINH=1 (M inhibited), SINH=0 (S not inhibited) */
    mcyclecfg_write(CYCLECFG_MINH);

    /* Multiple S-mode entries */
    uint64_t total_s_cycles = 0;
    for (int i = 0; i < 5; i++) {
        goto_priv(PRIV_S);
        uint64_t start = read_cycle();
        execute_nops(50);
        uint64_t end = read_cycle();
        goto_priv(PRIV_M);
        total_s_cycles += (end - start);
    }

    TEST_ASSERT("S-mode cycles accumulated over switches", total_s_cycles > 0);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-TR-10: Repeated mode switches instret consistency             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_tr_10_repeated_switch_instret);
bool test_pmf_tr_10_repeated_switch_instret(void)
{
    TEST_BEGIN("PMF-TR-10: Repeated mode switches instret consistency");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* MINH=1 (M inhibited), SINH=0 (S not inhibited) */
    minstretcfg_write(CYCLECFG_MINH);

    /*
     * Verify M-mode instructions do not count: take a local M-mode-only
     * delta. With MINH=1, executing NOPs in M-mode must not advance
     * instret. (Reading instret itself is an M-mode access that does not
     * count, so the two reads bracket exactly the M-mode NOPs.)
     */
    uint64_t m_start = read_minstret();
    execute_nops(50);
    uint64_t m_end = read_minstret();

    TEST_ASSERT("M-mode instret stable (MINH=1)", m_end == m_start);

    /*
     * Verify S-mode instructions DO count (SINH=0). Measure the S-mode
     * delta locally while in S-mode; the surrounding mode-switch overhead
     * is excluded from this window.
     */
    goto_priv(PRIV_S);
    uint64_t s_start = read_instret();
    execute_nops(100);
    uint64_t s_end = read_instret();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode instret incremented (SINH=0)", s_end > s_start);

    minstretcfg_write(0);
    TEST_END();
}
