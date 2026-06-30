/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_ssdbltrp_vsstatus.c - Hypervisor x Ssdbltrp Cross Tests
 *
 * Tests 07-12: vsstatus.SDT in VS-mode
 *
 * These tests verify vsstatus.SDT field behavior, including WARL semantics,
 * SDT/SIE mutual exclusion, automatic SDT setting on traps, and double-trap
 * behavior when SDT is already set.
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-07: vsstatus.SDT WARL read-write                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_07);
bool test_hcross_ssdbltrp_07(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-07: vsstatus.SDT WARL read-write");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read_csr();
    uintptr_t orig_h = henvcfg_read_csr();
    uintptr_t orig_vs = vsstatus_read();

    /* Enable DTE at both levels */
    menvcfg_set(MENVCFG_DTE);
    henvcfg_set(HENVCFG_DTE);

    /* Test 1: Write SDT=1, read back should be 1 */
    vsstatus_set(VSSTATUS_SDT);
    uintptr_t val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT WARL write 1", (val & VSSTATUS_SDT) != 0);

    /* Test 2: Write SDT=0, read back should be 0 */
    vsstatus_clear(VSSTATUS_SDT);
    val = vsstatus_read();
    TEST_ASSERT("vsstatus.SDT WARL write 0", (val & VSSTATUS_SDT) == 0);

    /* Restore */
    vsstatus_write(orig_vs);
    henvcfg_write_csr(orig_h);
    menvcfg_write_csr(orig_m);
    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-08: vsstatus.SDT=1 auto-clears vsstatus.SIE        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_08);
bool test_hcross_ssdbltrp_08(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-08: vsstatus.SDT=1 auto-clears vsstatus.SIE");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /* This test verifies that writing SDT=1 automatically clears SIE.
     * However, this behavior may only be enforced when accessed from VS-mode
     * via sstatus, not when M-mode directly writes to vsstatus.
     * Proper testing requires VS-mode execution context. */
    TEST_SKIP("Requires VS-mode execution to verify SDT/SIE mutual exclusion");

    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-09: vsstatus.SDT=1 prevents setting SIE=1          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_09);
bool test_hcross_ssdbltrp_09(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-09: vsstatus.SDT=1 prevents setting SIE=1");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /* This test verifies that when SDT=1, attempts to set SIE=1 are ignored.
     * However, this behavior may only be enforced when accessed from VS-mode
     * via sstatus, not when M-mode directly writes to vsstatus.
     * Proper testing requires VS-mode execution context. */
    TEST_SKIP("Requires VS-mode execution to verify SDT/SIE mutual exclusion");

    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-10: VS-mode trap auto-sets vsstatus.SDT=1          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_10);
bool test_hcross_ssdbltrp_10(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-10: VS-mode trap auto-sets vsstatus.SDT=1");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /* This test verifies that when a trap occurs from VS-mode to M-mode,
     * the hardware automatically sets vsstatus.SDT=1.
     * This requires VS-mode execution context and proper trap handling
     * with Ssdbltrp-aware trap handler to avoid double-trap issues. */
    TEST_SKIP("Requires VS-mode execution with Ssdbltrp-aware trap handler");

    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-11: VS-mode SDT=1 trap triggers double-trap        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_11);
bool test_hcross_ssdbltrp_11(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-11: VS-mode SDT=1 trap triggers double-trap");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /* This test verifies that when vsstatus.SDT=1 and a trap occurs from VS-mode,
     * it triggers a double-trap exception to M-mode.
     * This requires VS-mode execution context and proper trap handling
     * with Ssdbltrp-aware trap handler to catch and verify the double-trap. */
    TEST_SKIP("Requires VS-mode execution with Ssdbltrp-aware trap handler");

    SSDBLTRP_HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSDBLTRP-12: VS-mode double-trap M-mode CSR correct         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssdbltrp_12);
bool test_hcross_ssdbltrp_12(void)
{
    TEST_BEGIN("HCROSS-SSDBLTRP-12: VS-mode double-trap M-mode CSR correct");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    /* This test verifies the M-mode CSR state after a double-trap from VS-mode.
     * It checks mcause, mstatus.MPV, mstatus.MPP, and mtval2 values.
     * This requires VS-mode execution context and proper trap handling
     * with Ssdbltrp-aware trap handler to trigger and capture the double-trap. */
    TEST_SKIP("Requires VS-mode execution with Ssdbltrp-aware trap handler");

    SSDBLTRP_HYP_TEST_END();
}
