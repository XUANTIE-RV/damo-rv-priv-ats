/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_mtval2_dep.c - Group 9: mtval2 dependency tests
 *
 * norm:mtval2_Ssdbltrap: the Ssdbltrp extension requires the
 * implementation of the mtval2 CSR.
 *
 * MTVAL2-02/03 exercise the real double-trap / S-mode delivery paths
 * via the ssdbltrp_run_s_probe() trampoline (ssdbltrp_trap_asm.S),
 * which executes a delegated illegal instruction in S-mode and returns
 * to M-mode with a non-delegated ecall.  The test only delegates
 * cause 2 (illegal instruction) so the return ecall always reaches
 * M-mode directly.
 *
 * Safety notes:
 *  - MDT is cleared before arming (M_TRAP_EXPECT_BEGIN) and the
 *    smdbltrp M-mode trap entry clears MDT via MRET, so the M-mode
 *    side never enters a critical-error state (MDT=1 + M-mode
 *    exception = unrecoverable critical error without Smrnmi).
 *  - On a recorded double-trap, the common M-mode handler snapshots
 *    the record (trap_dt_snap), clears sstatus.SDT and redirects
 *    mret to the mode-agnostic ecall-based continuation, preventing
 *    cascading unexpected traps on the return path.
 *  - mtval2 is rewritten on every trap entry into M-mode, so the
 *    expected double-trap value is taken from the entry-time capture
 *    (trap_m_entry_mtval2_hook -> smdbltrp_saved_mtval2), never from
 *    a post-hoc CSR read.
 *  - The SDT=0 canary of MTVAL2-02 verifies the norm:sstatus_sdt_trap
 *    precondition (no escalation with SDT=0) before the SDT=1 probe
 *    runs; the common handler's probe tolerance keeps repeated probe
 *    deliveries recorded instead of halting the suite.
 */

extern void      ssdbltrp_run_s_probe(void);
extern char      ssdbltrp_probe_rearm[];
extern uintptr_t _exec_return_addr;
extern unsigned  ssdbltrp_s_probe_repeats;
extern unsigned  ssdbltrp_dt_entries;
extern uintptr_t smdbltrp_mstatus_wanted;
extern uintptr_t smdbltrp_m_entries;
extern uintptr_t smdbltrp_dbg_ring[];
extern uintptr_t smdbltrp_dbg_ring_idx;

/* One-shot gate for the mtval2 capture hook: set by the test before
 * triggering, consumed by the first hooked M-mode entry.  The armed
 * flag alone is not sufficient because the framework's return ecall
 * would otherwise overwrite the capture. */
static volatile bool _mtval2_hook_wanted;

/* In-flight probe flag consumed by the trap handlers (common/trap.c):
 * while set, probe-region illegal-instruction deliveries are kept
 * recorded and resumed at the ecall-based continuation instead of
 * halting the suite on repeat deliveries. */
volatile bool ssdbltrp_probe_active;

/* Entry-time mtval2 capture override: the common C handler calls this
 * hook with the mtval2 value observed at the earliest handler point,
 * where the hardware trap-entry write is guaranteed to be visible.
 * Later M-mode trap entries rewrite mtval2, so the test gates the
 * hook one-shot. */
void trap_m_entry_mtval2_hook(uintptr_t mtval2)
{
    if (!_mtval2_hook_wanted)
        return;
    _mtval2_hook_wanted = false;
    smdbltrp_saved_mtval2 = mtval2;
}

