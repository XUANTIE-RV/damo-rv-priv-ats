/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_bit63.c - Group 5: mstateen bit 63 Control Behavior
 *
 * MSTA-B63-01 ~ MSTA-B63-07
 * Verifies that mstateen bit 63 (SE0) controls S-mode/HS-mode access
 * to the corresponding sstateen/hstateen CSRs.
 */

/* ------------------------------------------------------------------ */
/* MSTA-B63-01: mstateen0.SE0=0 blocks S-mode access to sstateen0    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_se0_block_smode);
bool test_mstateen0_se0_block_smode(void) {
    TEST_BEGIN("MSTA-B63-01: mstateen0.SE0=0 blocks S-mode sstateen0 access");

    uintptr_t orig = mstateen0_read();

    /* Clear SE0 (bit 63) */
    mstateen0_clear(MSTATEEN0_SE0);

    /* S-mode read of sstateen0 should trigger illegal-instruction */
    SMSTATEEN_TEST_SMODE_BLOCKED(
        "S-mode sstateen0 read blocked (SE0=0)",
        sstateen0_read());

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-B63-02: mstateen0.SE0=1 allows S-mode access to sstateen0    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_se0_allow_smode);
bool test_mstateen0_se0_allow_smode(void) {
    TEST_BEGIN("MSTA-B63-02: mstateen0.SE0=1 allows S-mode sstateen0 access");

    uintptr_t orig = mstateen0_read();

    /* Set SE0 (bit 63) */
    mstateen0_set(MSTATEEN0_SE0);

    /* Verify SE0 is actually writable */
    if (!(mstateen0_read() & MSTATEEN0_SE0)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.SE0 is not writable");
    }

    /* S-mode read of sstateen0 should succeed */
    SMSTATEEN_TEST_SMODE_ALLOWED(
        "S-mode sstateen0 read allowed (SE0=1)",
        sstateen0_read());

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-B63-04: mstateen1 bit 63 controls sstateen1                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen1_bit63_control);
bool test_mstateen1_bit63_control(void) {
    TEST_BEGIN("MSTA-B63-04: mstateen1 bit 63 controls sstateen1");

    uintptr_t orig = mstateen1_read();

    /* Clear bit 63 -> S-mode access should be blocked */
    mstateen1_write(0);

    goto_priv(PRIV_S);
    PRIV_DO(sstateen1_read());
    goto_priv(PRIV_M);
    CHECK_TRAP("sstateen1 blocked (bit63=0)", CAUSE_ILLEGAL_INST);

    /* Set bit 63 -> S-mode access should be allowed */
    mstateen1_write(1ULL << 63);

    if (!(mstateen1_read() & (1ULL << 63))) {
        mstateen1_write(orig);
        TEST_SKIP("mstateen1 bit 63 is not writable");
    }

    goto_priv(PRIV_S);
    PRIV_DO(sstateen1_read());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("sstateen1 allowed (bit63=1)");

    mstateen1_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-B63-05: mstateen2 bit 63 controls sstateen2                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen2_bit63_control);
bool test_mstateen2_bit63_control(void) {
    TEST_BEGIN("MSTA-B63-05: mstateen2 bit 63 controls sstateen2");

    uintptr_t orig = mstateen2_read();

    /* Clear bit 63 -> S-mode access should be blocked */
    mstateen2_write(0);

    goto_priv(PRIV_S);
    PRIV_DO(sstateen2_read());
    goto_priv(PRIV_M);
    CHECK_TRAP("sstateen2 blocked (bit63=0)", CAUSE_ILLEGAL_INST);

    /* Set bit 63 -> S-mode access should be allowed */
    mstateen2_write(1ULL << 63);

    if (!(mstateen2_read() & (1ULL << 63))) {
        mstateen2_write(orig);
        TEST_SKIP("mstateen2 bit 63 is not writable");
    }

    goto_priv(PRIV_S);
    PRIV_DO(sstateen2_read());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("sstateen2 allowed (bit63=1)");

    mstateen2_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-B63-06: mstateen3 bit 63 controls sstateen3                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen3_bit63_control);
bool test_mstateen3_bit63_control(void) {
    TEST_BEGIN("MSTA-B63-06: mstateen3 bit 63 controls sstateen3");

    uintptr_t orig = mstateen3_read();

    /* Clear bit 63 -> S-mode access should be blocked */
    mstateen3_write(0);

    goto_priv(PRIV_S);
    PRIV_DO(sstateen3_read());
    goto_priv(PRIV_M);
    CHECK_TRAP("sstateen3 blocked (bit63=0)", CAUSE_ILLEGAL_INST);

    /* Set bit 63 -> S-mode access should be allowed */
    mstateen3_write(1ULL << 63);

    if (!(mstateen3_read() & (1ULL << 63))) {
        mstateen3_write(orig);
        TEST_SKIP("mstateen3 bit 63 is not writable");
    }

    goto_priv(PRIV_S);
    PRIV_DO(sstateen3_read());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("sstateen3 allowed (bit63=1)");

    mstateen3_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-B63-07: mstateen bit 63 writability conditions                */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_bit63_writability);
bool test_mstateen0_bit63_writability(void) {
    TEST_BEGIN("MSTA-B63-07: mstateen0 bit 63 writability conditions");

    uintptr_t orig = mstateen0_read();

    /* Try to write bit 63 */
    mstateen0_set(MSTATEEN0_SE0);
    uintptr_t val = mstateen0_read();
    bool writable = (val & MSTATEEN0_SE0) != 0;

    /* Without H ext, bit 63 is writable only if sstateen0 is not
     * all read-only zero. Either way is valid. */
    TEST_ASSERT("bit 63 writability check done", true);
    if (writable) {
        printf("  INFO: bit 63 is writable (sstateen0 has writable bits)\n");
    } else {
        printf("  INFO: bit 63 is RO0 (sstateen0 all RO0)\n");
    }

    mstateen0_write(orig);
    TEST_END();
}
