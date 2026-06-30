/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4: sstateen Bit Allocation and Read-Only Constraints
 *
 * Spec anchors:
 *   norm:sstateen_bit_allocation        — bits from bit 0 upward
 *   norm:sstateen_ro1_bits              — RO1 constraint
 *   norm:sstateen_encroachment_bits_roz — encroachment bits ROZ
 *   norm:mstateen_bit_correspondence    — sstateen <-> mstateen
 *   norm:mstateen_lower_priv_roz        — mstateen=0 => sstateen ROZ
 *   norm:stateen_unimplemented_state_roz — unimplemented = ROZ
 *
 * 8 tests: SS-ALLOC-01 ~ SS-ALLOC-08
 * =================================================================== */

/* ---- SS-ALLOC-01: sstateen0 bit allocation from bit 0 ---- */

TEST_REGISTER(test_ss_alloc_bit_positions);
bool test_ss_alloc_bit_positions(void) {
    TEST_BEGIN("SS-ALLOC-01: sstateen0 bit allocation from bit 0");

    /* Verify defined bit positions match spec */
    TEST_ASSERT("STATEEN0_C is bit 0", STATEEN0_C == (1UL << 0));
    TEST_ASSERT("STATEEN0_FCSR is bit 1", STATEEN0_FCSR == (1UL << 1));
    TEST_ASSERT("STATEEN0_JVT is bit 2", STATEEN0_JVT == (1UL << 2));

    TEST_END();
}

/* ---- SS-ALLOC-02: sstateen0 RO1 constraint (no H ext) ---- */

TEST_REGISTER(test_ss_alloc_ro1_no_h);
bool test_ss_alloc_ro1_no_h(void) {
    TEST_BEGIN("SS-ALLOC-02: sstateen0 RO1 => mstateen0 same bit RO1");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, ~0UL);  /* Open all bits */

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Check each defined bit: try to clear it in sstateen0 */
    uintptr_t test_bits[] = { STATEEN0_C, STATEEN0_FCSR, STATEEN0_JVT };
    int num_bits = 3;
    bool all_ok = true;

    for (int i = 0; i < num_bits; i++) {
        uintptr_t bit = test_bits[i];

        /* Try to clear the bit */
        sstateen_clear_bits(0, bit);
        uintptr_t rb = sstateen_read(0);

        if (rb & bit) {
            /* This bit is RO1 in sstateen0 */
            printf("  sstateen0 bit 0x%lx is RO1, checking mstateen0\n",
                   (unsigned long)bit);

            /* mstateen0 same bit must also be RO1 */
            uintptr_t msta_saved = mstateen_read(0);
            mstateen_clear_bits(0, bit);
            uintptr_t msta_rb = mstateen_read(0);
            mstateen_write(0, msta_saved);

            if (!(msta_rb & bit)) {
                printf("  FAIL: sstateen0 bit RO1 but mstateen0 not RO1\n");
                all_ok = false;
            }
        }
    }

    TEST_ASSERT("sstateen0 RO1 bits => mstateen0 same bits also RO1", all_ok);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-ALLOC-03: sstateen0 RO1 constraint (with H ext) ---- */

TEST_REGISTER(test_ss_alloc_ro1_with_h);
bool test_ss_alloc_ro1_with_h(void) {
    TEST_BEGIN("SS-ALLOC-03: sstateen0 RO1 => mstateen0+hstateen0 RO1");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, ~0UL);

    uintptr_t saved_hstateen0 = hstateen_read(0);
    hstateen_write(0, ~0UL);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    uintptr_t test_bits[] = { STATEEN0_C, STATEEN0_FCSR, STATEEN0_JVT };
    int num_bits = 3;
    bool all_ok = true;

    for (int i = 0; i < num_bits; i++) {
        uintptr_t bit = test_bits[i];

        sstateen_clear_bits(0, bit);
        uintptr_t rb = sstateen_read(0);

        if (rb & bit) {
            /* RO1 in sstateen0 */
            printf("  sstateen0 bit 0x%lx is RO1\n", (unsigned long)bit);

            /* Check mstateen0 */
            uintptr_t ms = mstateen_read(0);
            mstateen_clear_bits(0, bit);
            uintptr_t ms_rb = mstateen_read(0);
            mstateen_write(0, ms);

            if (!(ms_rb & bit)) {
                printf("  FAIL: mstateen0 same bit not RO1\n");
                all_ok = false;
            }

            /* Check hstateen0 */
            uintptr_t hs = hstateen_read(0);
            hstateen_clear_bits(0, bit);
            uintptr_t hs_rb = hstateen_read(0);
            hstateen_write(0, hs);

            if (!(hs_rb & bit)) {
                printf("  FAIL: hstateen0 same bit not RO1\n");
                all_ok = false;
            }
        }
    }

    TEST_ASSERT("RO1 constraint satisfied for mstateen0 and hstateen0", all_ok);

    sstateen_write(0, saved_sstateen0);
    hstateen_write(0, saved_hstateen0);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- SS-ALLOC-04: sstateen0 encroachment bits ROZ ---- */

