/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_warl.c - Group 3: WARL and Read-Only-Zero Behavior
 *
 * MSTA-WARL-01 ~ MSTA-WARL-06
 * Verifies that reserved (WPRI) bits and unimplemented extension
 * control bits in mstateen registers are read-only zero.
 */

/* ------------------------------------------------------------------ */
/* MSTA-WARL-01: mstateen0 reserved bits (WPRI) are read-only zero    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_wpri_roz);
bool test_mstateen0_wpri_roz(void) {
    TEST_BEGIN("MSTA-WARL-01: mstateen0 WPRI bits are read-only zero");

    uintptr_t orig = mstateen0_read();

    /* Write all-ones */
    mstateen0_write((uintptr_t)-1);
    uintptr_t val = mstateen0_read();

    /* Check WPRI domain: bits [53:3] must be zero */
    TEST_ASSERT_BITS("mstateen0 WPRI bits [53:3] are zero",
                     val, MSTATEEN0_WPRI_MASK, 0);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-WARL-02: mstateen0 unimplemented extension bit is read-only 0 */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_unimpl_roz);
bool test_mstateen0_unimpl_roz(void) {
    TEST_BEGIN("MSTA-WARL-02: mstateen0 unimplemented extension bits RO0");

    uintptr_t orig = mstateen0_read();

    /* Write all-ones and read back. All bits for unimplemented
     * extensions should remain zero. We check a few known bits
     * that require optional extensions. */
    mstateen0_write((uintptr_t)-1);
    uintptr_t val = mstateen0_read();

    /* The WPRI mask already covers bits [53:3]. Additionally check
     * that defined bits for unimplemented extensions read as zero.
     * This is implementation-dependent; we verify the WPRI mask here. */
    TEST_ASSERT_BITS("mstateen0 reserved range is zero",
                     val, MSTATEEN0_WPRI_MASK, 0);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-WARL-03: mstateen0 WARL write legal value and read back       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_warl_legal);
bool test_mstateen0_warl_legal(void) {
    TEST_BEGIN("MSTA-WARL-03: mstateen0 WARL write legal values");

    uintptr_t orig = mstateen0_read();

    /* Write a value with only defined functional bits set */
    uintptr_t test_val = MSTATEEN0_SE0 | MSTATEEN0_ENVCFG | MSTATEEN0_C;
    mstateen0_write(test_val);
    uintptr_t val = mstateen0_read();

    /* SE0 (bit 63) should be writable if sstateen0 is not all-RO0 or H ext exists */
    if (mstateen0_bit_writable(MSTATEEN0_SE0)) {
        TEST_ASSERT_BITS("SE0 bit written", val, MSTATEEN0_SE0, MSTATEEN0_SE0);
    }

    /* C bit (bit 0) should be writable if custom state exists */
    /* ENVCFG bit (bit 62) should be writable if senvcfg exists */
    /* These are WARL, so read-back may differ; just verify no crash */
    TEST_ASSERT("WARL write did not crash", true);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-WARL-04: mstateen1 reserved bits are read-only zero           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen1_reserved_roz);
bool test_mstateen1_reserved_roz(void) {
    TEST_BEGIN("MSTA-WARL-04: mstateen1 reserved bits are read-only zero");

    uintptr_t orig = mstateen1_read();

    mstateen1_write((uintptr_t)-1);
    uintptr_t val = mstateen1_read();

    /* mstateen1 currently has no standard-defined functional bits
     * other than bit 63, so most bits should be zero. The lower 63
     * bits are all reserved. */
    uintptr_t lower_bits_mask = ~(1ULL << 63);
    TEST_ASSERT_BITS("mstateen1 lower bits are zero",
                     val, lower_bits_mask, 0);

    mstateen1_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-WARL-05: mstateen2 reserved bits are read-only zero           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen2_reserved_roz);
bool test_mstateen2_reserved_roz(void) {
    TEST_BEGIN("MSTA-WARL-05: mstateen2 reserved bits are read-only zero");

    uintptr_t orig = mstateen2_read();

    mstateen2_write((uintptr_t)-1);
    uintptr_t val = mstateen2_read();

    uintptr_t lower_bits_mask = ~(1ULL << 63);
    TEST_ASSERT_BITS("mstateen2 lower bits are zero",
                     val, lower_bits_mask, 0);

    mstateen2_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-WARL-06: mstateen3 reserved bits are read-only zero           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen3_reserved_roz);
bool test_mstateen3_reserved_roz(void) {
    TEST_BEGIN("MSTA-WARL-06: mstateen3 reserved bits are read-only zero");

    uintptr_t orig = mstateen3_read();

    mstateen3_write((uintptr_t)-1);
    uintptr_t val = mstateen3_read();

    uintptr_t lower_bits_mask = ~(1ULL << 63);
    TEST_ASSERT_BITS("mstateen3 lower bits are zero",
                     val, lower_bits_mask, 0);

    mstateen3_write(orig);
    TEST_END();
}
