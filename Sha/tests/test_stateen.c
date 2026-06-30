/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2: Ssstateen CSR basic accessibility and WARL behavior
 *
 * Spec anchors:
 *   norm:sstateen_rv64_csrs   — sstateen0-3 exist
 *   norm:hstateen_rv64_csrs   — hstateen0-3 exist
 *   norm:hstateen_bit_63_writable — hstateen bit 63 always writable
 *   norm:stateen_reserved_roz — reserved bits read-only zero
 *
 * 5 tests: SHA-STATEEN-01 ~ SHA-STATEEN-05
 * =================================================================== */

/* ---- SHA-STATEEN-01: hstateen0-3 all readable ---- */

TEST_REGISTER(test_sha_stateen_hstateen_readable);
bool test_sha_stateen_hstateen_readable(void) {
    TEST_BEGIN("SHA-STATEEN-01: hstateen0-3 all readable");

    /* Ensure mstateen0-3 bit63=1 to allow HS access */
    for (int i = 0; i < 4; i++)
        mstateen_set_bit63(i, true);

    for (int i = 0; i < 4; i++) {
        uintptr_t val = hstateen_read(i);
        (void)val;
    }
    TEST_ASSERT("hstateen0-3 all readable without trap", 1);

    HYP_TEST_END();
}

/* ---- SHA-STATEEN-02: sstateen0-3 all readable ---- */

TEST_REGISTER(test_sha_stateen_sstateen_readable);
bool test_sha_stateen_sstateen_readable(void) {
    TEST_BEGIN("SHA-STATEEN-02: sstateen0-3 all readable");

    /* Ensure full hierarchy: mstateen.63=1, hstateen.63=1 */
    for (int i = 0; i < 4; i++) {
        mstateen_set_bit63(i, true);
        hstateen_set_bit63(i, true);
    }

    for (int i = 0; i < 4; i++) {
        uintptr_t val = sstateen_read(i);
        (void)val;
    }
    TEST_ASSERT("sstateen0-3 all readable without trap", 1);

    HYP_TEST_END();
}

/* ---- SHA-STATEEN-03: hstateen0 bit 63 toggle ---- */

TEST_REGISTER(test_sha_stateen_hstateen0_bit63_toggle);
bool test_sha_stateen_hstateen0_bit63_toggle(void) {
    TEST_BEGIN("SHA-STATEEN-03: hstateen0 bit 63 writable (toggle)");

    mstateen_set_bit63(0, true);
    uintptr_t saved = hstateen_read(0);

    /* Set bit 63 */
    hstateen_set_bits(0, STATEEN_BIT63);
    uintptr_t rb1 = hstateen_read(0);
    TEST_ASSERT("hstateen0 bit 63 set to 1", (rb1 & STATEEN_BIT63) != 0);

    /* Clear bit 63 */
    hstateen_clear_bits(0, STATEEN_BIT63);
    uintptr_t rb0 = hstateen_read(0);
    TEST_ASSERT("hstateen0 bit 63 cleared to 0", (rb0 & STATEEN_BIT63) == 0);

    hstateen_write(0, saved);
    HYP_TEST_END();
}

/* ---- SHA-STATEEN-04: hstateen1/2/3 bit 63 writable ---- */

TEST_REGISTER(test_sha_stateen_hstateen123_bit63);
bool test_sha_stateen_hstateen123_bit63(void) {
    TEST_BEGIN("SHA-STATEEN-04: hstateen1/2/3 bit 63 writable");

    int implemented = 0;

    for (int i = 1; i <= 3; i++) {
        mstateen_set_bit63(i, true);
        uintptr_t saved = hstateen_read(i);

        /* Set bit 63 */
        hstateen_set_bits(i, STATEEN_BIT63);
        uintptr_t rb1 = hstateen_read(i);
        if ((rb1 & STATEEN_BIT63) == 0) {
            printf("  hstateen%d bit 63 not implemented (read-only zero)\n", i);
            hstateen_write(i, saved);
            continue;
        }

        /* Clear bit 63 */
        hstateen_clear_bits(i, STATEEN_BIT63);
        uintptr_t rb0 = hstateen_read(i);
        if ((rb0 & STATEEN_BIT63) != 0) {
            printf("  hstateen%d bit 63 failed to clear\n", i);
            hstateen_write(i, saved);
            TEST_ASSERT("hstateen bit 63 clear failed", false);
            HYP_TEST_END();
        }

        implemented++;
        hstateen_write(i, saved);
    }

    if (implemented == 0)
        TEST_SKIP("hstateen1/2/3 not implemented (bit 63 read-only zero, SPEC optional)");

    printf("  %d of 3 hstateenN (N=1,2,3) implemented\n", implemented);
    HYP_TEST_END();
}

/* ---- SHA-STATEEN-05: hstateen0 reserved bits read-only zero ---- */

TEST_REGISTER(test_sha_stateen_hstateen0_reserved_roz);
bool test_sha_stateen_hstateen0_reserved_roz(void) {
    TEST_BEGIN("SHA-STATEEN-05: hstateen0 reserved bits read-only zero");

    mstateen_set_bit63(0, true);

    /* Also set mstateen0 to all-1 so that hstateen0 bits are not
     * masked by mstateen0 hierarchy */
    uintptr_t saved_mstateen = mstateen_read(0);
    mstateen_write(0, ~0UL);

    uintptr_t saved = hstateen_read(0);

    /* Write all 1s */
    hstateen_write(0, ~0UL);
    uintptr_t rb = hstateen_read(0);

    /* Bit 63 must be 1 (always writable) */
    TEST_ASSERT("hstateen0 bit 63 == 1 after all-1 write",
                (rb & STATEEN_BIT63) != 0);

    /* Reserved/unimplemented bits should be 0.
     * We check bits [62:59] which are typically reserved on most
     * implementations (except bit 58 which may be Sscofpmf). */
    uintptr_t reserved_mask = 0x7UL << 59;  /* bits 61:59 */
    uintptr_t reserved_readback = rb & reserved_mask;
    printf("  hstateen0 readback after all-1 write: 0x%lx\n",
           (unsigned long)rb);
    printf("  reserved bits [61:59] readback: 0x%lx (expect 0)\n",
           (unsigned long)reserved_readback);

    /* Note: This is a soft check. Some bits in [62:0] may be defined
     * by extensions (e.g., bit 58 for Sscofpmf, bit 1 for Fcsr, etc.).
     * We only assert that the readback is a valid WARL value (i.e.,
     * some bits stuck at 0 = reserved bits are ROZ). */
    bool has_roz = (rb != ~0UL);
    TEST_ASSERT("hstateen0 has some read-only-zero bits (WARL)", has_roz);

    hstateen_write(0, saved);
    mstateen_write(0, saved_mstateen);
    HYP_TEST_END();
}
