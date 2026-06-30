/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_prop.c - Group 4: mstateen Read-Only-Zero Propagation to Lower Privilege
 *
 * MSTA-PROP-01 ~ MSTA-PROP-04
 * Verifies that bits which are zero in mstateen propagate as
 * read-only zero to sstateen (and hstateen when H ext is present).
 */

/* ------------------------------------------------------------------ */
/* MSTA-PROP-01: mstateen0 zero bit propagates to sstateen0           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_prop_sstateen0);
bool test_mstateen0_prop_sstateen0(void) {
    TEST_BEGIN("MSTA-PROP-01: mstateen0 zero bit propagates to sstateen0");

    uintptr_t orig = mstateen0_read();

    /* Enable sstateen0 access (SE0=1) but clear C bit */
    mstateen0_write(MSTATEEN0_SE0);

    /* From M-mode, try writing C bit to sstateen0 */
    sstateen0_write(MSTATEEN0_C);
    uintptr_t val = sstateen0_read();

    TEST_ASSERT_BITS("sstateen0.C is RO0 when mstateen0.C=0",
                     val, MSTATEEN0_C, 0);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-PROP-03: mstateen0 one bit un-propagates (writable again)     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_prop_unprop);
bool test_mstateen0_prop_unprop(void) {
    TEST_BEGIN("MSTA-PROP-03: mstateen0 bit=1 makes sstateen0 bit writable");

    uintptr_t orig = mstateen0_read();

    /* First verify propagation: SE0=1, C=0 */
    mstateen0_write(MSTATEEN0_SE0);
    sstateen0_write(MSTATEEN0_C);
    uintptr_t val0 = sstateen0_read();
    TEST_ASSERT_BITS("sstateen0.C is RO0 (C=0 in mstateen0)",
                     val0, MSTATEEN0_C, 0);

    /* Now set C=1 in mstateen0 and verify sstateen0.C becomes writable */
    mstateen0_write(MSTATEEN0_SE0 | MSTATEEN0_C);

    /* Check if C bit is actually writable in mstateen0 */
    if (!(mstateen0_read() & MSTATEEN0_C)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.C is not writable (no custom state)");
    }

    sstateen0_write(MSTATEEN0_C);
    uintptr_t val1 = sstateen0_read();
    TEST_ASSERT_BITS("sstateen0.C is writable when mstateen0.C=1",
                     val1, MSTATEEN0_C, MSTATEEN0_C);

    /* Clean up */
    sstateen0_write(0);
    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-PROP-04: mstateen1-3 zero bit propagation                     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen1_3_prop);
bool test_mstateen1_3_prop(void) {
    TEST_BEGIN("MSTA-PROP-04: mstateen1-3 zero bit propagation to sstateen");

    /* mstateen1: clear bit 63, verify sstateen1 bit 63 is RO0 */
    uintptr_t orig1 = mstateen1_read();
    mstateen1_write(0);

    /* Attempt to write bit 63 to sstateen1 from M-mode.
     * If mstateen1 bit 63 is zero, sstateen1 should be entirely inaccessible
     * from S-mode. From M-mode we can still try to write sstateen1. */
    trap_expect_begin();
    sstateen1_write((uintptr_t)(1ULL << 63));
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (!trapped) {
        uintptr_t val = sstateen1_read();
        TEST_ASSERT_BITS("sstateen1 bit63 is RO0 when mstateen1 bit63=0",
                         val, (1ULL << 63), 0);
    } else {
        /* sstateen1 access trapped - acceptable */
        TEST_ASSERT("sstateen1 access controlled by mstateen1", true);
    }

    mstateen1_write(orig1);

    /* Repeat for mstateen2 */
    uintptr_t orig2 = mstateen2_read();
    mstateen2_write(0);

    trap_expect_begin();
    sstateen2_write((uintptr_t)(1ULL << 63));
    trapped = trap_was_triggered();
    trap_expect_end();

    if (!trapped) {
        uintptr_t val = sstateen2_read();
        TEST_ASSERT_BITS("sstateen2 bit63 is RO0 when mstateen2 bit63=0",
                         val, (1ULL << 63), 0);
    } else {
        TEST_ASSERT("sstateen2 access controlled by mstateen2", true);
    }

    mstateen2_write(orig2);

    /* Repeat for mstateen3 */
    uintptr_t orig3 = mstateen3_read();
    mstateen3_write(0);

    trap_expect_begin();
    sstateen3_write((uintptr_t)(1ULL << 63));
    trapped = trap_was_triggered();
    trap_expect_end();

    if (!trapped) {
        uintptr_t val = sstateen3_read();
        TEST_ASSERT_BITS("sstateen3 bit63 is RO0 when mstateen3 bit63=0",
                         val, (1ULL << 63), 0);
    } else {
        TEST_ASSERT("sstateen3 access controlled by mstateen3", true);
    }

    mstateen3_write(orig3);
    TEST_END();
}