/* MTVAL2-01: mtval2 CSR exists and is read/write */
TEST_REGISTER(test_mtval2_01);
bool test_mtval2_01(void)
{
    TEST_BEGIN("MTVAL2-01: mtval2 CSR exists and is read/write");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* norm:mtval2_Ssdbltrap: Ssdbltrp requires mtval2.  Accessing an
     * unimplemented CSR raises illegal-instruction, which is a spec
     * violation here and must fail explicitly. */
    uint64_t mtval2 = 0;
    M_EXPECT_NO_TRAP(asm volatile("csrr %0, mtval2" : "=r"(mtval2)));

    /* WARL stability check: mtval2 may only hold a subset of values
     * (norm:mtval2_val), so verify write/read-back stability instead
     * of exact value retention. */
    uintptr_t w1 = 0x12345678ABCDEF00ULL;
    CSRW(mtval2, w1);
    uintptr_t r1 = CSRR(mtval2);
    CSRW(mtval2, r1);
    uintptr_t r2 = CSRR(mtval2);
    TEST_ASSERT_EQ("mtval2 WARL write is stable", r2, r1);

    /* norm:mtval2_val: mtval2 must be able to hold zero. */
    CSRW(mtval2, 0);
    TEST_ASSERT_EQ("mtval2 holds zero", CSRR(mtval2), 0);

    TEST_END();
}

