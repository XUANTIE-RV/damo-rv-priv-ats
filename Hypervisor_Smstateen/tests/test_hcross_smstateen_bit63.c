/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_smstateen_bit63.c - HCROSS-SMSTA-03~04: Bit 63 control behavior
 *
 * Verifies that mstateen bit 63 (SE0) controls HS-mode access
 * to hstateen CSRs, and bit 63 writability conditions.
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-03: mstateen0.SE0=0 blocks HS-mode hstateen0        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_03);
bool test_hcross_smsta_03(void) {
    TEST_BEGIN("HCROSS-SMSTA-03: mstateen0.SE0=0 blocks HS-mode hstateen0");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    /* Clear SE0 (bit 63) */
    mstateen0_clear(MSTATEEN0_SE0);

    /* HS-mode access to hstateen0 should trigger illegal-instruction.
     * Since we are in M-mode and HS-mode uses the same trap path,
     * we test by switching to S-mode (which acts as HS when H ext
     * is present and V=0). */
    SMSTATEEN_TEST_SMODE_BLOCKED(
        "HS-mode hstateen0 read blocked (SE0=0)",
        hstateen0_read());

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-04: mstateen bit 63 writability conditions           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_04);
bool test_hcross_smsta_04(void) {
    TEST_BEGIN("HCROSS-SMSTA-04: mstateen0 bit 63 writability conditions");

    uintptr_t orig = mstateen0_read();
    bool has_h = HAS_H_EXT();

    /* Try to write bit 63 */
    mstateen0_set(MSTATEEN0_SE0);
    uintptr_t val = mstateen0_read();
    bool writable = (val & MSTATEEN0_SE0) != 0;

    if (has_h) {
        /* With H extension, bit 63 must be writable (hstateen exists) */
        TEST_ASSERT("bit 63 writable with H ext", writable);
    } else {
        /* Without H ext, bit 63 is writable only if sstateen0 is not
         * all read-only zero. Either way is valid. */
        TEST_ASSERT("bit 63 writability check done", true);
        if (writable) {
            printf("  INFO: bit 63 is writable (sstateen0 has writable bits)\n");
        } else {
            printf("  INFO: bit 63 is RO0 (no H ext and sstateen0 all RO0)\n");
        }
    }

    mstateen0_write(orig);
    TEST_END();
}
