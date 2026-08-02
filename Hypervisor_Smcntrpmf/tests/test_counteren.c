/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_counteren.c - Group 16: hcounteren interaction
 *
 * PMF-CTR-04:   hcounteren.CY=0, VS-mode read cycle -> virtual-inst.
 * HCROSS-PMF-01: VSINH inhibition orthogonal to hcounteren access.
 *
 * Migrated from Smcntrpmf_test_plan.md Group 7 (PMF-CTR-04) plus the
 * new orthogonality case HCROSS-PMF-01.
 */

/* ------------------------------------------------------------------ */
/* PMF-CTR-04: hcounteren.CY=0, VS-mode cannot read cycle            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ctr_04_hcounteren_cy0);
bool test_pmf_ctr_04_hcounteren_cy0(void)
{
    TEST_BEGIN("PMF-CTR-04: hcounteren.CY=0, VS-mode cannot read cycle");

    if (!HAS_H_EXT())
        TEST_SKIP("H extension not available");
    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    /* mcounteren.CY=1 (M permits), hcounteren.CY=0 (HS blocks VS/VU). */
    mcounteren_set(COUNTEREN_CY);
    hcounteren_clear(COUNTEREN_CY);

    /* VS-mode read of cycle must raise virtual-instruction (cause=22). */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_cycle, 0);
    TEST_ASSERT("trap triggered on VS cycle read", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("virtual-instruction cause",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();

    /* Restore: re-enable VS-mode counter access. */
    hcounteren_set(COUNTEREN_CY);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-PMF-01: VSINH inhibition orthogonal to hcounteren          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_pmf_01_orthogonal);
bool test_hcross_pmf_01_orthogonal(void)
{
    TEST_BEGIN("HCROSS-PMF-01: VSINH inhibition orthogonal to hcounteren");

    if (!HAS_H_EXT())
        TEST_SKIP("H extension not available");
    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Access fully permitted: mcounteren.CY=1, hcounteren.CY=1. */
    mcounteren_set(COUNTEREN_CY);
    hcounteren_set(COUNTEREN_CY);

    /* Counting inhibited for VS-mode (and M-mode to isolate the delta). */
    mcyclecfg_write(CYCLECFG_MINH | CYCLECFG_VSINH);

    /* (a) VS-mode can READ cycle (no trap) because access is permitted. */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_cycle, 0);
    TEST_ASSERT("VS cycle read permitted (no trap)", !trap_was_triggered());
    trap_expect_end();

    /* (b) ...but the value does NOT advance because VSINH=1 inhibits
     * counting. MINH=1 silences M-mode so the delta is VS-mode only. */
    uint64_t start = read_mcycle();
    run_in_vs_mode(_v_exec_nops, 500);
    uint64_t end = read_mcycle();
    TEST_ASSERT("VS cycle readable but not incrementing", end == start);

    mcyclecfg_write(0);
    TEST_END();
}