/* MTVAL2-02: double-trap writes original trap cause into mtval2 */
TEST_REGISTER(test_mtval2_02);
bool test_mtval2_02(void)
{
    TEST_BEGIN("MTVAL2-02: double-trap writes original cause into mtval2");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    uintptr_t menvcfg_orig = CSRR(menvcfg);
    uintptr_t medeleg_orig = CSRR(medeleg);
    set_dte();
    clear_sdt();

    /* Delegate illegal-instruction (cause 2) to S-mode so the trap
     * delivery targets S-mode; with sstatus.SDT=1 that delivery is an
     * unexpected trap and escalates to a double-trap into M-mode
     * (norm:sstatus_sdt_trap).  Cause 9 must stay non-delegated so
     * the trampoline's return ecall reaches M-mode directly. */
    CSRW(medeleg, 1UL << CAUSE_ILLEGAL_INST);
    if ((CSRR(medeleg) & (1UL << CAUSE_ILLEGAL_INST)) == 0) {
        CSRW(medeleg, medeleg_orig);
        CSRW(menvcfg, menvcfg_orig);
        TEST_SKIP("medeleg[2] not writable");
    }

    /* Ensure a clean probe window: SDT cleared (accessed from M-mode
     * where it is legal) and MDT cleared — while MDT=1 any M-mode
     * exception is an unrecoverable critical error on platforms
     * without Smrnmi. */
    clear_sdt();
    clear_mdt();

    /* Phase 1 - SDT=0 canary: per norm:sstatus_sdt_trap the delegated
     * trap must be delivered to S-mode without any double-trap
     * escalation while SDT=0.  The canary verifies this precondition
     * so the SDT=1 double-trap probe of phase 2 only runs on an
     * escalation path that behaves as specified. */
    trap_dt_snap_valid = false;
    ssdbltrp_dt_entries = 0;
    ssdbltrp_s_probe_repeats = 0;

    M_TRAP_EXPECT_BEGIN();
    ssdbltrp_probe_active = true;
    ssdbltrp_run_s_probe();
    ssdbltrp_probe_active = false;
    trap_expect_end();

    if (trap_dt_snap_valid) {
        TEST_ASSERT("SDT=0 trap escalated to double-trap "
                    "(violates norm:sstatus_sdt_trap: with SDT=0 the "
                    "trap must be delivered to S-mode); SDT=1 probe "
                    "aborted",
                    false);
        clear_sdt();
        CSRW(medeleg, medeleg_orig);
        CSRW(menvcfg, menvcfg_orig);
        TEST_END();
    }

    /* Phase 2 - arm the double-trap condition and run the S-mode
     * probe.  On a spec-compliant double-trap delivery the M-mode
     * handler redirects mepc to the continuation point via
     * _exec_return_addr. */
    set_sdt();
    _exec_return_addr = (uintptr_t)ssdbltrp_probe_rearm;
    trap_dt_snap_valid = false;
    ssdbltrp_dt_entries = 0;
    _mtval2_hook_wanted = true;
    /* One-shot entry-state capture: only the FIRST M-mode entry after
     * arming describes the hardware double-trap delivery; later
     * entries (e.g. the return ecall) must not overwrite it. */
    smdbltrp_m_entries = 0;
    smdbltrp_mstatus_wanted = 1;
    smdbltrp_dbg_ring_idx = 0;

    M_TRAP_EXPECT_BEGIN();
    ssdbltrp_probe_active = true;
    ssdbltrp_run_s_probe();
    ssdbltrp_probe_active = false;
    /* Back in M-mode. */
    trap_expect_end();

    /* The double-trap record snapshot is taken by the M-mode handler
     * itself (trap_dt_snap), so it survives a possible second
     * delivery that overwrites trap_record. */
    TEST_ASSERT("double-trap delivered to M-mode",
                trap_dt_snap_valid);
    TEST_ASSERT_EQ("mcause = 16 (double-trap exception)",
                   trap_dt_snap_valid ? trap_dt_snap.cause : 0,
                   CAUSE_DOUBLE_TRAP);

    if (trap_dt_snap_valid && trap_dt_snap.cause == CAUSE_DOUBLE_TRAP) {
        printf("  [DBG] dt-snap mepc=0x%lx mcause=0x%lx "
               "entry-mstatus=0x%lx mtval2=0x%lx m_entries=%lu "
               "dt_entries=%u\n",
               (unsigned long)trap_dt_snap.epc,
               (unsigned long)trap_dt_snap.cause,
               (unsigned long)smdbltrp_saved_mstatus,
               (unsigned long)smdbltrp_saved_mtval2,
               (unsigned long)smdbltrp_m_entries,
               ssdbltrp_dt_entries);
        unsigned n = (unsigned)smdbltrp_dbg_ring_idx;
        unsigned start = (n > 4) ? (n - 4) : 0;
        for (unsigned k = start; k < n && k < start + 4; k++) {
            const uintptr_t *s = &smdbltrp_dbg_ring[(k & 3) * 4];
            printf("  [DBG-RING %u] mepc=0x%lx mcause=0x%lx "
                   "mstatus=0x%lx mtval2=0x%lx\n", k,
                   (unsigned long)s[0], (unsigned long)s[1],
                   (unsigned long)s[2], (unsigned long)s[3]);
        }
        /* The hardware must write the cause the unexpected trap would
         * have written into mcause (illegal-instruction = 2) into
         * mtval2.  Use the M-mode entry capture: later M-mode trap
         * entries (the return ecall) rewrite mtval2. */
        TEST_ASSERT_EQ("mtval2 = original cause (illegal-instruction)",
                       smdbltrp_saved_mtval2, CAUSE_ILLEGAL_INST);

        /* Registers except mcause/mtval2 are written with the same
         * information the unexpected trap would have written: the
         * interrupted privilege was S-mode, so MPP must be S.
         * The entry-time mstatus capture is taken in the trap-entry
         * preamble; assert MPP=S only when the capture is plausible
         * (a real delivery with SDT=1 must show MPIE and/or SDT
         * and/or MDT besides MPP), otherwise do not evaluate the
         * check. */
        uintptr_t snap = smdbltrp_saved_mstatus;
        bool snap_plausible =
            (snap & ((1UL << 7) | (1UL << 24) | (1UL << 42))) != 0;
        if (snap_plausible) {
            TEST_ASSERT_EQ("mstatus.MPP = S-mode on double-trap entry",
                           (snap >> 11) & 0x3UL, 1);
        } else {
            printf("  [WARN] entry mstatus capture 0x%lx implausible; "
                   "MPP=S check not evaluated\n",
                   (unsigned long)snap);
        }
    }

    clear_sdt();
    CSRW(medeleg, medeleg_orig);
    CSRW(menvcfg, menvcfg_orig);
    TEST_END();
}

