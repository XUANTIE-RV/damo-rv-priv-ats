/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_instret_filter.c - Group 3: Instret Counter Privilege Mode Filtering
 *
 * Tests PMF-INS-01 through PMF-INS-14
 * Validates minstretcfg xINH bits inhibit/allow instret counting per mode,
 * and exception/xRET instruction counting behavior.
 */

/* ------------------------------------------------------------------ */
/* PMF-INS-01: All xINH=0, all modes count instret                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_01_all_zero);
bool test_pmf_ins_01_all_zero(void)
{
    TEST_BEGIN("PMF-INS-01: All xINH=0, all modes count instret");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* Clear all inhibit bits */
    minstretcfg_write(0);

    /* M-mode should count */
    uint64_t start = read_minstret();
    execute_nops(100);
    uint64_t end = read_minstret();

    TEST_ASSERT("M-mode instret increments", end > start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-02: MINH=1 inhibits M-mode instret counting               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_02_minh_inhibit);
bool test_pmf_ins_02_minh_inhibit(void)
{
    TEST_BEGIN("PMF-INS-02: MINH=1 inhibits M-mode instret");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* Set MINH=1 */
    minstretcfg_write(CYCLECFG_MINH);

    uint64_t start = read_minstret();
    execute_nops(200);
    uint64_t end = read_minstret();

    TEST_ASSERT("M-mode instret does not increment with MINH=1", end == start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-03: MINH=1 does not affect S-mode instret                 */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_03_minh_no_smode_effect);
bool test_pmf_ins_03_minh_no_smode_effect(void)
{
    TEST_BEGIN("PMF-INS-03: MINH=1 does not affect S-mode instret");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* Set MINH=1, SINH=0 */
    minstretcfg_write(CYCLECFG_MINH);

    goto_priv(PRIV_S);
    uint64_t start = read_instret();
    execute_nops(200);
    uint64_t end = read_instret();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode instret increments with MINH=1", end > start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-04: SINH=1 inhibits S-mode instret counting               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_04_sinh_inhibit);
bool test_pmf_ins_04_sinh_inhibit(void)
{
    TEST_BEGIN("PMF-INS-04: SINH=1 inhibits S-mode instret");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* Set SINH=1, MINH=1 */
    minstretcfg_write(CYCLECFG_SINH | CYCLECFG_MINH);

    goto_priv(PRIV_S);
    uint64_t start = read_instret();
    execute_nops(200);
    uint64_t end = read_instret();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode instret does not increment with SINH=1", end == start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-05: UINH=1 inhibits U-mode instret counting               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_05_uinh_inhibit);
bool test_pmf_ins_05_uinh_inhibit(void)
{
    TEST_BEGIN("PMF-INS-05: UINH=1 inhibits U-mode instret");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_umode())
        TEST_SKIP("U-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* Set UINH=1, MINH=1 */
    minstretcfg_write(CYCLECFG_UINH | CYCLECFG_MINH);

    /* Read counter in M-mode before U-mode execution */
    uint64_t start = read_minstret();

    /* Execute in U-mode (should not count with UINH=1) */
    goto_priv(PRIV_U);
    execute_nops(200);
    goto_priv(PRIV_M);

    /* Read counter in M-mode after */
    uint64_t end = read_minstret();

    TEST_ASSERT("U-mode instret does not increment with UINH=1", end == start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-08: Exception instruction does not increment instret      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_08_exception_no_retire);
bool test_pmf_ins_08_exception_no_retire(void)
{
    TEST_BEGIN("PMF-INS-08: Exception instruction does not increment instret");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /*
     * Per norm:instret_exception, an instruction that causes a synchronous
     * exception does NOT retire and hence does NOT increment instret.
     *
     * To isolate the ecall itself, inhibit M-mode (MINH=1) so that the
     * M-mode trap handler instructions and the MRET (originating from
     * inhibited M-mode) do not contribute to instret. Then the ecall is
     * the only relevant instruction: it traps (does not retire) and the
     * handler is silenced, so instret must remain stable across the trap.
     */
    minstretcfg_write(CYCLECFG_MINH);

    uint64_t start = read_minstret();

    /* Execute an ecall (causes exception, should NOT retire) */
    trap_expect_begin();
    asm volatile("ecall" ::: "memory");
    trap_expect_end();

    uint64_t after_ecall = read_minstret();

    /* With M-mode inhibited: ecall does not retire, M-mode handler and
     * MRET are silenced. instret must not change. */
    TEST_ASSERT("ecall did not increment instret (no retire)",
                after_ecall == start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-09: Exception in inhibited mode does not increment        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_09_exception_inhibited);
bool test_pmf_ins_09_exception_inhibited(void)
{
    TEST_BEGIN("PMF-INS-09: Exception in inhibited mode no increment");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* MINH=1 (M-mode inhibited) */
    minstretcfg_write(CYCLECFG_MINH);

    uint64_t start = read_minstret();

    trap_expect_begin();
    asm volatile("ecall" ::: "memory");
    trap_expect_end();

    execute_nops(50);
    uint64_t end = read_minstret();

    /* With MINH=1, nothing in M-mode should count */
    TEST_ASSERT("instret does not increment in inhibited M-mode", end == start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-10: xRET from non-inhibited mode increments instret       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_10_xret_non_inhibited);
bool test_pmf_ins_10_xret_non_inhibited(void)
{
    TEST_BEGIN("PMF-INS-10: xRET from non-inhibited mode increments");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* All modes non-inhibited */
    minstretcfg_write(0);

    /* Go to S-mode and execute SRET back to U-mode */
    /* First set up: go to S-mode, then SRET to U-mode */
    goto_priv(PRIV_S);

    uint64_t start = read_instret();

    /* Execute some nops and return */
    execute_nops(10);
    goto_priv(PRIV_M);

    uint64_t end = read_minstret();

    /* instret should have incremented (S-mode not inhibited) */
    TEST_ASSERT("instret increments in non-inhibited S-mode", end > start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-11: xRET from inhibited mode does not increment instret   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_11_xret_inhibited);
bool test_pmf_ins_11_xret_inhibited(void)
{
    TEST_BEGIN("PMF-INS-11: xRET from inhibited M-mode no increment");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* MINH=1 (M-mode inhibited) */
    minstretcfg_write(CYCLECFG_MINH);

    uint64_t start = read_minstret();

    /* Execute nops in M-mode (inhibited) */
    execute_nops(50);

    uint64_t end = read_minstret();

    /* Nothing should count in inhibited M-mode */
    TEST_ASSERT("instret does not increment in inhibited M-mode", end == start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-12: xRET from inhibited S-mode does not increment         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_12_xret_smode_inhibited);
bool test_pmf_ins_12_xret_smode_inhibited(void)
{
    TEST_BEGIN("PMF-INS-12: xRET from inhibited S-mode no increment");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* SINH=1, MINH=1 (both inhibited) */
    minstretcfg_write(CYCLECFG_SINH | CYCLECFG_MINH);

    goto_priv(PRIV_S);
    uint64_t start = read_instret();
    execute_nops(50);
    uint64_t end = read_instret();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode instret inhibited", end == start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-13: xRET from non-inhibited S-mode increments             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_13_xret_smode_non_inhibited);
bool test_pmf_ins_13_xret_smode_non_inhibited(void)
{
    TEST_BEGIN("PMF-INS-13: xRET from non-inhibited S-mode increments");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_smode())
        TEST_SKIP("S-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* SINH=0 (S-mode not inhibited), MINH=1 (M inhibited) */
    minstretcfg_write(CYCLECFG_MINH);

    goto_priv(PRIV_S);
    uint64_t start = read_instret();
    execute_nops(100);
    uint64_t end = read_instret();
    goto_priv(PRIV_M);

    TEST_ASSERT("S-mode instret increments when not inhibited", end > start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-14: Only U-mode non-inhibited scenario                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_14_umode_only);
bool test_pmf_ins_14_umode_only(void)
{
    TEST_BEGIN("PMF-INS-14: Only U-mode non-inhibited scenario");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!has_umode())
        TEST_SKIP("U-mode not supported");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* Inhibit all except U-mode: MINH=1, SINH=1, UINH=0 */
    minstretcfg_write(CYCLECFG_MINH | CYCLECFG_SINH);

    /* M-mode should not count */
    uint64_t m_start = read_minstret();
    execute_nops(100);
    uint64_t m_end = read_minstret();

    TEST_ASSERT("M-mode instret does not increment", m_end == m_start);

    /* Read counter before U-mode execution */
    uint64_t start = read_minstret();

    /* Execute in U-mode (should count) */
    goto_priv(PRIV_U);
    execute_nops(200);
    goto_priv(PRIV_M);

    /* Read counter after */
    uint64_t end = read_minstret();

    /* U-mode instructions should count */
    TEST_ASSERT("U-mode instret increments", end > start);

    minstretcfg_write(0);
    TEST_END();
}
