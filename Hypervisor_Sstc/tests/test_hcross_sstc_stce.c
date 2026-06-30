/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_sstc_stce.c - Group 9.1: henvcfg.STCE Field Control
 *
 * HCROSS-SSTC-01: henvcfg.STCE read-write round-trip
 * HCROSS-SSTC-02: henvcfg.STCE constrained by menvcfg.STCE
 *
 * Migrated from sstc_test_plan.md Group 1 (SSTC-STCE-03/04).
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-01: henvcfg.STCE read-write round-trip                 */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_01);
bool test_hcross_sstc_01(void)
{
    TEST_BEGIN("HCROSS-SSTC-01: henvcfg.STCE read-write round-trip");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig_m = menvcfg_read();
    uintptr_t orig_h = henvcfg_read();

    /* menvcfg.STCE must be 1 for henvcfg.STCE to be writable */
    menvcfg_set(MENVCFG_STCE);

    /* Write henvcfg.STCE=1 */
    henvcfg_set(HENVCFG_STCE);
    uintptr_t val = henvcfg_read();
    TEST_ASSERT("henvcfg.STCE set to 1", (val & HENVCFG_STCE) != 0);

    /* Write henvcfg.STCE=0 */
    henvcfg_clear(HENVCFG_STCE);
    val = henvcfg_read();
    TEST_ASSERT("henvcfg.STCE cleared to 0", (val & HENVCFG_STCE) == 0);

    /* Restore */
    henvcfg_write(orig_h);
    menvcfg_write(orig_m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-02: henvcfg.STCE constrained by menvcfg.STCE          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_02);
bool test_hcross_sstc_02(void)
{
    TEST_BEGIN("HCROSS-SSTC-02: henvcfg.STCE constrained by menvcfg.STCE");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig_m = menvcfg_read();
    uintptr_t orig_h = henvcfg_read();

    /* Ensure menvcfg.STCE=0 */
    menvcfg_clear(MENVCFG_STCE);

    /* Try to set henvcfg.STCE=1 -- should be ignored */
    henvcfg_set(HENVCFG_STCE);
    uintptr_t val = henvcfg_read();
    TEST_ASSERT("henvcfg.STCE remains 0 when menvcfg.STCE=0",
                (val & HENVCFG_STCE) == 0);

    /* Restore */
    henvcfg_write(orig_h);
    menvcfg_write(orig_m);
    TEST_END();
}
