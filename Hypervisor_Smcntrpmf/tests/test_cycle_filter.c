/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_cycle_filter.c - Group 16: VSINH/VUINH cycle counting inhibition
 *
 * PMF-CYC-08: VSINH=1 inhibits VS-mode cycle counting.
 * PMF-CYC-09: VUINH=1 inhibits VU-mode cycle counting.
 *
 * Migrated from Smcntrpmf_test_plan.md Group 2 (PMF-CYC-08/09).
 *
 * Strategy: inhibit M-mode (MINH=1) so the M-mode setup/teardown does
 * not contribute to mcycle, then run a NOP loop in VS/VU-mode via the
 * run_in_vs_mode()/run_in_vu_mode() trampoline and measure the mcycle
 * delta. With VSINH/VUINH set the delta must be zero.
 */

/* ------------------------------------------------------------------ */
/* PMF-CYC-08: VSINH=1 inhibits VS-mode cycle counting               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_08_vsinh_inhibit);
bool test_pmf_cyc_08_vsinh_inhibit(void)
{
    TEST_BEGIN("PMF-CYC-08: VSINH=1 inhibits VS-mode cycle counting");

    if (!HAS_H_EXT())
        TEST_SKIP("H extension not available");
    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Sanity: with no inhibition, VS-mode execution must advance mcycle.
     * MINH=1 silences M-mode so the delta reflects VS-mode only. */
    mcyclecfg_write(CYCLECFG_MINH);
    uint64_t base = read_mcycle();
    run_in_vs_mode(_v_exec_nops, 500);
    uint64_t after = read_mcycle();
    TEST_ASSERT("VS-mode cycle counts when VSINH=0", after > base);

    /* Inhibit VS-mode: the same VS-mode workload must not advance mcycle. */
    mcyclecfg_write(CYCLECFG_MINH | CYCLECFG_VSINH);
    uint64_t start = read_mcycle();
    run_in_vs_mode(_v_exec_nops, 500);
    uint64_t end = read_mcycle();
    TEST_ASSERT("VS-mode cycle inhibited (VSINH=1)", end == start);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CYC-09: VUINH=1 inhibits VU-mode cycle counting               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_cyc_09_vuinh_inhibit);
bool test_pmf_cyc_09_vuinh_inhibit(void)
{
    TEST_BEGIN("PMF-CYC-09: VUINH=1 inhibits VU-mode cycle counting");

    if (!HAS_H_EXT())
        TEST_SKIP("H extension not available");
    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!cycle_counter_functional())
        TEST_SKIP("cycle counter not functional");

    /* Sanity: with no inhibition, VU-mode execution must advance mcycle. */
    mcyclecfg_write(CYCLECFG_MINH);
    uint64_t base = read_mcycle();
    run_in_vu_mode(_v_exec_nops, 500);
    uint64_t after = read_mcycle();
    TEST_ASSERT("VU-mode cycle counts when VUINH=0", after > base);

    /* Inhibit VU-mode: the same VU-mode workload must not advance mcycle. */
    mcyclecfg_write(CYCLECFG_MINH | CYCLECFG_VUINH);
    uint64_t start = read_mcycle();
    run_in_vu_mode(_v_exec_nops, 500);
    uint64_t end = read_mcycle();
    TEST_ASSERT("VU-mode cycle inhibited (VUINH=1)", end == start);

    mcyclecfg_write(0);
    TEST_END();
}
