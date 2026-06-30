/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 1: hideleg[13] Writability Verification
 *
 * Tests LCFIDLG-WR-01 and LCFIDLG-WR-02 verify that hideleg bit 13
 * can be written to 1 and cleared to 0 when Shlcofideleg is implemented.
 *
 * Spec reference:
 *   norm:vsip_vsie_lcofi (R1): If Shlcofideleg is implemented,
 *   hideleg bit 13 is writable; otherwise read-only zero.
 */

/* ------------------------------------------------------------------
 * LCFIDLG-WR-01 / WR-02: hideleg[13] can be set to 1 and cleared to 0
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_hideleg_bit13_writable);
bool test_shlcofideleg_hideleg_bit13_writable(void) {
    TEST_BEGIN("LCFIDLG-WR: hideleg[13] writable");

    /* Skip if Shlcofideleg is not implemented. */
    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved = hideleg_read();

    /* WR-01: Write 1, read back. */
    hideleg_write(saved | LCOFI_BIT);
    TEST_ASSERT("hideleg[13] can be set to 1",
                (hideleg_read() & LCOFI_BIT) != 0);

    /* WR-02: Write 0, read back. */
    hideleg_write(hideleg_read() & ~LCOFI_BIT);
    TEST_ASSERT("hideleg[13] can be cleared to 0",
                (hideleg_read() & LCOFI_BIT) == 0);

    /* Restore. */
    hideleg_write(saved);
    HYP_TEST_END();
}
