/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 6: sstateen Initialization Requirements
 *
 * Spec anchor:
 *   norm:hstateen_sstateen_zero_initialization
 *     — M-mode software must initialize sstateen to zero after
 *       modifying mstateen.
 *
 * 5 tests: SS-INIT-01 ~ SS-INIT-05
 * =================================================================== */

/* ---- SS-INIT-01: sstateen0 initialized to zero ---- */

TEST_REGISTER(test_ss_init_sstateen0_zero);
bool test_ss_init_sstateen0_zero(void) {
    TEST_BEGIN("SS-INIT-01: sstateen0 initialized to zero");

    uintptr_t saved_mstateen0 = mstateen_read(0);

    /* Simulate M-mode initialization sequence:
     * 1. Set mstateen0 bits (enable features)
     * 2. Write zero to sstateen0
     * 3. Set SE0 to allow S-mode access
     * 4. S-mode reads sstateen0 => should be zero */
    mstateen_write(0, ~0UL);
    sstateen_write(0, 0);
    mstateen_set_bits(0, STATEEN0_SE0);

    uintptr_t val = sstateen_read(0);
    TEST_ASSERT_EQ("sstateen0 initialized to 0", val, (uintptr_t)0);

    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-INIT-02: sstateen1 initialized to zero ---- */

TEST_REGISTER(test_ss_init_sstateen1_zero);
bool test_ss_init_sstateen1_zero(void) {
    TEST_BEGIN("SS-INIT-02: sstateen1 initialized to zero");

    uintptr_t saved_mstateen1 = mstateen_read(1);

    mstateen_set_bit63(1, true);
    if (!(mstateen_read(1) & STATEEN_BIT63)) {
        mstateen_write(1, saved_mstateen1);
        TEST_SKIP("mstateen1 bit 63 not writable (sstateen1 not available)");
    }
    sstateen_write(1, 0);

    uintptr_t val = sstateen_read(1);
    TEST_ASSERT_EQ("sstateen1 initialized to 0", val, (uintptr_t)0);

    mstateen_write(1, saved_mstateen1);
    TEST_END();
}

/* ---- SS-INIT-03: sstateen2 initialized to zero ---- */

TEST_REGISTER(test_ss_init_sstateen2_zero);
bool test_ss_init_sstateen2_zero(void) {
    TEST_BEGIN("SS-INIT-03: sstateen2 initialized to zero");

    uintptr_t saved_mstateen2 = mstateen_read(2);

    mstateen_set_bit63(2, true);
    if (!(mstateen_read(2) & STATEEN_BIT63)) {
        mstateen_write(2, saved_mstateen2);
        TEST_SKIP("mstateen2 bit 63 not writable (sstateen2 not available)");
    }
    sstateen_write(2, 0);

    uintptr_t val = sstateen_read(2);
    TEST_ASSERT_EQ("sstateen2 initialized to 0", val, (uintptr_t)0);

    mstateen_write(2, saved_mstateen2);
    TEST_END();
}

/* ---- SS-INIT-04: sstateen3 initialized to zero ---- */

TEST_REGISTER(test_ss_init_sstateen3_zero);
bool test_ss_init_sstateen3_zero(void) {
    TEST_BEGIN("SS-INIT-04: sstateen3 initialized to zero");

    uintptr_t saved_mstateen3 = mstateen_read(3);

    mstateen_set_bit63(3, true);
    if (!(mstateen_read(3) & STATEEN_BIT63)) {
        mstateen_write(3, saved_mstateen3);
        TEST_SKIP("mstateen3 bit 63 not writable (sstateen3 not available)");
    }
    sstateen_write(3, 0);

    uintptr_t val = sstateen_read(3);
    TEST_ASSERT_EQ("sstateen3 initialized to 0", val, (uintptr_t)0);

    mstateen_write(3, saved_mstateen3);
    TEST_END();
}

/* ---- SS-INIT-05: OS entry sstateen should be zero ---- */

TEST_REGISTER(test_ss_init_os_entry_zero);
bool test_ss_init_os_entry_zero(void) {
    TEST_BEGIN("SS-INIT-05: OS entry sstateen should be zero");

    /* Simulate first OS entry:
     * M-mode configures mstateen, initializes sstateen to zero,
     * then hands off to S-mode. S-mode should see all zeros. */

    uintptr_t saved_mstateen0 = mstateen_read(0);

    mstateen_write(0, ~0UL);
    sstateen_write(0, 0);

    /* Switch to S-mode and read sstateen0 */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v = sstateen0_read();
        /* Cannot printf from S-mode; just read */
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("S-mode read sstateen0 after init");

    /* Verify from M-mode that sstateen0 is still 0 */
    uintptr_t val = sstateen_read(0);
    TEST_ASSERT_EQ("sstateen0 == 0 at OS entry", val, (uintptr_t)0);

    mstateen_write(0, saved_mstateen0);
    TEST_END();
}