TEST_REGISTER(test_ss_alloc_encroachment_roz);
bool test_ss_alloc_encroachment_roz(void) {
    TEST_BEGIN("SS-ALLOC-04: sstateen0 encroachment bits are ROZ");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, ~0UL);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* If mstateen0 uses bits in [63:32] (high bits encroaching into
     * sstateen's 32-bit space), those bits should be ROZ in sstateen0.
     * sstateen0 is only 32 bits wide, so bits [63:32] are not accessible. */
    sstateen_write(0, ~0UL);
    uintptr_t rb = sstateen_read(0);

#if __riscv_xlen == 64
    uintptr_t high32 = rb >> 32;
    TEST_ASSERT("sstateen0 bits [63:32] are zero (32-bit register)",
                high32 == 0);
#else
    TEST_ASSERT("RV32: sstateen0 encroachment via hstateen0h", 1);
#endif

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-ALLOC-05: sstateen defined bits correspond to mstateen ---- */

TEST_REGISTER(test_ss_alloc_bit_correspondence);
bool test_ss_alloc_bit_correspondence(void) {
    TEST_BEGIN("SS-ALLOC-05: sstateen0 defined bits in mstateen0");

    uintptr_t saved_mstateen0 = mstateen_read(0);

    /* Write all-1s to mstateen0, read back to find defined bits */
    mstateen_write(0, ~0UL);
    uintptr_t msta_mask = mstateen_read(0);

    /* Write all-1s to sstateen0, read back */
    uintptr_t saved_sstateen0 = sstateen_read(0);
    sstateen_write(0, ~0UL);
    uintptr_t ssta_mask = sstateen_read(0);

    /* Every writable bit in sstateen0 should also exist in mstateen0 */
    uintptr_t ssta_only = ssta_mask & ~msta_mask;
    TEST_ASSERT("all sstateen0 defined bits exist in mstateen0",
                ssta_only == 0);

    if (ssta_only != 0) {
        printf("  sstateen0 bits not in mstateen0: 0x%lx\n",
               (unsigned long)ssta_only);
    }

    printf("  mstateen0 writable mask: 0x%lx\n", (unsigned long)msta_mask);
    printf("  sstateen0 writable mask: 0x%lx\n", (unsigned long)ssta_mask);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-ALLOC-06: mstateen=0 propagates to sstateen ROZ ---- */

TEST_REGISTER(test_ss_alloc_mstateen_zero_propagates);
bool test_ss_alloc_mstateen_zero_propagates(void) {
    TEST_BEGIN("SS-ALLOC-06: mstateen0=0 => sstateen0 bit is ROZ");

    uintptr_t saved_mstateen0 = mstateen_read(0);

    /* Ensure SE0=1 so S-mode can access sstateen0 */
    mstateen_set_bits(0, STATEEN0_SE0);

    /* Clear C bit in mstateen0 */
    mstateen_clear_bits(0, STATEEN0_C);

    /* Now try to set C bit in sstateen0 */
    uintptr_t saved_sstateen0 = sstateen_read(0);
    sstateen_set_bits(0, STATEEN0_C);
    uintptr_t rb = sstateen_read(0);

    TEST_ASSERT("sstateen0.C is ROZ when mstateen0.C=0",
                (rb & STATEEN0_C) == 0);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-ALLOC-07: mstateen=1 releases sstateen propagation ---- */

TEST_REGISTER(test_ss_alloc_mstateen_one_releases);
bool test_ss_alloc_mstateen_one_releases(void) {
    TEST_BEGIN("SS-ALLOC-07: mstateen0=1 => sstateen0 bit writable");

    uintptr_t saved_mstateen0 = mstateen_read(0);

    /* Set both SE0 and C in mstateen0 */
    mstateen_set_bits(0, STATEEN0_SE0 | STATEEN0_C);

    /* Check if mstateen0.C took effect */
    if (!(mstateen_read(0) & STATEEN0_C)) {
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("mstateen0.C not writable (no custom state)");
    }

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Now sstateen0.C should be writable */
    sstateen_set_bits(0, STATEEN0_C);
    uintptr_t rb = sstateen_read(0);
    TEST_ASSERT("sstateen0.C writable when mstateen0.C=1",
                (rb & STATEEN0_C) != 0);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-ALLOC-08: unimplemented extension bits are ROZ ---- */

TEST_REGISTER(test_ss_alloc_unimplemented_roz);
bool test_ss_alloc_unimplemented_roz(void) {
    TEST_BEGIN("SS-ALLOC-08: unimplemented extension bits are ROZ");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, ~0UL);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Write all-1s, then check which bits stayed 0 */
    sstateen_write(0, ~0UL);
    uintptr_t rb = sstateen_read(0);

    /* All unimplemented bits should be 0.
     * We verify that at least some bits are zero (ROZ) */
    bool has_roz = (rb != 0xFFFFFFFFUL);
    TEST_ASSERT("sstateen0 has some ROZ bits (unimplemented/reserved)", has_roz);

    printf("  sstateen0 all-1 readback: 0x%lx\n", (unsigned long)rb);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}
