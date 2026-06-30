/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_smstateen_prop.c - HCROSS-SMSTA-02: RO0 propagation to hstateen0
 *
 * Verifies that bits which are zero in mstateen propagate as
 * read-only zero to hstateen when H ext is present.
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-02: mstateen0 zero bit propagates to hstateen0       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_02);
bool test_hcross_smsta_02(void) {
    TEST_BEGIN("HCROSS-SMSTA-02: mstateen0 zero bit propagates to hstateen0");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    /* Enable hstateen0 access (SE0=1) but clear C bit */
    mstateen0_write(MSTATEEN0_SE0);

    /* From M-mode, try writing C bit to hstateen0 */
    hstateen0_write(MSTATEEN0_C);
    uintptr_t val = hstateen0_read();

    TEST_ASSERT_BITS("hstateen0.C is RO0 when mstateen0.C=0",
                     val, MSTATEEN0_C, 0);

    mstateen0_write(orig);
    TEST_END();
}
