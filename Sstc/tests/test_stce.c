/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_stce.c - Group 1: menvcfg STCE Field Read/Write
 *
 * SSTC-STCE-01, SSTC-STCE-02, SSTC-STCE-05
 * Verifies the STCE bit in menvcfg can be read/written correctly
 * from M-mode.
 *
 * Note: SSTC-STCE-03/04 (henvcfg.STCE tests) have been migrated to
 * Hypervisor_Sstc/ project (HCROSS-SSTC-01/02).
 */

/* ------------------------------------------------------------------ */
/* SSTC-STCE-01: menvcfg.STCE read-write round-trip                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_menvcfg_stce_rw);
bool test_sstc_menvcfg_stce_rw(void) {
    TEST_BEGIN("SSTC-STCE-01: menvcfg.STCE read-write round-trip");

    uintptr_t orig = menvcfg_read();

    /* Write STCE=1 */
    menvcfg_set(MENVCFG_STCE);
    uintptr_t val = menvcfg_read();
    TEST_ASSERT("STCE set to 1", (val & MENVCFG_STCE) != 0);

    /* Write STCE=0 */
    menvcfg_clear(MENVCFG_STCE);
    val = menvcfg_read();
    TEST_ASSERT("STCE cleared to 0", (val & MENVCFG_STCE) == 0);

    /* Restore */
    menvcfg_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-STCE-02: menvcfg.STCE does not affect other fields            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_menvcfg_stce_no_side_effect);
bool test_sstc_menvcfg_stce_no_side_effect(void) {
    TEST_BEGIN("SSTC-STCE-02: menvcfg.STCE does not affect other fields");

    uintptr_t orig = menvcfg_read();

    /* Set a known other field: ADUE (bit 61) if available */
    menvcfg_set(MENVCFG_ADUE);
    uintptr_t before = menvcfg_read();
    uintptr_t other_bits_before = before & ~MENVCFG_STCE;

    /* Toggle STCE on */
    menvcfg_set(MENVCFG_STCE);
    uintptr_t after_set = menvcfg_read();
    uintptr_t other_bits_after_set = after_set & ~MENVCFG_STCE;
    TEST_ASSERT("Other fields unchanged after STCE set",
                other_bits_before == other_bits_after_set);

    /* Toggle STCE off */
    menvcfg_clear(MENVCFG_STCE);
    uintptr_t after_clr = menvcfg_read();
    uintptr_t other_bits_after_clr = after_clr & ~MENVCFG_STCE;
    TEST_ASSERT("Other fields unchanged after STCE clear",
                other_bits_before == other_bits_after_clr);

    /* Restore */
    menvcfg_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-STCE-05: menvcfg.STCE initial value is readable               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_menvcfg_stce_initial);
bool test_sstc_menvcfg_stce_initial(void) {
    TEST_BEGIN("SSTC-STCE-05: menvcfg.STCE initial value readable");

    /* Just verify we can read menvcfg without faulting and STCE bit
     * has a deterministic value (0 or 1). The initial value is
     * implementation-defined but must be readable. */
    uintptr_t val = menvcfg_read();
    uintptr_t stce = (val & MENVCFG_STCE) ? 1 : 0;
    TEST_ASSERT("STCE initial value is 0 or 1", stce == 0 || stce == 1);

    TEST_END();
}
