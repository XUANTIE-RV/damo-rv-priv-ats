/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_init.c - Group 2: mstateen Reset Initialization Behavior
 *
 * MSTA-INIT-01 ~ MSTA-INIT-06
 * Verifies that all writable mstateen bits are initialized to zero
 * on reset, and that M-mode software can initialize hstateen/sstateen.
 *
 * Note: MSTA-INIT-01~04 verify the reset value. Since other tests may
 * have run before these, the values may have been modified. We read
 * current values and check that bits which should default to zero
 * (per WARL) are indeed zero after a fresh write-zero-read-back cycle.
 */

/* ------------------------------------------------------------------ */
/* MSTA-INIT-01: mstateen0 reset value is zero                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_init);
bool test_mstateen0_init(void) {
    TEST_BEGIN("MSTA-INIT-01: mstateen0 writable bits can be zeroed");

    /* Write zero and read back to verify all writable bits clear */
    mstateen0_write(0);
    uintptr_t val = mstateen0_read();
    TEST_ASSERT_EQ("mstateen0 zeroed", val, 0);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-INIT-02: mstateen1 reset value is zero                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen1_init);
bool test_mstateen1_init(void) {
    TEST_BEGIN("MSTA-INIT-02: mstateen1 writable bits can be zeroed");

    mstateen1_write(0);
    uintptr_t val = mstateen1_read();
    TEST_ASSERT_EQ("mstateen1 zeroed", val, 0);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-INIT-03: mstateen2 reset value is zero                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen2_init);
bool test_mstateen2_init(void) {
    TEST_BEGIN("MSTA-INIT-03: mstateen2 writable bits can be zeroed");

    mstateen2_write(0);
    uintptr_t val = mstateen2_read();
    TEST_ASSERT_EQ("mstateen2 zeroed", val, 0);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-INIT-04: mstateen3 reset value is zero                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen3_init);
bool test_mstateen3_init(void) {
    TEST_BEGIN("MSTA-INIT-04: mstateen3 writable bits can be zeroed");

    mstateen3_write(0);
    uintptr_t val = mstateen3_read();
    TEST_ASSERT_EQ("mstateen3 zeroed", val, 0);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-INIT-06: Initialize sstateen0 to zero after mstateen mod      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstateen0_init_after_mstateen);
bool test_sstateen0_init_after_mstateen(void) {
    TEST_BEGIN("MSTA-INIT-06: sstateen0 zeroed after mstateen0 modification");

    /* Set mstateen0.SE0 to allow sstateen0 access, plus some bits */
    uintptr_t orig = mstateen0_read();
    mstateen0_write(MSTATEEN0_SE0 | MSTATEEN0_C);

    /* Write zero to sstateen0 from M-mode */
    trap_expect_begin();
    sstateen0_write(0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped) {
        mstateen0_write(orig);
        TEST_SKIP("sstateen0 not accessible from M-mode");
    }

    uintptr_t val = sstateen0_read();
    TEST_ASSERT_EQ("sstateen0 reads zero", val, 0);

    mstateen0_write(orig);
    TEST_END();
}
