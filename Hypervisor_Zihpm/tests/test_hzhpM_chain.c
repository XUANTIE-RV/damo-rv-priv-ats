/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 7.2: VU-mode three-layer gating chain (implemented counter)
 *
 * HZHPM-05
 *
 * Spec anchors:
 *   hcounteren_gate_v1_counter (norm:hcounteren_op specialization)
 *     — VU-mode counter access requires mcounteren[N]=1 and
 *       hcounteren[N]=1; a closed hcounteren bit traps with
 *       virtual-instruction.
 *   H_scsrs_nomatch_vu_counter (norm:H_scsrs_nomatch specialization)
 *     — scounteren has no VS counterpart and keeps gating VU-mode
 *       counter visibility when V=1: clearing scounteren[N] alone
 *       must block the VU read.
 *
 * Chain under test: mcounteren -> hcounteren -> scounteren.
 * =================================================================== */

/* ---- HZHPM-05: hpmcounter VU three-layer gating ---- */

TEST_REGISTER(test_hzhpM_05);
bool test_hzhpM_05(void)
{
    TEST_BEGIN("HZHPM-05: hpmcounter VU three-layer gating chain");
    REQUIRE_H_EXT();

    unsigned n = hzhpM_find_implemented();
    if (n == 0)
        TEST_SKIP("no implemented hpmcounter3-31 found");

    uintptr_t bit = 1UL << n;
    uint16_t addr = CSR_HPMCOUNTER(n);

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();
    uintptr_t saved_scen = scounteren_read();

    if (!MCNT_SET_STICK(bit)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren[N] is read-only zero");
    }
    if (!HCNT_SET_STICK(bit)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("hcounteren[N] is read-only zero");
    }
    if (!SCNT_SET_STICK(bit)) {
        scounteren_write(saved_scen);
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("scounteren[N] is read-only zero");
    }

    /* Phase 1: all three layers open -> VU read succeeds. */
    trap_expect_begin();
    (void)run_in_vu_mode(_vs_read_hpm, addr);
    TEST_ASSERT("VU hpmcounter read no trap (all gates open)",
                !trap_was_triggered());
    trap_expect_end();

    /* Phase 2: clear scounteren[N] alone -> VU read must trap. */
    if (!SCNT_CLR_STICK(bit)) {
        scounteren_write(saved_scen);
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("scounteren[N] is not clearable");
    }
    trap_expect_begin();
    (void)run_in_vu_mode(_vs_read_hpm, addr);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trapped ? trap_get_cause() : 0;
    trap_expect_end();

    TEST_ASSERT("VU hpmcounter read trapped (scounteren[N]=0)", trapped);
    if (trapped)
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    scounteren_write(saved_scen);
    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}
