/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mode_filter_vs.c - Group 10.2: VSINH/VUINH counting inhibition
 *
 * Tests HCROSS-SSCOFPMF-04 through HCROSS-SSCOFPMF-06
 * (new cases filling the gap left by Sscofpmf_test_plan.md Group 2,
 * which only covers M/S/U-mode filtering).
 *
 * norm:mhpmevent_inh_op:
 *   Each xINH bit, when set, inhibits counting of events while in
 *   privilege mode x. If the associated privilege mode is not
 *   implemented, the bit is read-only zero.
 *
 * See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 10.
 */

/* VS/VU-mode trampoline: execute a known instruction sequence */
static uintptr_t _v_execute_nops(uintptr_t count)
{
    cofpmf_execute_nops((unsigned)count);
    return 0;
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCOFPMF-04: VSINH=1 inhibits VS-mode counting             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sscofpmf_04_vsinh_inhibit);
bool test_hcross_sscofpmf_04_vsinh_inhibit(void) {
    TEST_BEGIN("HCROSS-SSCOFPMF-04: VSINH=1 inhibits VS-mode counting");
    if (!has_hext()) TEST_SKIP("H extension not supported");
    unsigned n = find_first_counter();
    if (!n) TEST_SKIP("no hpmcounter implemented");
    if (!is_counting_functional(n)) TEST_SKIP("counter not counting on this platform");

    /* Phase 1: VSINH=1 (+MINH=1 to exclude M-mode transition overhead),
     * the counter must NOT increment while executing in VS-mode. */
    cofpmf_write_event(n, COFPMF_EVENT_INSTRET | MHPMEVENT_VSINH | MHPMEVENT_MINH);
    uint64_t ev_readback = cofpmf_read_event(n);
    if ((ev_readback & MHPMEVENT_VSINH) == 0) {
        /* H extension is implemented (checked above), so VS-mode exists
         * and VSINH must NOT be read-only zero per norm:mhpmevent_inh_op. */
        cofpmf_write_event(n, 0);
        TEST_ASSERT("VSINH must be writable when VS-mode is implemented", false);
        TEST_END();
    }
    cofpmf_write_counter(n, 0);
    run_in_vs_mode(_v_execute_nops, 200);
    uint64_t count_inhibited = cofpmf_read_counter(n);
    cofpmf_stop_counting(n);

    /* Phase 2: positive control with VSINH=0 — the same VS-mode
     * sequence must count, proving the phase-1 zero is due to the
     * inhibit bit rather than a non-counting platform. */
    cofpmf_write_event(n, COFPMF_EVENT_INSTRET | MHPMEVENT_MINH);
    cofpmf_write_counter(n, 0);
    run_in_vs_mode(_v_execute_nops, 200);
    uint64_t count_allowed = cofpmf_read_counter(n);
    cofpmf_stop_counting(n);
    cofpmf_write_counter(n, 0);

    TEST_ASSERT("counter did not increment with VSINH=1", count_inhibited == 0);
    TEST_ASSERT("VS-mode counting works with VSINH=0", count_allowed > 0);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCOFPMF-05: VUINH=1 inhibits VU-mode counting             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sscofpmf_05_vuinh_inhibit);
bool test_hcross_sscofpmf_05_vuinh_inhibit(void) {
    TEST_BEGIN("HCROSS-SSCOFPMF-05: VUINH=1 inhibits VU-mode counting");
    if (!has_hext()) TEST_SKIP("H extension not supported");
    unsigned n = find_first_counter();
    if (!n) TEST_SKIP("no hpmcounter implemented");
    if (!is_counting_functional(n)) TEST_SKIP("counter not counting on this platform");

    /* Phase 1: VUINH=1 (+MINH=1), the counter must NOT increment
     * while executing in VU-mode. */
    cofpmf_write_event(n, COFPMF_EVENT_INSTRET | MHPMEVENT_VUINH | MHPMEVENT_MINH);
    uint64_t ev_readback = cofpmf_read_event(n);
    if ((ev_readback & MHPMEVENT_VUINH) == 0) {
        /* H extension is implemented (checked above), so VU-mode exists
         * and VUINH must NOT be read-only zero per norm:mhpmevent_inh_op. */
        cofpmf_write_event(n, 0);
        TEST_ASSERT("VUINH must be writable when VU-mode is implemented", false);
        TEST_END();
    }
    cofpmf_write_counter(n, 0);
    run_in_vu_mode(_v_execute_nops, 200);
    uint64_t count_inhibited = cofpmf_read_counter(n);
    cofpmf_stop_counting(n);

    /* Phase 2: positive control with VUINH=0. */
    cofpmf_write_event(n, COFPMF_EVENT_INSTRET | MHPMEVENT_MINH);
    cofpmf_write_counter(n, 0);
    run_in_vu_mode(_v_execute_nops, 200);
    uint64_t count_allowed = cofpmf_read_counter(n);
    cofpmf_stop_counting(n);
    cofpmf_write_counter(n, 0);

    TEST_ASSERT("counter did not increment with VUINH=1", count_inhibited == 0);
    TEST_ASSERT("VU-mode counting works with VUINH=0", count_allowed > 0);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCOFPMF-06: VSINH/VUINH read-only zero without H-ext      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sscofpmf_06_no_h_roz);
bool test_hcross_sscofpmf_06_no_h_roz(void) {
    TEST_BEGIN("HCROSS-SSCOFPMF-06: VSINH/VUINH read-only zero (no H-ext)");
    if (has_hext()) TEST_SKIP("H extension present, negative branch not applicable");
    unsigned n = find_first_counter();
    if (!n) TEST_SKIP("no hpmcounter implemented");

    uint64_t orig = cofpmf_read_event(n);

    /* Attempt to set VSINH and VUINH; both must stay read-only zero */
    cofpmf_write_event(n, orig | MHPMEVENT_VSINH | MHPMEVENT_VUINH);
    uint64_t val = cofpmf_read_event(n);

    cofpmf_write_event(n, orig);

    TEST_ASSERT("VSINH read-only zero without H extension",
                (val & MHPMEVENT_VSINH) == 0);
    TEST_ASSERT("VUINH read-only zero without H extension",
                (val & MHPMEVENT_VUINH) == 0);

    TEST_END();
}
