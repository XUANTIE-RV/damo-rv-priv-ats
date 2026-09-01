/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 7.1: Unimplemented hpmcounter behavior when V=1
 *
 * HZHPM-01 ~ HZHPM-04
 *
 * Spec anchors:
 *   norm:hpm_unimplemented_counter_access
 *     — Accessing an unimplemented counter may cause an
 *       illegal-instruction exception OR return a constant value;
 *       both are legal, and both remain legal when V=1. With the
 *       gating open, virtual-instruction must NOT be forced for the
 *       unimplemented counter itself.
 *   hcounteren_gate_v1_counter (norm:hcounteren_op specialization)
 *     — When the gate is closed (hcounteren[N]=0, mcounteren[N]=1),
 *       the access traps with virtual-instruction (cause=22)
 *       regardless of whether the counter is implemented.
 *     — When mcounteren[N]=0 (e.g. read-only zero, as for QEMU's
 *       HPM bits), the mcounteren layer takes precedence and the
 *       access from V=1 traps with illegal-instruction (cause=2);
 *       virtual-instruction is NOT permitted in that case.
 *
 * Implementation status of hpmcounterN is probed with the
 * Shcounterenw strategy (mhpmcounterN write-then-readback).
 * =================================================================== */

/* ---- HZHPM-01: VS-mode access, gate open ---- */

TEST_REGISTER(test_hzhpM_01);
bool test_hzhpM_01(void)
{
    TEST_BEGIN("HZHPM-01: VS unimplemented hpmcounter (gate open)");
    REQUIRE_H_EXT();

    unsigned n = hzhpM_find_unimplemented();
    if (n == 0)
        TEST_SKIP("all hpmcounter3-31 implemented (no unimplemented found)");

    uintptr_t bit = 1UL << n;
    uint16_t addr = CSR_HPMCOUNTER(n);

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();

    bool m_ok = MCNT_SET_STICK(bit);
    bool h_ok = m_ok && HCNT_SET_STICK(bit);

    trap_expect_begin();
    uintptr_t v = run_in_vs_mode(_vs_read_hpm, addr);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trapped ? trap_get_cause() : 0;
    trap_expect_end();

    if (!m_ok) {
        /* mcounteren[N] read-only zero: the counter can never be
         * offered to the guest; the mcounteren layer takes precedence
         * and the access must report illegal-instruction, NOT
         * virtual-instruction. */
        printf("  hpmcounter%u: mcounteren[%u] read-only zero, "
               "mcounteren-layer path\n", n, n);
        TEST_ASSERT("access trapped (mcounteren[N]=0)", trapped);
        if (trapped)
            TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                           cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    } else if (h_ok) {
        /* Gate fully open: legal outcomes are a constant value or an
         * illegal-instruction from the unimplemented counter itself.
         * virtual-instruction is NOT permitted here. */
        if (trapped) {
            printf("  hpmcounter%u: trapped, cause=%lu\n",
                   n, (unsigned long)cause);
            TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                           cause, (uintptr_t)CAUSE_ILLEGAL_INST);
        } else {
            printf("  hpmcounter%u: constant value 0x%lx\n",
                   n, (unsigned long)v);
            TEST_ASSERT("constant value with gate open is legal", 1);
        }
    } else {
        /* Gate cannot be opened (hcounteren[N] read-only zero):
         * norm:hcounteren_warl says V=1 reads then trap. */
        printf("  hpmcounter%u: hcounteren[%u] read-only zero, "
               "gating path\n", n, n);
        TEST_ASSERT("access trapped with gate closed", trapped);
        if (trapped)
            TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                           cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }

    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}

/* ---- HZHPM-02: VU-mode access, gates fully open ---- */

TEST_REGISTER(test_hzhpM_02);
bool test_hzhpM_02(void)
{
    TEST_BEGIN("HZHPM-02: VU unimplemented hpmcounter (gates open)");
    REQUIRE_H_EXT();

    unsigned n = hzhpM_find_unimplemented();
    if (n == 0)
        TEST_SKIP("all hpmcounter3-31 implemented (no unimplemented found)");

    uintptr_t bit = 1UL << n;
    uint16_t addr = CSR_HPMCOUNTER(n);

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();
    uintptr_t saved_scen = scounteren_read();

    bool m_ok = MCNT_SET_STICK(bit);
    bool h_ok = m_ok && HCNT_SET_STICK(bit);
    bool sc_ok = m_ok && SCNT_SET_STICK(bit);

    trap_expect_begin();
    uintptr_t v = run_in_vu_mode(_vs_read_hpm, addr);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trapped ? trap_get_cause() : 0;
    trap_expect_end();

    if (!m_ok) {
        /* mcounteren[N] read-only zero: the mcounteren layer takes
         * precedence for VU too; the access must report
         * illegal-instruction, NOT virtual-instruction. */
        printf("  hpmcounter%u: mcounteren[%u] read-only zero, "
               "mcounteren-layer path\n", n, n);
        TEST_ASSERT("access trapped (mcounteren[N]=0)", trapped);
        if (trapped)
            TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                           cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    } else if (h_ok && sc_ok) {
        /* All three layers open: constant value or illegal-instruction
         * from the unimplemented counter itself are both legal. */
        if (trapped) {
            printf("  hpmcounter%u: trapped, cause=%lu\n",
                   n, (unsigned long)cause);
            TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                           cause, (uintptr_t)CAUSE_ILLEGAL_INST);
        } else {
            printf("  hpmcounter%u: constant value 0x%lx\n",
                   n, (unsigned long)v);
            TEST_ASSERT("constant value with gates open is legal", 1);
        }
    } else {
        /* hcounteren[N] or scounteren[N] read-only zero: VU-mode
         * access must trap with virtual-instruction. */
        printf("  hpmcounter%u: gate not fully openable "
               "(h_ok=%d, sc_ok=%d), gating path\n", n, h_ok, sc_ok);
        TEST_ASSERT("access trapped with gate closed", trapped);
        if (trapped)
            TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                           cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }

    scounteren_write(saved_scen);
    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}

