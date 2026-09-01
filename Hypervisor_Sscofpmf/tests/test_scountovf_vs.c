/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_scountovf_vs.c - Group 10.1: VS-mode scountovf dual gating
 *
 * Tests HCROSS-SSCOFPMF-01 through HCROSS-SSCOFPMF-03
 * (migrated from COFPMF-SOV-08/09/10 in Sscofpmf_test_plan.md).
 *
 * norm:scountovf_vsmode_read_access:
 *   In VS-mode, scountovf bit X is readable when both mcounteren
 *   bit X and hcounteren bit X are set, otherwise reads as zero.
 *
 * See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 10.
 */

/* VS-mode trampoline: read scountovf and return its value */
static uintptr_t _vs_read_scountovf(uintptr_t arg)
{
    (void)arg;
    return CSRR(scountovf);
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCOFPMF-01: VS-mode both gates allow, real OF visible     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sscofpmf_01_vs_both_allow);
bool test_hcross_sscofpmf_01_vs_both_allow(void) {
    TEST_BEGIN("HCROSS-SSCOFPMF-01: VS-mode scountovf both gates allow");
    if (!has_hext()) TEST_SKIP("H extension not supported");
    unsigned n = find_first_counter();
    if (!n) TEST_SKIP("no hpmcounter implemented");
    if (!is_scountovf_functional(n)) TEST_SKIP("scountovf not functional on this platform");

    /* Set both mcounteren and hcounteren bits */
    uintptr_t mc_orig = CSRR(mcounteren);
    uintptr_t hc_orig = CSRR(hcounteren);
    CSRW(mcounteren, mc_orig | (1UL << n));
    CSRW(hcounteren, hc_orig | (1UL << n));

    /* Set OF=1 */
    uint64_t orig = cofpmf_read_event(n);
    cofpmf_write_event(n, orig | MHPMEVENT_OF);

    /* VS-mode read: no trap, and the bit reflects the real OF value */
    trap_expect_begin();
    uintptr_t sovf = run_in_vs_mode(_vs_read_scountovf, 0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* Restore before asserting (so cleanup always runs) */
    cofpmf_write_event(n, orig);
    CSRW(mcounteren, mc_orig);
    CSRW(hcounteren, hc_orig);

    TEST_ASSERT("VS-mode scountovf read no trap", !trapped);
    TEST_ASSERT("VS-mode sees real OF value", (sovf & (1UL << n)) != 0);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCOFPMF-02: VS-mode mcounteren=0 reads bit as zero        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sscofpmf_02_vs_mc0);
bool test_hcross_sscofpmf_02_vs_mc0(void) {
    TEST_BEGIN("HCROSS-SSCOFPMF-02: VS-mode scountovf mcounteren=0");
    if (!has_hext()) TEST_SKIP("H extension not supported");
    unsigned n = find_first_counter();
    if (!n) TEST_SKIP("no hpmcounter implemented");
    if (!is_scountovf_functional(n)) TEST_SKIP("scountovf not functional on this platform");

    /* Clear mcounteren bit (hcounteren doesn't matter), set OF=1 */
    uintptr_t mc_orig = CSRR(mcounteren);
    uintptr_t hc_orig = CSRR(hcounteren);
    CSRW(mcounteren, mc_orig & ~(1UL << n));
    CSRW(hcounteren, hc_orig | (1UL << n));

    uint64_t orig = cofpmf_read_event(n);
    cofpmf_write_event(n, orig | MHPMEVENT_OF);

    /* VS-mode read: no trap, but the bit must be masked to zero */
    trap_expect_begin();
    uintptr_t sovf = run_in_vs_mode(_vs_read_scountovf, 0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    cofpmf_write_event(n, orig);
    CSRW(mcounteren, mc_orig);
    CSRW(hcounteren, hc_orig);

    TEST_ASSERT("VS-mode scountovf read no trap", !trapped);
    TEST_ASSERT("VS-mode bit masked to 0 with mcounteren=0",
                (sovf & (1UL << n)) == 0);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCOFPMF-03: VS-mode hcounteren=0 reads bit as zero        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sscofpmf_03_vs_hc0);
bool test_hcross_sscofpmf_03_vs_hc0(void) {
    TEST_BEGIN("HCROSS-SSCOFPMF-03: VS-mode scountovf hcounteren=0");
    if (!has_hext()) TEST_SKIP("H extension not supported");
    unsigned n = find_first_counter();
    if (!n) TEST_SKIP("no hpmcounter implemented");
    if (!is_scountovf_functional(n)) TEST_SKIP("scountovf not functional on this platform");

    /* Set mcounteren=1, clear hcounteren, set OF=1 */
    uintptr_t mc_orig = CSRR(mcounteren);
    uintptr_t hc_orig = CSRR(hcounteren);
    CSRW(mcounteren, mc_orig | (1UL << n));
    CSRW(hcounteren, hc_orig & ~(1UL << n));

    uint64_t orig = cofpmf_read_event(n);
    cofpmf_write_event(n, orig | MHPMEVENT_OF);

    /* VS-mode read: no trap, but the bit must be masked to zero */
    trap_expect_begin();
    uintptr_t sovf = run_in_vs_mode(_vs_read_scountovf, 0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    cofpmf_write_event(n, orig);
    CSRW(mcounteren, mc_orig);
    CSRW(hcounteren, hc_orig);

    TEST_ASSERT("VS-mode scountovf read no trap", !trapped);
    TEST_ASSERT("VS-mode bit masked to 0 with hcounteren=0",
                (sovf & (1UL << n)) == 0);

    TEST_END();
}
