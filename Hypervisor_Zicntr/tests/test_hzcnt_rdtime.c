/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 6.1: rdtime instruction-level htimedelta semantics (VS/VU)
 *
 * HZCNT-01 ~ HZCNT-06
 *
 * Spec anchors:
 *   norm:zicntr_rdtime_op + norm:htimedelta_sz_acc_op
 *     — With V=1, reads of the time CSR in VS/VU-mode (including the
 *       rdtime instruction) return time + htimedelta. HS-mode reads
 *       are unaffected.
 *   hcounteren_gate_v1_counter (norm:hcounteren_op specialization)
 *     — hcounteren.TM=0 with mcounteren.TM=1: V=1 time reads raise
 *       virtual-instruction (cause=22). mcounteren.TM=0: the access
 *       traps with illegal-instruction (cause=2) instead.
 *
 * The CSR-level htimedelta semantics are already covered by
 * Hypervisor_CSR_test_plan.md HTDLT-01~05 (csrr path); this group
 * exercises the instruction-level (rdtime) path.
 *
 * Clock comparisons use difference intervals, never exact equality:
 * the real time naturally advances between the guest read and the
 * reference read.
 * =================================================================== */

/* ---- HZCNT-01: VS-mode rdtime returns time + delta ---- */

TEST_REGISTER(test_hzcnt_01);
bool test_hzcnt_01(void)
{
    TEST_BEGIN("HZCNT-01: VS-mode rdtime returns time + htimedelta");
    REQUIRE_H_EXT();
    REQUIRE_ZICNTR();

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();

    if (!mcounteren_bit_set_sticky(CNT_BIT_TM)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren.TM is read-only zero");
    }
    if (!hcounteren_bit_set_sticky(CNT_BIT_TM)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("hcounteren.TM is read-only zero");
    }
    if (!htimedelta_set_checked(HZCNT_DELTA)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("htimedelta write did not stick");
    }

    trap_expect_begin();
    uintptr_t v = run_in_vs_mode(_vs_read_time, 0);
    TEST_ASSERT("VS-mode rdtime no trap", !trap_was_triggered());
    trap_expect_end();

    uintptr_t t_now = csr_read(CSR_TIME);
    int64_t diff = (int64_t)(v - t_now);
    printf("  rdtime=0x%lx, time=0x%lx, diff=%ld (delta=0x%lx)\n",
           (unsigned long)v, (unsigned long)t_now, (long)diff,
           (unsigned long)HZCNT_DELTA);
    TEST_ASSERT("rdtime - time <= delta",
                diff <= (int64_t)HZCNT_DELTA);
    TEST_ASSERT("rdtime - time >= delta - bound",
                diff >= (int64_t)(HZCNT_DELTA - HZCNT_BOUND));

    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}

/* ---- HZCNT-02: VU-mode rdtime returns time + delta ---- */

TEST_REGISTER(test_hzcnt_02);
bool test_hzcnt_02(void)
{
    TEST_BEGIN("HZCNT-02: VU-mode rdtime returns time + htimedelta");
    REQUIRE_H_EXT();
    REQUIRE_ZICNTR();

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();
    uintptr_t saved_scen = scounteren_read();

    if (!mcounteren_bit_set_sticky(CNT_BIT_TM)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren.TM is read-only zero");
    }
    if (!hcounteren_bit_set_sticky(CNT_BIT_TM)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("hcounteren.TM is read-only zero");
    }
    if (!scounteren_bit_set_sticky(CNT_BIT_TM)) {
        scounteren_write(saved_scen);
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("scounteren.TM is read-only zero");
    }
    if (!htimedelta_set_checked(HZCNT_DELTA)) {
        scounteren_write(saved_scen);
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("htimedelta write did not stick");
    }

    trap_expect_begin();
    uintptr_t v = run_in_vu_mode(_vs_read_time, 0);
    TEST_ASSERT("VU-mode rdtime no trap", !trap_was_triggered());
    trap_expect_end();

    uintptr_t t_now = csr_read(CSR_TIME);
    int64_t diff = (int64_t)(v - t_now);
    printf("  rdtime=0x%lx, time=0x%lx, diff=%ld (delta=0x%lx)\n",
           (unsigned long)v, (unsigned long)t_now, (long)diff,
           (unsigned long)HZCNT_DELTA);
    TEST_ASSERT("rdtime - time <= delta",
                diff <= (int64_t)HZCNT_DELTA);
    TEST_ASSERT("rdtime - time >= delta - bound",
                diff >= (int64_t)(HZCNT_DELTA - HZCNT_BOUND));

    scounteren_write(saved_scen);
    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}

/* ---- HZCNT-03: HS-mode rdtime is NOT offset by delta ---- */

