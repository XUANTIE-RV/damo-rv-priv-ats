/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_counteren.c - Group 7: Interaction with mcounteren/scounteren
 *
 * Tests PMF-CTR-01 through PMF-CTR-05
 * Validates counter access control vs privilege mode filtering orthogonality.
 */

/* ------------------------------------------------------------------ */
/* PMF-CTR-01: mcounteren.CY=0, S-mode cannot read cycle             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ctr_01_mcounteren_cy0);
bool test_pmf_ctr_01_mcounteren_cy0(void)
{
    TEST_BEGIN("PMF-CTR-01: mcounteren.CY=0, S-mode cannot read cycle");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");

    /* Save and clear mcounteren.CY */
    uintptr_t orig = CSRR(mcounteren);
    CSRW(mcounteren, orig & ~1UL);

    /* mcyclecfg all zeros (no filtering) */
    mcyclecfg_write(0);

    /* S-mode read cycle should trap */
    goto_priv(PRIV_S);
    trap_expect_begin();
    asm volatile("csrr x0, 0xC00" ::: "memory");  /* cycle */
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();
    goto_priv(PRIV_M);

    TEST_ASSERT("trap triggered", trapped);
    TEST_ASSERT_EQ("cause is illegal instruction", cause, CAUSE_ILLEGAL_INST);

    /* Restore */
    CSRW(mcounteren, orig);
    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CTR-02: mcounteren.CY=1, S-mode reads cycle (inhibited)       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ctr_02_mcounteren_cy1_inhibited);
bool test_pmf_ctr_02_mcounteren_cy1_inhibited(void)
{
    TEST_BEGIN("PMF-CTR-02: mcounteren.CY=1, S-mode reads inhibited cycle");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Ensure mcounteren.CY=1 */
    uintptr_t orig = CSRR(mcounteren);
    CSRW(mcounteren, orig | 1UL);

    /* Set SINH=1, MINH=1 (S and M inhibited) */
    mcyclecfg_write(CYCLECFG_SINH | CYCLECFG_MINH);

    /* Read counter before S-mode execution */
    uint64_t start = read_mcycle();

    /* S-mode execution (should not count with SINH=1) */
    goto_priv(PRIV_S);
    execute_nops(200);
    goto_priv(PRIV_M);

    /* Read counter after */
    uint64_t end = read_mcycle();

    TEST_ASSERT("S-mode cycle not incrementing with SINH=1", end == start);

    /* Restore */
    CSRW(mcounteren, orig);
    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CTR-03: scounteren.CY=0, U-mode cannot read cycle             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ctr_03_scounteren_cy0);
bool test_pmf_ctr_03_scounteren_cy0(void)
{
    TEST_BEGIN("PMF-CTR-03: scounteren.CY=0, U-mode cannot read cycle");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode() || !has_umode())
        TEST_SKIP("S-mode and U-mode required");

    /* Ensure mcounteren.CY=1, scounteren.CY=0 */
    uintptr_t mc_orig = CSRR(mcounteren);
    CSRW(mcounteren, mc_orig | 1UL);

    trap_expect_begin();
    uintptr_t sc_orig = CSRR(scounteren);
    if (trap_was_triggered()) {
        trap_expect_end();
        CSRW(mcounteren, mc_orig);
        TEST_SKIP("scounteren not accessible");
    }
    trap_expect_end();

    CSRW(scounteren, sc_orig & ~1UL);

    /* U-mode read cycle should trap */
    goto_priv(PRIV_U);
    trap_expect_begin();
    asm volatile("csrr x0, 0xC00" ::: "memory");
    bool trapped = trap_was_triggered();
    trap_expect_end();
    goto_priv(PRIV_M);

    TEST_ASSERT("U-mode trap on cycle read", trapped);

    /* Restore */
    CSRW(scounteren, sc_orig);
    CSRW(mcounteren, mc_orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CTR-05: Filtering and access control are orthogonal           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ctr_05_orthogonal);
bool test_pmf_ctr_05_orthogonal(void)
{
    TEST_BEGIN("PMF-CTR-05: Filtering and access control orthogonal");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_umode())
        TEST_SKIP("U-mode not supported");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Ensure mcounteren.CY=1, scounteren.CY=1 */
    uintptr_t mc_orig = CSRR(mcounteren);
    CSRW(mcounteren, mc_orig | 1UL);

    trap_expect_begin();
    uintptr_t sc_orig = CSRR(scounteren);
    if (!trap_was_triggered()) {
        CSRW(scounteren, sc_orig | 1UL);
    }
    trap_expect_end();

    /* Set UINH=1, MINH=1 (U and M inhibited) */
    mcyclecfg_write(CYCLECFG_UINH | CYCLECFG_MINH);

    /* Read counter before U-mode execution */
    uint64_t start = read_mcycle();

    /* U-mode execution (should not count with UINH=1) */
    goto_priv(PRIV_U);
    execute_nops(200);
    goto_priv(PRIV_M);

    /* Read counter after */
    uint64_t end = read_mcycle();

    /* U-mode can access cycle (mcounteren.CY=1), but filtering inhibits it */
    TEST_ASSERT("U-mode cycle inhibited by mcyclecfg", end == start);

    /* Restore */
    CSRW(mcounteren, mc_orig);
    mcyclecfg_write(0);
    TEST_END();
}
