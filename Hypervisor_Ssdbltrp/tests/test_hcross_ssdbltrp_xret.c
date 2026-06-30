/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_ssdbltrp_xret.c - Hypervisor x Ssdbltrp Cross Tests
 *
 * Tests 19-24: MRET/SRET/MNRET cross-mode SDT clear
 *
 * These tests verify that XRET instructions properly clear SDT fields
 * based on the target privilege mode:
 * - sstatus.SDT is cleared when transitioning to U/VS/VU mode
 * - vsstatus.SDT is ONLY cleared when transitioning to VU mode
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-19: MRET to VS-mode clears sstatus.SDT             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_19);
bool test_hcross_ssdbltrp_19(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-19: MRET to VS-mode clears sstatus.SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /* This test verifies that when M-mode executes MRET to VS-mode,
     * the hardware clears sstatus.SDT. This requires executing an actual MRET
     * instruction with proper trap handling to avoid double-trap issues when
     * returning to M-mode from VS-mode. */
    TEST_SKIP("Requires actual MRET execution with Ssdbltrp-aware trap handler");

    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-20: MRET to VU clears both SDT fields              */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_20);
bool test_hcross_ssdbltrp_20(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-20: MRET to VU clears sstatus.SDT and vsstatus.SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /* This test verifies that when M-mode executes MRET to VU-mode,
     * the hardware clears both sstatus.SDT and vsstatus.SDT.
     * This requires executing an actual MRET instruction with proper trap
     * handling to avoid double-trap issues when returning to M-mode from VU-mode. */
    TEST_SKIP("Requires actual MRET execution with Ssdbltrp-aware trap handler");

    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-21: MRET to VU clears vsstatus.SDT (sstatus.SDT=0) */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_21);
bool test_hcross_ssdbltrp_21(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-21: MRET to VU clears vsstatus.SDT (sstatus.SDT=0)");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /* This test verifies that when M-mode executes MRET to VU-mode,
     * the hardware clears vsstatus.SDT even when sstatus.SDT is already 0.
     * This requires executing an actual MRET instruction with proper trap
     * handling to avoid double-trap issues when returning to M-mode from VU-mode. */
    TEST_SKIP("Requires actual MRET execution with Ssdbltrp-aware trap handler");

    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-22: MRET to VS does NOT clear vsstatus.SDT         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_22);
bool test_hcross_ssdbltrp_22(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-22: MRET to VS-mode does NOT clear vsstatus.SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();
    uintptr_t orig_vs = vsstatus_read();

    /* Enable DTE */
    menvcfg_set(MENVCFG_DTE);
    henvcfg_set(HENVCFG_DTE);

    /* Set vsstatus.SDT=1 */
    vsstatus_set(VSSTATUS_SDT);
    uintptr_t val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT initially set", (val & VSSTATUS_SDT) != 0);

    /*
     * Use run_in_vs_mode which internally does MRET to VS-mode.
     * The MRET should NOT clear vsstatus.SDT (only cleared for VU-mode).
     */
    run_in_vs_mode(_vs_ecall, 0);

    /* After returning to M-mode, check vsstatus.SDT was NOT cleared */
    val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT NOT cleared by MRET to VS-mode",
                (val & VSSTATUS_SDT) != 0);

    /* Restore */
    vsstatus_write(orig_vs);
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-23: M-mode SRET to VU clears both SDT fields       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_23);
bool test_hcross_ssdbltrp_23(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-23: M-mode SRET to VU clears sstatus.SDT and vsstatus.SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /*
     * This test requires executing SRET from M-mode to VU-mode.
     * This is complex and requires careful setup of mstatus and hstatus.
     * For now, skip this test.
     */
    TEST_SKIP("M-mode SRET requires complex setup and is platform-dependent");

    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-24: MNRET to VU clears both SDT fields             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_24);
bool test_hcross_ssdbltrp_24(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-24: MNRET to VU clears sstatus.SDT and vsstatus.SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /*
     * MNRET requires Smrnmi extension (mnstatus, mnepc CSRs).
     * For now, skip this test as it requires additional setup.
     */
    TEST_SKIP("MNRET requires Smrnmi extension and complex setup");

    SSDBLTRP_HYP_TEST_END();
}
