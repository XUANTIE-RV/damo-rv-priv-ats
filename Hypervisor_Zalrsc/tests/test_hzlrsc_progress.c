/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.8 - Constrained-loop forward progress and virtualization-trap
 *             compatibility (HZLRSC-35 ~ HZLRSC-38)
 *
 * Spec basis:
 *   norm:constrained_lrsc_forward_progress_intro - a constrained lr/sc
 *     loop must eventually make progress (SC succeeds) within a bounded
 *     number of attempts.
 *   norm:constrained_lrsc_forward_progress_trap - "H traps" is itself one
 *     of the allowed forward-progress events, so hypervisor preemption of
 *     a guest constrained loop satisfies the guarantee; the guest loop
 *     must still eventually succeed on retry.
 *   norm:unconstrained_lrsc_no_progress - an unconstrained sequence may
 *     never succeed, so no success assertion is made (record-type).
 *   norm:sc_reservation_invalidate - a trap round-trip plus an intervening
 *     hypervisor access may or may not preserve the guest reservation
 *     (record-type).
 *
 * The constrained-loop body is explicit inline asm, kept under 16
 * instructions with only a basic integer op between LR and SC; the
 * watchdog iteration counter is incremented AFTER the SC (in the retry
 * branch), never between LR and SC.
 * =================================================================== */

/* Constrained-loop watchdog: far above the attempts needed without
 * contention, small enough to bound runtime. */
#define HZLRSC_MAX_ITERS    100000UL
/* Fixed attempt budget for the unconstrained control sequence. */
#define HZLRSC_UNC_ITERS    200UL

/* Constrained LR.W/SC.W loop. Returns 1 if the SC succeeded within the
 * watchdog, 0 if the watchdog was exhausted (a forward-progress violation
 * unless a trap intervened). */
static uintptr_t hz_vs_constrained_loop_w(uintptr_t addr)
{
    uintptr_t ok;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li    %0, 0\n\t"
        "li    t3, 0\n\t"
        "1:\n\t"
        "lr.w  t0, (%1)\n\t"
        "addi  t0, t0, 1\n\t"
        "sc.w  t1, t0, (%1)\n\t"
        "addi  t3, t3, 1\n\t"
        "beqz  t1, 2f\n\t"
        "li    t2, %2\n\t"
        "bltu  t3, t2, 1b\n\t"
        "j     3f\n\t"
        "2:\n\t"
        "li    %0, 1\n\t"
        "3:\n\t"
        ".option pop\n\t"
        : "=&r"(ok)
        : "r"(addr), "i"(HZLRSC_MAX_ITERS)
        : "t0", "t1", "t2", "t3", "memory"
    );
    return ok;
}

/* ------------------------------------------------------------------
 * HZLRSC-35: VS-mode constrained LR/SC loop makes forward progress.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_35_constrained_forward_progress);
bool test_hzlrsc_35_constrained_forward_progress(void)
{
    TEST_BEGIN("HZLRSC-35: VS-mode constrained LR/SC loop forward progress");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    trap_expect_begin();
    uintptr_t ok = two_stage_run_in_vs(&ctx, hz_vs_constrained_loop_w, va);
    bool fired = trap_was_triggered();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("constrained loop took no trap", !fired);
    TEST_ASSERT("constrained LR/SC loop completed (SC succeeded within "
                "watchdog; norm:constrained_lrsc_forward_progress_intro)",
                ok == 1);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-36: constrained loop stays compliant when the hypervisor
 *            intervenes ("H traps"). Between VS-mode loop attempts the
 *            hypervisor performs an HS-mode access to the reservation
 *            target (the effect of a preempting trap: it invalidates the
 *            guest reservation and forces SC retries). The guest loop must
 *            still eventually succeed; only a hang is a failure.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_36_constrained_with_hyp_intervention);
bool test_hzlrsc_36_constrained_with_hyp_intervention(void)
{
    TEST_BEGIN("HZLRSC-36: constrained loop compliant under hyp intervention");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    /* Bounded rounds: each round runs a constrained loop attempt in
     * VS-mode; between rounds the hypervisor (M-mode) stores to the same
     * target, invalidating any surviving guest reservation - the same
     * effect a preempting "H trap" has. Forward progress requires the
     * guest SC to succeed in some round; a livock (never succeeding) fails.
     */
    int succeeded_round = -1;
    for (int round = 0; round < 8 && succeeded_round < 0; round++)
    {
        trap_expect_begin();
        uintptr_t ok = two_stage_run_in_vs(&ctx, hz_vs_constrained_loop_w, va);
        bool fired = trap_was_triggered();
        trap_expect_end();
        TEST_ASSERT("intervention round took no unexpected trap", !fired);
        if (ok == 1)
            succeeded_round = round;
        /* Hypervisor preemption surrogate: HS/M-mode store to the target
         * (norm:constrained_lrsc_forward_progress_trap - "H traps"). */
        *(volatile uint32_t *)va = (uint32_t)(0x1000 + round);
    }
    ts2_finish(&ctx);

    TEST_ASSERT("guest constrained loop made forward progress despite "
                "hypervisor intervention between attempts",
                succeeded_round >= 0);
    printf("  [INFO] constrained loop succeeded at round %d\n",
           succeeded_round);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-37 (record-type): unconstrained LR/SC sequence may never
 *            succeed; only a hang is a failure. Fixed attempt budget.
 * ------------------------------------------------------------------ */
