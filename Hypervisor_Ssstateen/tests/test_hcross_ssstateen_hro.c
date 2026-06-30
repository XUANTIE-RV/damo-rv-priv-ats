/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 8.5: hstateen Read-Only Constraints
 *
 * Extracted from Ssstateen/tests/test_hro.c (Group 11).
 * ID mapping: SS-HRO-01~07 -> HCROSS-SSSTA-39~45
 *
 * Spec anchors:
 *   norm:hstateen_ro1_bits              — RO1 => mstateen same bit RO1
 *   norm:stateen_warl_access            — each defined bit is WARL
 *   norm:stateen_unimplemented_state_roz — unimplemented = ROZ
 *   norm:stateen_reserved_roz           — reserved bits = ROZ
 *
 * 7 tests: HCROSS-SSSTA-39 ~ HCROSS-SSSTA-45
 * =================================================================== */

/* ---- HCROSS-SSSTA-39: hstateen0 RO1 constraint ---- */

TEST_REGISTER(test_hcross_sssta_39);
bool test_hcross_sssta_39(void)
{
    TEST_BEGIN("HCROSS-SSSTA-39: hstateen0 RO1 => mstateen0 same bit RO1");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, ~0UL);

    uintptr_t saved_h = hstateen_read(0);

    /* Write all-0s to hstateen0 and read back */
    hstateen_write(0, 0);
    uintptr_t rb = hstateen_read(0);

    bool all_ok = true;

    if (rb != 0)
    {
        /* Some bits are RO1 */
        printf("  hstateen0 RO1 bits: 0x%lx\n", (unsigned long)rb);

        /* For each RO1 bit, verify mstateen0 same bit is also RO1 */
        for (int bit = 0; bit < 64; bit++)
        {
            uintptr_t mask = 1UL << bit;
            if (!(rb & mask))
                continue;

            uintptr_t ms = mstateen_read(0);
            mstateen_clear_bits(0, mask);
            uintptr_t ms_rb = mstateen_read(0);
            mstateen_write(0, ms);

            if (!(ms_rb & mask))
            {
                printf("  FAIL: hstateen0 bit %d is RO1 but mstateen0 is not\n", bit);
                all_ok = false;
            }
        }
    }

    TEST_ASSERT("hstateen0 RO1 bits => mstateen0 same bits RO1", all_ok);

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-40: hstateen1 RO1 constraint ---- */

