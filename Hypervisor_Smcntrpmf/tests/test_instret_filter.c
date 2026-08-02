/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_instret_filter.c - Group 16: VSINH/VUINH instret inhibition
 *
 * PMF-INS-06: VSINH=1 inhibits VS-mode instret counting.
 * PMF-INS-07: VUINH=1 inhibits VU-mode instret counting.
 *
 * Migrated from Smcntrpmf_test_plan.md Group 3 (PMF-INS-06/07).
 *
 * Strategy: inhibit M-mode (MINH=1) so M-mode setup/teardown does not
 * contribute to minstret, then run a NOP loop in VS/VU-mode via the
 * run_in_vs_mode()/run_in_vu_mode() trampoline and measure the minstret
 * delta. With VSINH/VUINH set the delta must be zero.
 */

/* ------------------------------------------------------------------ */
/* PMF-INS-06: VSINH=1 inhibits VS-mode instret counting             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_06_vsinh_inhibit);
bool test_pmf_ins_06_vsinh_inhibit(void)
{
    TEST_BEGIN("PMF-INS-06: VSINH=1 inhibits VS-mode instret");

    if (!HAS_H_EXT())
        TEST_SKIP("H extension not available");
    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* Sanity: with no inhibition, VS-mode execution advances minstret. */
    minstretcfg_write(CYCLECFG_MINH);
    uint64_t base = read_minstret();
    run_in_vs_mode(_v_exec_nops, 500);
    uint64_t after = read_minstret();
    TEST_ASSERT("VS-mode instret counts when VSINH=0", after > base);

    /* Inhibit VS-mode: the same workload must not advance minstret. */
    minstretcfg_write(CYCLECFG_MINH | CYCLECFG_VSINH);
    uint64_t start = read_minstret();
    run_in_vs_mode(_v_exec_nops, 500);
    uint64_t end = read_minstret();
    TEST_ASSERT("VS-mode instret inhibited (VSINH=1)", end == start);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-INS-07: VUINH=1 inhibits VU-mode instret counting             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_ins_07_vuinh_inhibit);
bool test_pmf_ins_07_vuinh_inhibit(void)
{
    TEST_BEGIN("PMF-INS-07: VUINH=1 inhibits VU-mode instret");

    if (!HAS_H_EXT())
        TEST_SKIP("H extension not available");
    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");
    if (!instret_counter_functional())
        TEST_SKIP("instret counter not functional");

    /* Sanity: with no inhibition, VU-mode execution advances minstret. */
    minstretcfg_write(CYCLECFG_MINH);
    uint64_t base = read_minstret();
    run_in_vu_mode(_v_exec_nops, 500);
    uint64_t after = read_minstret();
    TEST_ASSERT("VU-mode instret counts when VUINH=0", after > base);

    /* Inhibit VU-mode: the same workload must not advance minstret. */
    minstretcfg_write(CYCLECFG_MINH | CYCLECFG_VUINH);
    uint64_t start = read_minstret();
    run_in_vu_mode(_v_exec_nops, 500);
    uint64_t end = read_minstret();
    TEST_ASSERT("VU-mode instret inhibited (VUINH=1)", end == start);

    minstretcfg_write(0);
    TEST_END();
}
