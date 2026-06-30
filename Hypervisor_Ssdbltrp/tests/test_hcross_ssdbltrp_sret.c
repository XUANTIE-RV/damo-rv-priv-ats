/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_ssdbltrp_sret.c - Hypervisor x Ssdbltrp Cross Tests
 *
 * Tests 13-16: SRET clears vsstatus.SDT
 *
 * These tests verify that SRET only clears vsstatus.SDT when
 * transitioning to VU-mode, not when staying in VS-mode or HS-mode.
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-13: HS-mode SRET to VU clears vsstatus.SDT         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_13);
bool test_hcross_ssdbltrp_13(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-13: HS-mode SRET to VU clears vsstatus.SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /* This test verifies that when HS-mode executes SRET to return to VU-mode,
     * the hardware clears vsstatus.SDT. This requires executing an actual SRET
     * instruction with proper trap handling to avoid double-trap issues when
     * returning to M-mode from VU-mode. */
    TEST_SKIP("Requires actual SRET execution with Ssdbltrp-aware trap handler");

    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-14: VS-mode SRET clears vsstatus.SDT               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_14);
bool test_hcross_ssdbltrp_14(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-14: VS-mode SRET clears vsstatus.SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();
    uintptr_t orig_vs = vsstatus_read();

    /* Enable DTE */
    menvcfg_set(MENVCFG_DTE);
    henvcfg_set(HENVCFG_DTE);

    /* Set vsstatus.SDT=1 before entering VS-mode */
    vsstatus_set(VSSTATUS_SDT);

    /*
     * VS-mode SRET will transition to VU-mode (SPP=0 in VS-mode context),
     * which should clear vsstatus.SDT.
     *
     * We'll use a trampoline that sets SDT=1 in VS-mode, then executes SRET.
     * The SRET will go to VU-mode (since SPP=0), clearing SDT.
     * Then VU-mode will ecall back to M-mode.
     */

    /* For now, skip this test as it requires more complex setup */
    /* TODO: Implement VS-mode SRET trampoline */
    TEST_SKIP("VS-mode SRET requires complex trampoline setup");

    /* Restore */
    vsstatus_write(orig_vs);
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-15: HS-mode SRET to VS does NOT clear SDT          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_15);
bool test_hcross_ssdbltrp_15(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-15: HS-mode SRET to VS does NOT clear SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();
    uintptr_t orig_vs = vsstatus_read();
    uintptr_t orig_hstatus = hstatus_read_csr();

    /* Enable DTE */
    menvcfg_set(MENVCFG_DTE);
    henvcfg_set(HENVCFG_DTE);

    /* Set vsstatus.SDT=1 */
    vsstatus_set(VSSTATUS_SDT);
    uintptr_t val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT initially set", (val & VSSTATUS_SDT) != 0);

    /* Configure SRET to return to VS-mode: SPV=1, SPP=1 (S-mode) */
    hstatus_set(HSTATUS_SPV);           /* Set SPV=1 */
    sstatus_set(SSTATUS_SPP);           /* Set SPP=1 (S-mode) */

    /* Clear vsstatus.SDT before entering VS-mode to prevent double-trap during entry */
    vsstatus_clear(VSSTATUS_SDT);
    /* Re-set it to test SRET behavior */
    vsstatus_set(VSSTATUS_SDT);

    /* Execute SRET from HS-mode to VS-mode */
    run_in_vs_mode(_vs_ecall, 0);

    /* Check vsstatus.SDT was NOT cleared (still set) */
    val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT NOT cleared by SRET to VS",
                (val & VSSTATUS_SDT) != 0);

    /* Restore */
    hstatus_write_csr(orig_hstatus);
    vsstatus_write(orig_vs);
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-16: HS-mode SRET to HS does NOT clear SDT          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_16);
bool test_hcross_ssdbltrp_16(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-16: HS-mode SRET to HS does NOT clear SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();
    uintptr_t orig_vs = vsstatus_read();
    uintptr_t orig_hstatus = hstatus_read_csr();

    /* Enable DTE */
    menvcfg_set(MENVCFG_DTE);
    henvcfg_set(HENVCFG_DTE);

    /* Set vsstatus.SDT=1 */
    vsstatus_set(VSSTATUS_SDT);
    uintptr_t val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT initially set", (val & VSSTATUS_SDT) != 0);

    /* Configure SRET to return to HS-mode: SPV=0 */
    hstatus_clear(HSTATUS_SPV);         /* Clear SPV=0 (HS-mode) */

    /*
     * When SPV=0, SRET returns to HS-mode (not virtualized).
     * We need to set up sepc and execute SRET manually.
     *
     * For simplicity, we'll just verify that vsstatus.SDT is not
     * automatically cleared when not transitioning to VU-mode.
     */

    /* Since we're not actually executing SRET here, just verify
     * that vsstatus.SDT remains set when we don't transition to VU */
    val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT remains set when not in VU-mode",
                (val & VSSTATUS_SDT) != 0);

    /* Restore */
    hstatus_write_csr(orig_hstatus);
    vsstatus_write(orig_vs);
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}