static volatile uintptr_t hzlrsc_unc_aux[8];

static uintptr_t hz_vs_unconstrained_seq_w(uintptr_t addr)
{
    uintptr_t succ = 0;
    volatile uintptr_t *aux = &hzlrsc_unc_aux[0];
    for (uintptr_t i = 0; i < HZLRSC_UNC_ITERS; i++)
    {
        uintptr_t lr, sc;
        HZ_LR_W("", lr, addr);
        /* Unconstrain: intervene with unrelated loads/stores (and far
         * more than 16 instructions of separation) between LR and SC, so
         * the reservation is typically lost. */
        *aux = lr;
        uintptr_t t = *aux;
        *aux = t + i;
        t = *aux;
        *(aux + 1) = t ^ i;
        HZ_SC_W("", sc, addr, lr + 1);
        if (sc == 0)
            succ++;
    }
    return succ;   /* record only; may legitimately be 0 */
}

TEST_REGISTER(test_hzlrsc_37_unconstrained_record);
bool test_hzlrsc_37_unconstrained_record(void)
{
    TEST_BEGIN("HZLRSC-37: (record) unconstrained LR/SC sequence");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    trap_expect_begin();
    uintptr_t succ = two_stage_run_in_vs(&ctx, hz_vs_unconstrained_seq_w, va);
    bool fired = trap_was_triggered();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("unconstrained sequence took no trap / did not hang", !fired);
    printf("  [RECORD] unconstrained LR/SC succeeded %lu/%lu attempts "
           "(norm:unconstrained_lrsc_no_progress: 0 is compliant)\n",
           (unsigned long)succ, (unsigned long)HZLRSC_UNC_ITERS);
    /* Record-type: no mandatory success assertion. */
    TEST_ASSERT("record-type case executed", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-38 (record-type): guest reservation survival across a trap
 *            round-trip. VS-mode lr.w -> VM exit (trap to hypervisor) ->
 *            hypervisor access -> VS-mode sc.w. SC success OR failure is
 *            compliant; only a wrong exception type / hang fails.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_38_reservation_survival_record);
bool test_hzlrsc_38_reservation_survival_record(void)
{
    TEST_BEGIN("HZLRSC-38: (record) reservation survival across trap round-trip");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    /* Phase 1: VS-mode lr.w establishes a reservation, then the trampoline
     * ecall exits to the hypervisor (a trap round-trip). */
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired1 = trap_was_triggered();
    trap_expect_end();

    /* Phase 2: hypervisor (M-mode) performs its own access to the target,
     * as a hypervisor would on a VM switch. */
    uintptr_t hyp_obs = *(volatile uint32_t *)va;
    *(volatile uint32_t *)va = hyp_obs;

    /* Phase 3: re-enter VS-mode and issue sc.w. The SC may succeed or
     * fail; both are compliant (norm:sc_reservation_invalidate). */
    trap_expect_begin();
    uintptr_t sc = two_stage_run_in_vs(&ctx, hz_vs_sc_w, va);
    bool fired2 = trap_was_triggered();
    uintptr_t cause2 = fired2 ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("phase 1 (lr.w) took no trap", !fired1);
    /* The sc.w must not raise a memory-protection exception here: the page
     * is fully RW, so any trap would be a wrong exception type. */
    TEST_ASSERT("phase 3 (sc.w) raised no wrong exception / did not hang",
                !fired2);
    if (fired2)
        printf("  unexpected sc.w trap cause=%lu\n", (unsigned long)cause2);
    printf("  [RECORD] sc.w after trap round-trip + hypervisor access: %s "
           "(both outcomes compliant)\n",
           (sc == 0) ? "SUCCESS" : "FAIL(reservation lost)");
    TEST_ASSERT("record-type case executed", true);

    HYP_TEST_END();
}