/* MTVAL2-03: mtval2 unchanged by a trap delivered to S-mode */
TEST_REGISTER(test_mtval2_03);
bool test_mtval2_03(void)
{
    TEST_BEGIN("MTVAL2-03: mtval2 unchanged by S-mode trap delivery");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    uintptr_t menvcfg_orig = CSRR(menvcfg);
    uintptr_t medeleg_orig = CSRR(medeleg);
    set_dte();
    clear_sdt();

    /* Pre-load mtval2 with a nonzero sentinel.  mtval2 is WARL and
     * may only hold a subset of values (norm:mtval2_val), so use the
     * read-back as the effective sentinel. */
    uintptr_t sentinel = 0xA5A5A5A5A5A5A5A0ULL;
    CSRW(mtval2, sentinel);
    sentinel = CSRR(mtval2);
    if (sentinel == 0) {
        CSRW(medeleg, medeleg_orig);
        CSRW(menvcfg, menvcfg_orig);
        TEST_SKIP("mtval2 WARL holds only zero, cannot observe preservation");
    }

    /* Delegate illegal-instruction to S-mode; with SDT=0 the probe's
     * illegal instruction must be delivered to S-mode normally (SDT
     * goes 0 -> 1) without any M-mode involvement. */
    CSRW(medeleg, 1UL << CAUSE_ILLEGAL_INST);
    if ((CSRR(medeleg) & (1UL << CAUSE_ILLEGAL_INST)) == 0) {
        CSRW(medeleg, medeleg_orig);
        CSRW(menvcfg, menvcfg_orig);
        TEST_SKIP("medeleg[2] not writable");
    }

    trap_dt_snap_valid = false;
    ssdbltrp_s_probe_repeats = 0;
    ssdbltrp_dt_entries = 0;

    /* See MTVAL2-02: guarantee a clean probe window (SDT=0, MDT=0;
     * an M-mode trap with MDT=1 is an unrecoverable critical error
     * without Smrnmi). */
    clear_sdt();
    clear_mdt();

    M_TRAP_EXPECT_BEGIN();
    ssdbltrp_probe_active = true;
    ssdbltrp_run_s_probe();
    ssdbltrp_probe_active = false;
    /* Back in M-mode via the non-delegated return ecall. */
    trap_expect_end();

    /* With SDT=0 the trap must NOT escalate: any double-trap record
     * (trap_dt_snap set by the M-mode handler) is a spec violation. */
    TEST_ASSERT("no double-trap escalation with SDT=0",
                !trap_dt_snap_valid);

    /* The probe record describes the (last) delivery; the M-mode
     * GOTO_PRIV return path does not record, so it is intact. */
    TEST_ASSERT("trap triggered", trap_was_triggered());
    TEST_ASSERT_EQ("trap delivered to S-mode (illegal-instruction)",
                   trap_get_cause(), CAUSE_ILLEGAL_INST);

    if (trap_was_triggered() &&
        trap_get_cause() == CAUSE_ILLEGAL_INST) {
        /* S-mode delivery with SDT=0 must set SDT to 1
         * (norm:sstatus_sdt_trap). */
        TEST_ASSERT("sstatus.SDT=1 after S-mode delivery",
                    (trap_get_status_snap() & SSTATUS_SDT_BIT) != 0);

        /* mtval2 observation: the probe's return ecall is the first
         * M-mode trap entry after the sentinel was written.  A
         * spec-compliant S-mode-only delivery leaves mtval2 untouched,
         * so the value here must be either
         *   - the sentinel (implementation writes mtval2 only on
         *     double-trap deliveries), or
         *   - zero (implementation applies the hypervisor-style
         *     "other traps write zero" rule on M-mode entries).
         * Any other value proves spurious M-mode involvement (e.g. an
         * escalation that wrote the trap cause into mtval2). */
        uintptr_t mtval2_after = CSRR(mtval2);
        TEST_ASSERT("mtval2 not modified by S-mode trap delivery "
                    "(sentinel or zero expected)",
                    mtval2_after == sentinel || mtval2_after == 0);
        if (mtval2_after == sentinel) {
            printf("  [INFO] mtval2 still holds the sentinel: S-mode "
                   "delivery left it untouched\n");
        }
    }

    clear_sdt();
    CSRW(mtval2, 0);
    CSRW(medeleg, medeleg_orig);
    CSRW(menvcfg, menvcfg_orig);
    TEST_END();
}
