/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 8.6: hstateen Encoding and mstateen Consistency
 *
 * Extracted from Ssstateen/tests/test_henc.c (Group 12).
 * ID mapping: SS-HENC-01~05 -> HCROSS-SSSTA-46~50
 *
 * Spec anchor:
 *   norm:hstateen_encoding
 *     — hstateen CSR encoding is identical to mstateen CSR encoding.
 *       The only difference is hstateen controls VS/VU access.
 *
 * 5 tests: HCROSS-SSSTA-46 ~ HCROSS-SSSTA-50
 * =================================================================== */

/* ---- HCROSS-SSSTA-46: hstateen0 bit fields match mstateen0 ---- */

TEST_REGISTER(test_hcross_sssta_46);
bool test_hcross_sssta_46(void)
{
    TEST_BEGIN("HCROSS-SSSTA-46: hstateen0 bit fields match mstateen0");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    uintptr_t saved_h = hstateen_read(0);

    /* Get mstateen0 writable mask */
    mstateen_write(0, ~0UL);
    uintptr_t msta_mask = mstateen_read(0);

    /* Get hstateen0 writable mask (with mstateen0 fully open) */
    hstateen_write(0, ~0UL);
    uintptr_t hsta_mask = hstateen_read(0);

    /* Known functional bits that should have identical positions */
    uintptr_t known_bits = STATEEN0_C | STATEEN0_FCSR | STATEEN0_JVT |
                           STATEEN0_CONTEXT | STATEEN0_IMSIC | STATEEN0_AIA |
                           STATEEN0_CSRIND | STATEEN0_ENVCFG | STATEEN0_SE0;

    /* For each known bit that exists in both, position must match */
    uintptr_t msta_known = msta_mask & known_bits;
    uintptr_t hsta_known = hsta_mask & known_bits;

    /* hstateen0 known bits should be a subset of mstateen0 known bits
     * (mstateen0 may have additional bits like P1P13, SRMCFG) */
    uintptr_t hsta_only = hsta_known & ~msta_known;

    TEST_ASSERT("hstateen0 known bits are subset of mstateen0",
                hsta_only == 0);

    printf("  mstateen0 writable mask:  0x%lx\n", (unsigned long)msta_mask);
    printf("  hstateen0 writable mask:  0x%lx\n", (unsigned long)hsta_mask);
    printf("  mstateen0 known bits:     0x%lx\n", (unsigned long)msta_known);
    printf("  hstateen0 known bits:     0x%lx\n", (unsigned long)hsta_known);

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-47: hstateen0 functional bits symmetric with mstateen0 ---- */

TEST_REGISTER(test_hcross_sssta_47);
bool test_hcross_sssta_47(void)
{
    TEST_BEGIN("HCROSS-SSSTA-47: hstateen0 functional bits symmetric with mstateen0");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    uintptr_t saved_h = hstateen_read(0);

    mstateen_write(0, ~0UL);
    uintptr_t msta_mask = mstateen_read(0);

    hstateen_write(0, ~0UL);
    uintptr_t hsta_mask = hstateen_read(0);

    /* The overlapping bits should be identical */
    uintptr_t overlap = msta_mask & hsta_mask;
    uintptr_t hsta_extra = hsta_mask & ~msta_mask;

    TEST_ASSERT("hstateen0 has no bits outside mstateen0 mask",
                hsta_extra == 0);

    printf("  Overlap mask:  0x%lx\n", (unsigned long)overlap);
    printf("  hstateen0 extra (should be 0): 0x%lx\n",
           (unsigned long)hsta_extra);

    /* mstateen0 may have bits not in hstateen0 (e.g., P1P13, SRMCFG) */
    uintptr_t msta_extra = msta_mask & ~hsta_mask;
    printf("  mstateen0 extra (OK, may have M-only bits): 0x%lx\n",
           (unsigned long)msta_extra);

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-48: hstateen1 encoding matches mstateen1 ---- */

TEST_REGISTER(test_hcross_sssta_48);
bool test_hcross_sssta_48(void)
{
    TEST_BEGIN("HCROSS-SSSTA-48: hstateen1 encoding matches mstateen1");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(1);
    mstateen_set_bit63(1, true);

    if (!(mstateen_read(1) & STATEEN_BIT63))
    {
        mstateen_write(1, saved_m);
        TEST_SKIP("mstateen1 bit 63 not writable (hstateen1 not accessible)");
    }

    uintptr_t saved_h = hstateen_read(1);

    mstateen_write(1, ~0UL);
    uintptr_t msta_mask = mstateen_read(1);

    hstateen_write(1, ~0UL);
    uintptr_t hsta_mask = hstateen_read(1);

    uintptr_t hsta_extra = hsta_mask & ~msta_mask;
    TEST_ASSERT("hstateen1 bits are subset of mstateen1", hsta_extra == 0);

    printf("  mstateen1 mask: 0x%lx\n", (unsigned long)msta_mask);
    printf("  hstateen1 mask: 0x%lx\n", (unsigned long)hsta_mask);

    hstateen_write(1, saved_h);
    mstateen_write(1, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-49: hstateen2 encoding matches mstateen2 ---- */

TEST_REGISTER(test_hcross_sssta_49);
bool test_hcross_sssta_49(void)
{
    TEST_BEGIN("HCROSS-SSSTA-49: hstateen2 encoding matches mstateen2");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(2);
    mstateen_set_bit63(2, true);

    if (!(mstateen_read(2) & STATEEN_BIT63))
    {
        mstateen_write(2, saved_m);
        TEST_SKIP("mstateen2 bit 63 not writable (hstateen2 not accessible)");
    }

    uintptr_t saved_h = hstateen_read(2);

    mstateen_write(2, ~0UL);
    uintptr_t msta_mask = mstateen_read(2);

    hstateen_write(2, ~0UL);
    uintptr_t hsta_mask = hstateen_read(2);

    uintptr_t hsta_extra = hsta_mask & ~msta_mask;
    TEST_ASSERT("hstateen2 bits are subset of mstateen2", hsta_extra == 0);

    printf("  mstateen2 mask: 0x%lx\n", (unsigned long)msta_mask);
    printf("  hstateen2 mask: 0x%lx\n", (unsigned long)hsta_mask);

    hstateen_write(2, saved_h);
    mstateen_write(2, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-50: hstateen3 encoding matches mstateen3 ---- */

TEST_REGISTER(test_hcross_sssta_50);
bool test_hcross_sssta_50(void)
{
    TEST_BEGIN("HCROSS-SSSTA-50: hstateen3 encoding matches mstateen3");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(3);
    mstateen_set_bit63(3, true);

    if (!(mstateen_read(3) & STATEEN_BIT63))
    {
        mstateen_write(3, saved_m);
        TEST_SKIP("mstateen3 bit 63 not writable (hstateen3 not accessible)");
    }

    uintptr_t saved_h = hstateen_read(3);

    mstateen_write(3, ~0UL);
    uintptr_t msta_mask = mstateen_read(3);

    hstateen_write(3, ~0UL);
    uintptr_t hsta_mask = hstateen_read(3);

    uintptr_t hsta_extra = hsta_mask & ~msta_mask;
    TEST_ASSERT("hstateen3 bits are subset of mstateen3", hsta_extra == 0);

    printf("  mstateen3 mask: 0x%lx\n", (unsigned long)msta_mask);
    printf("  hstateen3 mask: 0x%lx\n", (unsigned long)hsta_mask);

    hstateen_write(3, saved_h);
    mstateen_write(3, saved_m);
    HYP_TEST_END();
}