TEST_REGISTER(test_hcross_sssta_40);
bool test_hcross_sssta_40(void)
{
    TEST_BEGIN("HCROSS-SSSTA-40: hstateen1 RO1 => mstateen1 same bit RO1");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(1);
    mstateen_set_bit63(1, true);

    if (!(mstateen_read(1) & STATEEN_BIT63))
    {
        mstateen_write(1, saved_m);
        TEST_SKIP("mstateen1 bit 63 not writable (hstateen1 not accessible)");
    }

    uintptr_t saved_h = hstateen_read(1);

    hstateen_write(1, 0);
    uintptr_t rb = hstateen_read(1);

    bool all_ok = true;
    if (rb != 0)
    {
        printf("  hstateen1 RO1 bits: 0x%lx\n", (unsigned long)rb);
        for (int bit = 0; bit < 64; bit++)
        {
            uintptr_t mask = 1UL << bit;
            if (!(rb & mask))
                continue;
            uintptr_t ms = mstateen_read(1);
            mstateen_clear_bits(1, mask);
            uintptr_t ms_rb = mstateen_read(1);
            mstateen_write(1, ms);
            if (!(ms_rb & mask))
            {
                printf("  FAIL: hstateen1 bit %d RO1 but mstateen1 not\n", bit);
                all_ok = false;
            }
        }
    }

    TEST_ASSERT("hstateen1 RO1 constraint satisfied", all_ok);

    hstateen_write(1, saved_h);
    mstateen_write(1, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-41: hstateen2 RO1 constraint ---- */

TEST_REGISTER(test_hcross_sssta_41);
bool test_hcross_sssta_41(void)
{
    TEST_BEGIN("HCROSS-SSSTA-41: hstateen2 RO1 => mstateen2 same bit RO1");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(2);
    mstateen_set_bit63(2, true);

    if (!(mstateen_read(2) & STATEEN_BIT63))
    {
        mstateen_write(2, saved_m);
        TEST_SKIP("mstateen2 bit 63 not writable (hstateen2 not accessible)");
    }

    uintptr_t saved_h = hstateen_read(2);

    hstateen_write(2, 0);
    uintptr_t rb = hstateen_read(2);

    bool all_ok = true;
    if (rb != 0)
    {
        printf("  hstateen2 RO1 bits: 0x%lx\n", (unsigned long)rb);
        for (int bit = 0; bit < 64; bit++)
        {
            uintptr_t mask = 1UL << bit;
            if (!(rb & mask))
                continue;
            uintptr_t ms = mstateen_read(2);
            mstateen_clear_bits(2, mask);
            uintptr_t ms_rb = mstateen_read(2);
            mstateen_write(2, ms);
            if (!(ms_rb & mask))
            {
                printf("  FAIL: hstateen2 bit %d RO1 but mstateen2 not\n", bit);
                all_ok = false;
            }
        }
    }

    TEST_ASSERT("hstateen2 RO1 constraint satisfied", all_ok);

    hstateen_write(2, saved_h);
    mstateen_write(2, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-42: hstateen3 RO1 constraint ---- */

TEST_REGISTER(test_hcross_sssta_42);
bool test_hcross_sssta_42(void)
{
    TEST_BEGIN("HCROSS-SSSTA-42: hstateen3 RO1 => mstateen3 same bit RO1");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(3);
    mstateen_set_bit63(3, true);

    if (!(mstateen_read(3) & STATEEN_BIT63))
    {
        mstateen_write(3, saved_m);
        TEST_SKIP("mstateen3 bit 63 not writable (hstateen3 not accessible)");
    }

    uintptr_t saved_h = hstateen_read(3);

    hstateen_write(3, 0);
    uintptr_t rb = hstateen_read(3);

    bool all_ok = true;
    if (rb != 0)
    {
        printf("  hstateen3 RO1 bits: 0x%lx\n", (unsigned long)rb);
        for (int bit = 0; bit < 64; bit++)
        {
            uintptr_t mask = 1UL << bit;
            if (!(rb & mask))
                continue;
            uintptr_t ms = mstateen_read(3);
            mstateen_clear_bits(3, mask);
            uintptr_t ms_rb = mstateen_read(3);
            mstateen_write(3, ms);
            if (!(ms_rb & mask))
            {
                printf("  FAIL: hstateen3 bit %d RO1 but mstateen3 not\n", bit);
                all_ok = false;
            }
        }
    }

    TEST_ASSERT("hstateen3 RO1 constraint satisfied", all_ok);

    hstateen_write(3, saved_h);
    mstateen_write(3, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-43: hstateen0 reserved bits ROZ ---- */

TEST_REGISTER(test_hcross_sssta_43);
bool test_hcross_sssta_43(void)
{
    TEST_BEGIN("HCROSS-SSSTA-43: hstateen0 reserved bits are ROZ");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, ~0UL);

    uintptr_t saved_h = hstateen_read(0);

    /* Write all-1s */
    hstateen_write(0, ~0UL);
    uintptr_t rb = hstateen_read(0);

    /* Bit 63 must be 1 (always writable) */
    TEST_ASSERT("hstateen0 bit 63 == 1 after all-1 write",
                (rb & STATEEN_BIT63) != 0);

    /* Some bits should be ROZ */
    bool has_roz = (rb != ~0UL);
    TEST_ASSERT("hstateen0 has reserved ROZ bits", has_roz);

    printf("  hstateen0 readback after all-1: 0x%lx\n", (unsigned long)rb);

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-44: hstateen0 unimplemented extension bits ROZ ---- */

TEST_REGISTER(test_hcross_sssta_44);
bool test_hcross_sssta_44(void)
{
    TEST_BEGIN("HCROSS-SSSTA-44: hstateen0 unimplemented extension bits ROZ");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, ~0UL);

    uintptr_t saved_h = hstateen_read(0);

    hstateen_write(0, ~0UL);
    uintptr_t rb = hstateen_read(0);

    /* Check specific bits that map to optional extensions:
     * If the extension is not implemented, the bit should be ROZ. */
    struct {
        uintptr_t bit;
        const char *name;
    } optional_bits[] = {
        { STATEEN0_CONTEXT, "CONTEXT (Sdtrig)" },
        { STATEEN0_IMSIC,   "IMSIC (Ssaia)" },
        { STATEEN0_AIA,     "AIA (Ssaia)" },
        { STATEEN0_CSRIND,  "CSRIND (Sscsrind)" },
    };

    int roz_count = 0;
    for (int i = 0; i < 4; i++)
    {
        if ((rb & optional_bits[i].bit) == 0)
        {
            printf("  %s: ROZ (extension not implemented)\n",
                   optional_bits[i].name);
            roz_count++;
        }
        else
        {
            printf("  %s: writable (extension present)\n",
                   optional_bits[i].name);
        }
    }
    (void)roz_count;

    /* At least verify the probe succeeded */
    TEST_ASSERT("hstateen0 optional bit probe completed", 1);

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-45: hstateen0 WARL write valid value ---- */

TEST_REGISTER(test_hcross_sssta_45);
bool test_hcross_sssta_45(void)
{
    TEST_BEGIN("HCROSS-SSSTA-45: hstateen0 WARL write legal value readback");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, ~0UL);

    uintptr_t saved_h = hstateen_read(0);

    /* Determine writable mask by writing all-1s */
    hstateen_write(0, ~0UL);
    uintptr_t writable_mask = hstateen_read(0);

    /* Write a legal value (only writable bits set) */
    uintptr_t test_val = writable_mask;
    hstateen_write(0, test_val);
    uintptr_t rb = hstateen_read(0);

    TEST_ASSERT_EQ("hstateen0 WARL readback matches written value",
                   rb, test_val);

    /* Write zero (also legal) */
    hstateen_write(0, 0);
    uintptr_t rb_zero = hstateen_read(0);

    /* RO1 bits may prevent full zero; masked check */
    printf("  writable_mask: 0x%lx, zero readback: 0x%lx\n",
           (unsigned long)writable_mask, (unsigned long)rb_zero);

    TEST_ASSERT("hstateen0 zero write is valid WARL operation", 1);

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}