/* ---- HZHPM-03: hcounteren[N]=0 blocks unimplemented counter ---- */

TEST_REGISTER(test_hzhpM_03);
bool test_hzhpM_03(void)
{
    TEST_BEGIN("HZHPM-03: hcounteren[N]=0 blocks unimplemented hpm (cause=22)");
    REQUIRE_H_EXT();

    unsigned n = hzhpM_find_unimplemented();
    if (n == 0)
        TEST_SKIP("all hpmcounter3-31 implemented (no unimplemented found)");

    uintptr_t bit = 1UL << n;
    uint16_t addr = CSR_HPMCOUNTER(n);

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();

    bool m_ok = MCNT_SET_STICK(bit);
    if (m_ok && !HCNT_CLR_STICK(bit)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("hcounteren[N] is not clearable");
    }

    trap_expect_begin();
    (void)run_in_vs_mode(_vs_read_hpm, addr);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trapped ? trap_get_cause() : 0;
    trap_expect_end();

    /* Gate closed: the trap is raised by the gating condition and is
     * independent of the counter's implementation status. With
     * mcounteren[N]=1 the report is virtual-instruction; when
     * mcounteren[N] is itself read-only zero the mcounteren layer
     * takes precedence and reports illegal-instruction instead. */
    TEST_ASSERT("access trapped with gate closed", trapped);
    if (trapped) {
        if (m_ok)
            TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                           cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        else
            TEST_ASSERT_EQ("cause=2 (illegal-instruction, mcounteren layer)",
                           cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}

/* ---- HZHPM-04: recording - repeated access consistency ---- */

TEST_REGISTER(test_hzhpM_04);
bool test_hzhpM_04(void)
{
    TEST_BEGIN("HZHPM-04: repeated access consistency (recording)");
    REQUIRE_H_EXT();

    unsigned n = hzhpM_find_unimplemented();
    if (n == 0)
        TEST_SKIP("all hpmcounter3-31 implemented (no unimplemented found)");

    uintptr_t bit = 1UL << n;
    uint16_t addr = CSR_HPMCOUNTER(n);

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();

    /* The consistency scenario follows HZHPM-01's gate-open branch;
     * when the gate cannot be opened (mcounteren[N] or hcounteren[N]
     * read-only zero), the repeated outcome is trivially the same
     * trap and the scenario is not applicable. */
    if (!MCNT_SET_STICK(bit)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren[N] read-only zero (gate not openable)");
    }
    if (!HCNT_SET_STICK(bit)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("hcounteren[N] read-only zero (gate not openable)");
    }

    #define HZHPM_ITER  3
    bool trapped[HZHPM_ITER];
    uintptr_t cause[HZHPM_ITER];
    uintptr_t value[HZHPM_ITER];

    for (int i = 0; i < HZHPM_ITER; i++) {
        trap_expect_begin();
        value[i] = run_in_vs_mode(_vs_read_hpm, addr);
        trapped[i] = trap_was_triggered();
        cause[i] = trapped[i] ? trap_get_cause() : 0;
        trap_expect_end();
        printf("  run %d: trapped=%d cause=%lu value=0x%lx\n",
               i + 1, trapped[i], (unsigned long)cause[i],
               (unsigned long)value[i]);
    }

    /* If the implementation returns a constant value, repeated reads
     * must be identical; if it traps, the cause must be identical. */
    for (int i = 1; i < HZHPM_ITER; i++) {
        TEST_ASSERT("trap/no-trap outcome consistent across runs",
                    trapped[i] == trapped[0]);
        if (trapped[0]) {
            TEST_ASSERT("trap cause consistent across runs",
                        cause[i] == cause[0]);
        } else {
            TEST_ASSERT("constant value consistent across runs",
                        value[i] == value[0]);
        }
    }

    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}
