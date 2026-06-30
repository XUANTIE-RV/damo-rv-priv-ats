/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_rw.c - Group 2: stimecmp CSR Read/Write
 *
 * SSTC-RW-01 ~ SSTC-RW-06
 * Verifies that the stimecmp CSR supports full 64-bit read/write
 * from M-mode and S-mode (with proper access control enabled).
 */

/* ------------------------------------------------------------------ */
/* SSTC-RW-01: M-mode stimecmp all-ones read-write                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stimecmp_rw_all_ones);
bool test_sstc_stimecmp_rw_all_ones(void) {
    TEST_BEGIN("SSTC-RW-01: M-mode stimecmp all-ones read-write");

    menvcfg_set(MENVCFG_STCE);

    stimecmp_write((uintptr_t)-1);
    uintptr_t val = stimecmp_read();
    TEST_ASSERT_EQ("stimecmp all-ones", val, (uintptr_t)-1);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-RW-02: M-mode stimecmp all-zeros read-write                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stimecmp_rw_all_zeros);
bool test_sstc_stimecmp_rw_all_zeros(void) {
    TEST_BEGIN("SSTC-RW-02: M-mode stimecmp all-zeros read-write");

    menvcfg_set(MENVCFG_STCE);

    stimecmp_write(0);
    uintptr_t val = stimecmp_read();
    TEST_ASSERT_EQ("stimecmp all-zeros", val, 0);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-RW-03: M-mode stimecmp alternating bit patterns               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stimecmp_rw_alternating);
bool test_sstc_stimecmp_rw_alternating(void) {
    TEST_BEGIN("SSTC-RW-03: M-mode stimecmp alternating bit patterns");

    menvcfg_set(MENVCFG_STCE);

    uintptr_t pat1 = (uintptr_t)0x5555555555555555ULL;
    uintptr_t pat2 = (uintptr_t)0xAAAAAAAAAAAAAAAAULL;

    stimecmp_write(pat1);
    uintptr_t val = stimecmp_read();
    TEST_ASSERT_EQ("stimecmp 0x5555...", val, pat1);

    stimecmp_write(pat2);
    val = stimecmp_read();
    TEST_ASSERT_EQ("stimecmp 0xAAAA...", val, pat2);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-RW-04: M-mode stimecmp high 32-bit verification               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stimecmp_rw_high_bits);
bool test_sstc_stimecmp_rw_high_bits(void) {
    TEST_BEGIN("SSTC-RW-04: M-mode stimecmp high 32-bit verification");

    menvcfg_set(MENVCFG_STCE);

    uintptr_t pattern = (uintptr_t)0x1234567800000000ULL;
    stimecmp_write(pattern);
    uintptr_t val = stimecmp_read();
    TEST_ASSERT_EQ("stimecmp high bits", val, pattern);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-RW-05: S-mode stimecmp read-write                             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stimecmp_rw_smode);
bool test_sstc_stimecmp_rw_smode(void) {
    TEST_BEGIN("SSTC-RW-05: S-mode stimecmp read-write");

    /* Enable S-mode access */
    sstc_enable_smode_access();
    sstc_clear_timer();

    uintptr_t test_val = (uintptr_t)0xDEADBEEFCAFE0000ULL;

    /* Switch to S-mode and write stimecmp */
    goto_priv(PRIV_S);
    stimecmp_write(test_val);
    goto_priv(PRIV_M);

    /* Read back in M-mode */
    uintptr_t val = stimecmp_read();
    TEST_ASSERT_EQ("S-mode write visible in M-mode", val, test_val);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-RW-06: stimecmp write does not affect time                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stimecmp_no_affect_time);
bool test_sstc_stimecmp_no_affect_time(void) {
    TEST_BEGIN("SSTC-RW-06: stimecmp write does not affect time");

    menvcfg_set(MENVCFG_STCE);

    /* Read time before */
    uintptr_t t1 = time_read();

    /* Write stimecmp to various values */
    stimecmp_write(0);
    stimecmp_write((uintptr_t)-1);
    stimecmp_write(0x12345678);

    /* Read time after */
    uintptr_t t2 = time_read();

    /* time should have monotonically increased */
    TEST_ASSERT("time monotonically increases after stimecmp writes",
                t2 > t1);

    sstc_clear_timer();
    TEST_END();
}