TEST_REGISTER(test_hzcnt_03);
bool test_hzcnt_03(void)
{
    TEST_BEGIN("HZCNT-03: HS-mode rdtime not affected by htimedelta");
    REQUIRE_H_EXT();
    REQUIRE_ZICNTR();

    uintptr_t saved_mcen = mcounteren_read();

    /* HS-mode time reads are gated by mcounteren (V=0). */
    if (!mcounteren_bit_set_sticky(CNT_BIT_TM)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren.TM is read-only zero");
    }
    if (!htimedelta_set_checked(HZCNT_DELTA)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("htimedelta write did not stick");
    }

    goto_priv(PRIV_S);
    PRIV_DO(HS_RDTIME_CAPTURE());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("HS-mode rdtime with htimedelta set");

    uintptr_t t_now = csr_read(CSR_TIME);
    int64_t diff = (int64_t)(g_hs_time_val - t_now);
    printf("  hs_rdtime=0x%lx, time=0x%lx, diff=%ld\n",
           (unsigned long)g_hs_time_val, (unsigned long)t_now,
           (long)diff);
    /* HS read returns the raw time: no delta applied, and the read
     * happened no later than the reference read. */
    TEST_ASSERT("HS rdtime <= time (no delta added)", diff <= 0);
    TEST_ASSERT("HS rdtime within window of time",
                diff >= -(int64_t)HZCNT_BOUND);

    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}

/* ---- HZCNT-04: negative delta makes rdtime < time ---- */

TEST_REGISTER(test_hzcnt_04);
bool test_hzcnt_04(void)
{
    TEST_BEGIN("HZCNT-04: negative htimedelta -> VS rdtime < time");
    REQUIRE_H_EXT();
    REQUIRE_ZICNTR();

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();

    if (!mcounteren_bit_set_sticky(CNT_BIT_TM)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren.TM is read-only zero");
    }
    if (!hcounteren_bit_set_sticky(CNT_BIT_TM)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("hcounteren.TM is read-only zero");
    }
    if (!htimedelta_set_checked(HZCNT_NEG_DELTA)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("htimedelta write did not stick");
    }

    trap_expect_begin();
    uintptr_t v = run_in_vs_mode(_vs_read_time, 0);
    TEST_ASSERT("VS-mode rdtime no trap", !trap_was_triggered());
    trap_expect_end();

    uintptr_t t_now = csr_read(CSR_TIME);
    /* v = time_at_vs - 0x100000 (truncated to 64 bits), so the
     * signed difference t_now - v == 0x100000 + elapsed window.
     *
     * Note: the unsigned comparison "v < time" holds only when the
     * real time already exceeds the offset magnitude; right after
     * boot the truncated value may wrap above the real time, so the
     * signed difference is the robust observable (the truncation
     * semantics of norm:htimedelta_sz_acc_op are exactly what makes
     * the signed difference constant). */
    int64_t d = (int64_t)(t_now - v);
    printf("  rdtime=0x%lx, time=0x%lx, time-rdtime=%ld\n",
           (unsigned long)v, (unsigned long)t_now, (long)d);
    TEST_ASSERT("time - rdtime >= 0x100000 (signed, truncation applied)",
                d >= (int64_t)0x100000);
    TEST_ASSERT("time - rdtime within bound",
                d <= (int64_t)(0x100000 + HZCNT_BOUND));

    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}

/* ---- HZCNT-05: hcounteren.TM=0 -> VS rdtime cause=22 ---- */

TEST_REGISTER(test_hzcnt_05);
bool test_hzcnt_05(void)
{
    TEST_BEGIN("HZCNT-05: hcounteren.TM=0 VS rdtime -> virtual-inst");
    REQUIRE_H_EXT();
    REQUIRE_ZICNTR();

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();

    if (!mcounteren_bit_set_sticky(CNT_BIT_TM)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren.TM is read-only zero");
    }
    if (!hcounteren_bit_clear_sticky(CNT_BIT_TM)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("hcounteren.TM is not clearable");
    }

    trap_expect_begin();
    (void)run_in_vs_mode(_vs_read_time, 0);
    TEST_ASSERT("VS-mode rdtime trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();

    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}

/* ---- HZCNT-06: mcounteren.TM=0 -> VS rdtime cause=2 ---- */

TEST_REGISTER(test_hzcnt_06);
bool test_hzcnt_06(void)
{
    TEST_BEGIN("HZCNT-06: mcounteren.TM=0 VS rdtime -> illegal-inst");
    REQUIRE_H_EXT();
    REQUIRE_ZICNTR();

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();

    if (!mcounteren_bit_clear_sticky(CNT_BIT_TM)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren.TM is not clearable");
    }
    /* hcounteren setting is irrelevant: mcounteren gates first. */
    (void)saved_hcen;

    trap_expect_begin();
    (void)run_in_vs_mode(_vs_read_time, 0);
    TEST_ASSERT("VS-mode rdtime trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        /* mcounteren layer takes precedence: illegal-instruction,
         * NOT virtual-instruction. */
        TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    }
    trap_expect_end();

    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}
