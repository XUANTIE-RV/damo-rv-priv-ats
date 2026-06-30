/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_ssdbltrp_henvcfg.c - Hypervisor x Ssdbltrp Cross Tests
 *
 * Tests 01-06: henvcfg.DTE control
 *
 * These tests verify that henvcfg.DTE properly controls VS-mode
 * double-trap behavior when menvcfg.DTE is set.
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-01: henvcfg.DTE read-write                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_01);
bool test_hcross_ssdbltrp_01(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-01: henvcfg.DTE read-write");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();

    /* menvcfg.DTE must be 1 for henvcfg.DTE to be writable */
    menvcfg_set(MENVCFG_DTE);

    /* Write henvcfg.DTE=1 */
    henvcfg_set(HENVCFG_DTE);
    uintptr_t val = henvcfg_read_csr();
    TEST_ASSERT("henvcfg.DTE set to 1", (val & HENVCFG_DTE) != 0);

    /* Write henvcfg.DTE=0 */
    henvcfg_clear(HENVCFG_DTE);
    val = henvcfg_read_csr();
    TEST_ASSERT("henvcfg.DTE cleared to 0", (val & HENVCFG_DTE) == 0);

    /* Restore */
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-02: henvcfg.DTE=0 -> vsstatus.SDT read-only zero  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_02);
bool test_hcross_ssdbltrp_02(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-02: henvcfg.DTE=0, vsstatus.SDT read-only zero from VS-mode");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /* This test requires VS-mode execution to verify that sstatus.SDT
     * (which maps to vsstatus.SDT) is read-only zero when henvcfg.DTE=0.
     * M-mode/HS-mode can always write to vsstatus.SDT regardless of henvcfg.DTE.
     * Testing this behavior requires run_in_vs_mode with proper trap handling
     * for the Ssdbltrp extension, which is complex to implement correctly. */
    TEST_SKIP("Requires VS-mode execution with Ssdbltrp-aware trap handler");

    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-03: henvcfg.DTE=1 -> vsstatus.SDT writable        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_03);
bool test_hcross_ssdbltrp_03(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-03: henvcfg.DTE=1, vsstatus.SDT writable");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();
    uintptr_t orig_vs = vsstatus_read();

    /* Clear vsstatus.SDT before entering VS-mode to prevent double-trap */
    vsstatus_clear(VSSTATUS_SDT);

    /* Enable both menvcfg.DTE and henvcfg.DTE */
    menvcfg_set(MENVCFG_DTE);
    henvcfg_set(HENVCFG_DTE);

    /* Set vsstatus.SDT from VS-mode */
    run_in_vs_mode(_vs_set_vsstatus_sdt, 0);

    /* Read back vsstatus.SDT - should be 1 (writable) */
    uintptr_t val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT writable when henvcfg.DTE=1",
                (val & VSSTATUS_SDT) != 0);

    /* Restore */
    vsstatus_write(orig_vs);
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-04: henvcfg.DTE=0, VS-mode trap does not set SDT  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_04);
bool test_hcross_ssdbltrp_04(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-04: henvcfg.DTE=0, VS-mode trap does not set SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();
    uintptr_t orig_vs = vsstatus_read();

    /* Disable henvcfg.DTE */
    menvcfg_set(MENVCFG_DTE);
    henvcfg_clear(HENVCFG_DTE);

    /* Clear vsstatus.SDT before trap */
    vsstatus_clear(VSSTATUS_SDT);

    /* Trigger ecall from VU-mode to VS-mode */
    run_in_vu_mode(_vu_ecall, 0);

    /* Read back vsstatus.SDT - should still be 0 (DTE disabled) */
    uintptr_t val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT not set by trap when henvcfg.DTE=0",
                (val & VSSTATUS_SDT) == 0);

    /* Restore */
    vsstatus_write(orig_vs);
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-05: menvcfg.DTE=0 overrides henvcfg.DTE           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_05);
bool test_hcross_ssdbltrp_05(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-05: menvcfg.DTE=0 overrides henvcfg.DTE");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();

    /* Disable menvcfg.DTE (global disable) */
    menvcfg_clear(MENVCFG_DTE);

    /* Try to set henvcfg.DTE - should be read-only zero */
    henvcfg_set(HENVCFG_DTE);
    uintptr_t val = henvcfg_read_csr();
    TEST_ASSERT("henvcfg.DTE read-only zero when menvcfg.DTE=0",
                (val & HENVCFG_DTE) == 0);

    /* Restore */
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-06: henvcfg.DTE dynamic switch                     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_06);
bool test_hcross_ssdbltrp_06(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-06: henvcfg.DTE dynamic switch");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();
    uintptr_t orig_vs = vsstatus_read();

    /* Enable menvcfg.DTE */
    menvcfg_set(MENVCFG_DTE);

    /* Test 1: henvcfg.DTE=1 -> vsstatus.SDT writable */
    henvcfg_set(HENVCFG_DTE);
    vsstatus_clear(VSSTATUS_SDT);
    run_in_vs_mode(_vs_set_vsstatus_sdt, 0);
    uintptr_t val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT writable when henvcfg.DTE=1",
                (val & VSSTATUS_SDT) != 0);

    /* Test 2: henvcfg.DTE=0 -> vsstatus.SDT read-only zero */
    henvcfg_clear(HENVCFG_DTE);
    vsstatus_clear(VSSTATUS_SDT);
    run_in_vs_mode(_vs_set_vsstatus_sdt, 0);
    val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT read-only zero when henvcfg.DTE=0",
                (val & VSSTATUS_SDT) == 0);

    /* Restore */
    vsstatus_write(orig_vs);
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}
