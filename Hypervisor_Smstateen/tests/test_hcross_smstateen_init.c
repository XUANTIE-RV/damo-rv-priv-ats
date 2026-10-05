/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_smstateen_init.c - HCROSS-SMSTA-01: hstateen0 initialization
 *
 * Verifies that M-mode software can initialize hstateen0 to zero
 * after modifying mstateen0.
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-01: hstateen0 zeroed after mstateen0 modification    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_01);
bool test_hcross_smsta_01(void) {
    TEST_BEGIN("HCROSS-SMSTA-01: hstateen0 zeroed after mstateen0 modification");

    if (!H_AVAILABLE) TEST_SKIP("H extension not available");

    /* Set mstateen0.SE0 to allow hstateen0 access, plus some bits */
    uintptr_t orig = mstateen_read(0);
    mstateen_write(0, MSTATEEN0_SE0 | MSTATEEN0_C);

    /* Write zero to hstateen0 */
    trap_expect_begin();
    hstateen_write(0, 0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped) {
        TEST_SKIP("hstateen0 not accessible");
    }

    uintptr_t val = hstateen_read(0);
    TEST_ASSERT_EQ("hstateen0 reads zero", val, 0);

    mstateen_write(0, orig);
    TEST_END();
}
