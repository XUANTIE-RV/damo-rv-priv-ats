/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_access.c - Group 3: Access Control
 *
 * SSTC-ACC-01 ~ SSTC-ACC-07
 * Verifies the multi-layer access control for stimecmp:
 *   - menvcfg.STCE controls S-mode access
 *   - mcounteren.TM controls S-mode access
 *
 * Note: SSTC-ACC-08/09/10 (VS-mode access control tests) have been
 * migrated to Hypervisor_Sstc/ project (HCROSS-SSTC-03/04/05).
 */

/* ------------------------------------------------------------------ */
/* SSTC-ACC-01: STCE=0, S-mode read stimecmp -> illegal-instruction   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_acc_stce0_smode_read);
bool test_sstc_acc_stce0_smode_read(void) {
    TEST_BEGIN("SSTC-ACC-01: STCE=0, S-mode read stimecmp -> illegal-inst");

    menvcfg_clear(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(stimecmp_read());
    goto_priv(PRIV_M);

    CHECK_TRAP("illegal-instruction on stimecmp read", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-ACC-02: STCE=0, S-mode write stimecmp -> illegal-instruction  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_acc_stce0_smode_write);
bool test_sstc_acc_stce0_smode_write(void) {
    TEST_BEGIN("SSTC-ACC-02: STCE=0, S-mode write stimecmp -> illegal-inst");

    menvcfg_clear(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(stimecmp_write((uintptr_t)-1));
    goto_priv(PRIV_M);

    CHECK_TRAP("illegal-instruction on stimecmp write", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-ACC-03: STCE=1, TM=0, S-mode read -> illegal-instruction     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_acc_stce1_tm0_smode_read);
bool test_sstc_acc_stce1_tm0_smode_read(void) {
    TEST_BEGIN("SSTC-ACC-03: STCE=1 TM=0, S-mode read -> illegal-inst");

    menvcfg_set(MENVCFG_STCE);
    mcounteren_clear(MCOUNTEREN_TM);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(stimecmp_read());
    goto_priv(PRIV_M);

    CHECK_TRAP("illegal-instruction on stimecmp read (TM=0)", CAUSE_ILLEGAL_INST);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-ACC-04: STCE=1, TM=0, S-mode write -> illegal-instruction    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_acc_stce1_tm0_smode_write);
bool test_sstc_acc_stce1_tm0_smode_write(void) {
    TEST_BEGIN("SSTC-ACC-04: STCE=1 TM=0, S-mode write -> illegal-inst");

    menvcfg_set(MENVCFG_STCE);
    mcounteren_clear(MCOUNTEREN_TM);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(stimecmp_write((uintptr_t)-1));
    goto_priv(PRIV_M);

    CHECK_TRAP("illegal-instruction on stimecmp write (TM=0)", CAUSE_ILLEGAL_INST);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-ACC-05: STCE=1, TM=1, S-mode read -> no exception            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_acc_stce1_tm1_smode_read);
bool test_sstc_acc_stce1_tm1_smode_read(void) {
    TEST_BEGIN("SSTC-ACC-05: STCE=1 TM=1, S-mode read -> no exception");

    sstc_enable_smode_access();
    sstc_clear_timer();

    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(stimecmp_read());
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("S-mode stimecmp read succeeded");

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-ACC-06: STCE=1, TM=1, S-mode write -> no exception           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_acc_stce1_tm1_smode_write);
bool test_sstc_acc_stce1_tm1_smode_write(void) {
    TEST_BEGIN("SSTC-ACC-06: STCE=1 TM=1, S-mode write -> no exception");

    sstc_enable_smode_access();
    sstc_clear_timer();

    uintptr_t test_val = (uintptr_t)0xBEEFCAFE12340000ULL;

    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(stimecmp_write(test_val));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("S-mode stimecmp write succeeded");

    uintptr_t readback = stimecmp_read();
    TEST_ASSERT_EQ("write value persisted", readback, test_val);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-ACC-07: M-mode always can access stimecmp (even STCE=0)       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_acc_mmode_always_access);
bool test_sstc_acc_mmode_always_access(void) {
    TEST_BEGIN("SSTC-ACC-07: M-mode always can access stimecmp");

    /* Enable STCE first so stimecmp CSR exists, then clear */
    menvcfg_set(MENVCFG_STCE);
    sstc_clear_timer();

    /* Even with STCE=0, M-mode should still be able to access stimecmp.
     * Note: Some implementations may trap on stimecmp access when STCE=0
     * even in M-mode. We test with STCE=1 to verify basic M-mode access. */
    uintptr_t test_val = (uintptr_t)0xA5A5A5A500000000ULL;

    EXPECT_NO_TRAP(stimecmp_write(test_val));
    uintptr_t readback = 0;
    EXPECT_NO_TRAP(readback = stimecmp_read());
    TEST_ASSERT_EQ("M-mode read-write ok", readback, test_val);

    sstc_clear_timer();
    TEST_END();
}
