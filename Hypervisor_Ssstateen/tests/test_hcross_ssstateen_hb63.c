/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 8.2: hstateen bit 63 controls sstateen access
 *
 * Extracted from Ssstateen/tests/test_hb63.c (Group 8).
 * ID mapping: SS-HB63-01~09 -> HCROSS-SSSTA-07~15
 *
 * Spec anchors:
 *   norm:hstateen_bit_63_op       — bit 63 controls VS sstateen access
 *   norm:hstateen_bit_63_writable — bit 63 always writable
 *
 * 9 tests: HCROSS-SSSTA-07 ~ HCROSS-SSSTA-15
 * =================================================================== */

/* ---- HCROSS-SSSTA-07: hstateen0 bit 63 writable (write 0) ---- */

TEST_REGISTER(test_hcross_sssta_07);
bool test_hcross_sssta_07(void)
{
    TEST_BEGIN("HCROSS-SSSTA-07: hstateen0 bit 63 writable (write 0)");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bit63(0, true);

    uintptr_t saved = hstateen_read(0);

    hstateen_clear_bits(0, STATEEN_BIT63);
    uintptr_t rb = hstateen_read(0);
    TEST_ASSERT("hstateen0 bit 63 cleared to 0", (rb & STATEEN_BIT63) == 0);

    hstateen_write(0, saved);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-08: hstateen0 bit 63 writable (write 1) ---- */

TEST_REGISTER(test_hcross_sssta_08);
bool test_hcross_sssta_08(void)
{
    TEST_BEGIN("HCROSS-SSSTA-08: hstateen0 bit 63 writable (write 1)");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bit63(0, true);

    uintptr_t saved = hstateen_read(0);

    hstateen_set_bits(0, STATEEN_BIT63);
    uintptr_t rb = hstateen_read(0);
    TEST_ASSERT("hstateen0 bit 63 set to 1", (rb & STATEEN_BIT63) != 0);

    hstateen_write(0, saved);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-09: hstateen0.SE0=0 blocks VS-mode read sstateen0 ---- */

TEST_REGISTER(test_hcross_sssta_09);
bool test_hcross_sssta_09(void)
{
    TEST_BEGIN("HCROSS-SSSTA-09: hstateen0.SE0=0 blocks VS-mode read sstateen0");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bit63(0, true);

    uintptr_t saved_hstateen0 = hstateen_read(0);
    hstateen_clear_bits(0, STATEEN_BIT63);

    EXPECT_VIRTUAL_INST(
        run_in_vs_mode(_vs_read_sstateen0, 0)
    );

    hstateen_write(0, saved_hstateen0);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-10: hstateen0.SE0=1 allows VS-mode read sstateen0 ---- */

TEST_REGISTER(test_hcross_sssta_10);
bool test_hcross_sssta_10(void)
{
    TEST_BEGIN("HCROSS-SSSTA-10: hstateen0.SE0=1 allows VS-mode read sstateen0");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bit63(0, true);

    uintptr_t saved_hstateen0 = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63);

    VS_EXPECT_NO_TRAP(
        run_in_vs_mode(_vs_read_sstateen0, 0)
    );

    hstateen_write(0, saved_hstateen0);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-11: hstateen0.SE0=0 blocks VS-mode write sstateen0 ---- */

TEST_REGISTER(test_hcross_sssta_11);
bool test_hcross_sssta_11(void)
{
    TEST_BEGIN("HCROSS-SSSTA-11: hstateen0.SE0=0 blocks VS-mode write sstateen0");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bit63(0, true);

    uintptr_t saved_hstateen0 = hstateen_read(0);
    hstateen_clear_bits(0, STATEEN_BIT63);

    EXPECT_VIRTUAL_INST(
        run_in_vs_mode(_vs_write_sstateen0, 0)
    );

    hstateen_write(0, saved_hstateen0);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-12: hstateen1 bit 63 writable ---- */

TEST_REGISTER(test_hcross_sssta_12);
bool test_hcross_sssta_12(void)
{
    TEST_BEGIN("HCROSS-SSSTA-12: hstateen1 bit 63 writable");
    REQUIRE_H_EXT();

    uintptr_t saved = mstateen_read(1);
    mstateen_set_bit63(1, true);

    if (!(mstateen_read(1) & STATEEN_BIT63))
    {
        mstateen_write(1, saved);
        TEST_SKIP("mstateen1 bit 63 not writable (hstateen1 not accessible)");
    }

    uintptr_t saved_h = hstateen_read(1);

    /* Write 1 */
    hstateen_set_bits(1, STATEEN_BIT63);
    uintptr_t rb1 = hstateen_read(1);

    /* Write 0 */
    hstateen_clear_bits(1, STATEEN_BIT63);
    uintptr_t rb0 = hstateen_read(1);

    if ((rb1 & STATEEN_BIT63) == 0)
    {
        hstateen_write(1, saved_h);
        mstateen_write(1, saved);
        TEST_SKIP("hstateen1 bit 63 not implemented (ROZ)");
    }

    TEST_ASSERT("hstateen1 bit 63 set", (rb1 & STATEEN_BIT63) != 0);
    TEST_ASSERT("hstateen1 bit 63 cleared", (rb0 & STATEEN_BIT63) == 0);

    hstateen_write(1, saved_h);
    mstateen_write(1, saved);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-13: hstateen1 bit63=0 blocks VS-mode sstateen1 ---- */

TEST_REGISTER(test_hcross_sssta_13);
bool test_hcross_sssta_13(void)
{
    TEST_BEGIN("HCROSS-SSSTA-13: hstateen1 bit63=0 blocks VS-mode sstateen1");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(1);
    mstateen_set_bit63(1, true);

    if (!(mstateen_read(1) & STATEEN_BIT63))
    {
        mstateen_write(1, saved_m);
        TEST_SKIP("mstateen1 bit 63 not writable (hstateen1 not accessible)");
    }

    uintptr_t saved_h = hstateen_read(1);

    /* Probe bit 63 */
    hstateen_set_bits(1, STATEEN_BIT63);
    if (!(hstateen_read(1) & STATEEN_BIT63))
    {
        hstateen_write(1, saved_h);
        mstateen_write(1, saved_m);
        TEST_SKIP("hstateen1 bit 63 not writable");
    }

    /* Clear bit 63 -> VS-mode cannot access sstateen1 */
    hstateen_clear_bits(1, STATEEN_BIT63);

    EXPECT_VIRTUAL_INST(
        run_in_vs_mode(_vs_read_sstateen, 1)
    );

    hstateen_write(1, saved_h);
    mstateen_write(1, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-14: hstateen2 bit 63 controls sstateen2 ---- */

TEST_REGISTER(test_hcross_sssta_14);
bool test_hcross_sssta_14(void)
{
    TEST_BEGIN("HCROSS-SSSTA-14: hstateen2 bit 63 controls sstateen2");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(2);
    mstateen_set_bit63(2, true);

    if (!(mstateen_read(2) & STATEEN_BIT63))
    {
        mstateen_write(2, saved_m);
        TEST_SKIP("mstateen2 bit 63 not writable (hstateen2 not accessible)");
    }

    uintptr_t saved_h = hstateen_read(2);

    /* Probe bit 63 */
    hstateen_set_bits(2, STATEEN_BIT63);
    if (!(hstateen_read(2) & STATEEN_BIT63))
    {
        hstateen_write(2, saved_h);
        mstateen_write(2, saved_m);
        TEST_SKIP("hstateen2 bit 63 not writable");
    }

    /* bit63=0: VS-mode trap */
    hstateen_clear_bits(2, STATEEN_BIT63);
    EXPECT_VIRTUAL_INST(
        run_in_vs_mode(_vs_read_sstateen, 2)
    );

    /* bit63=1: VS-mode OK */
    hstateen_set_bits(2, STATEEN_BIT63);
    VS_EXPECT_NO_TRAP(
        run_in_vs_mode(_vs_read_sstateen, 2)
    );

    hstateen_write(2, saved_h);
    mstateen_write(2, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-15: hstateen3 bit 63 controls sstateen3 ---- */

TEST_REGISTER(test_hcross_sssta_15);
bool test_hcross_sssta_15(void)
{
    TEST_BEGIN("HCROSS-SSSTA-15: hstateen3 bit 63 controls sstateen3");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(3);
    mstateen_set_bit63(3, true);

    if (!(mstateen_read(3) & STATEEN_BIT63))
    {
        mstateen_write(3, saved_m);
        TEST_SKIP("mstateen3 bit 63 not writable (hstateen3 not accessible)");
    }

    uintptr_t saved_h = hstateen_read(3);

    /* Probe bit 63 */
    hstateen_set_bits(3, STATEEN_BIT63);
    if (!(hstateen_read(3) & STATEEN_BIT63))
    {
        hstateen_write(3, saved_h);
        mstateen_write(3, saved_m);
        TEST_SKIP("hstateen3 bit 63 not writable");
    }

    /* bit63=0: VS-mode trap */
    hstateen_clear_bits(3, STATEEN_BIT63);
    EXPECT_VIRTUAL_INST(
        run_in_vs_mode(_vs_read_sstateen, 3)
    );

    /* bit63=1: VS-mode OK */
    hstateen_set_bits(3, STATEEN_BIT63);
    VS_EXPECT_NO_TRAP(
        run_in_vs_mode(_vs_read_sstateen, 3)
    );

    hstateen_write(3, saved_h);
    mstateen_write(3, saved_m);
    HYP_TEST_END();
}
