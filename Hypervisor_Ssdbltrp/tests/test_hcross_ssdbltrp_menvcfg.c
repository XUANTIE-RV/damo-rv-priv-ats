/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_ssdbltrp_menvcfg.c - Hypervisor x Ssdbltrp Cross Tests
 *
 * Tests 17-18: menvcfg.DTE control
 *
 * These tests verify that menvcfg.DTE=0 disables double-trap
 * functionality globally, making both henvcfg.DTE and vsstatus.SDT
 * read-only zero.
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-17: menvcfg.DTE=0, vsstatus.SDT read-only zero     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_17);
bool test_hcross_ssdbltrp_17(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-17: menvcfg.DTE=0, vsstatus.SDT read-only zero");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();
    uintptr_t orig_vs = vsstatus_read();

    /* Disable menvcfg.DTE (global disable) */
    menvcfg_clear(MENVCFG_DTE);

    /* Try to set henvcfg.DTE - should be read-only zero */
    henvcfg_set(HENVCFG_DTE);
    uintptr_t val = henvcfg_read_csr();
    TEST_ASSERT("henvcfg.DTE read-only zero when menvcfg.DTE=0",
                (val & HENVCFG_DTE) == 0);

    /* Try to set vsstatus.SDT from VS-mode */
    run_in_vs_mode(_vs_set_vsstatus_sdt, 0);

    /* Read back vsstatus.SDT - should be 0 (read-only zero) */
    val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT read-only zero when menvcfg.DTE=0",
                (val & VSSTATUS_SDT) == 0);

    /* Restore */
    vsstatus_write(orig_vs);
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-18: menvcfg.DTE=0, henvcfg.DTE read-only zero      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_18);
bool test_hcross_ssdbltrp_18(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-18: menvcfg.DTE=0, henvcfg.DTE read-only zero");

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

    /* Try to write henvcfg.DTE directly - should still be read-only zero */
    henvcfg_write_csr(HENVCFG_DTE);
    val = henvcfg_read_csr();
    TEST_ASSERT("henvcfg.DTE write ignored when menvcfg.DTE=0",
                (val & HENVCFG_DTE) == 0);

    /* Restore */
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}
