/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "types.h"
#include "encoding.h"
#include "uart.h"

/* Halt the machine with a fail status via RVMODEL_HALT_FAIL (entry.S).
 * On QEMU/Spike/Sail this terminates the simulation; on HW it spins. */
extern void _halt_fail(void) __attribute__((noreturn));

/* Build with -DENABLE_TRAP_ARM_DIAG (or temporarily #define here) to
 * enable trap-arm state-trace counters that print on UNEXPECTED TRAP.
 * Default-off: zero overhead in normal builds. Used during the
 * fix-gvalid03-fetch-recovery plan to empirically confirm armed-bit
 * clear sites. Note: CFLAGS_EXTRA= is NOT honored by Makefile.common
 * — define here directly when re-enabling. */

/* ===================================================================
 * Privilege mode definitions are in encoding.h.
 * =================================================================== */

/* ===================================================================
 * Trap record - captures exception information for test assertions
 * =================================================================== */
typedef volatile struct {
    bool      armed;        /* true = expecting a possible exception */
    bool      triggered;    /* true = an exception was captured */
    unsigned  priv_level;   /* privilege level where exception occurred */
    uintptr_t cause;        /* mcause / scause value */
    uintptr_t epc;          /* mepc / sepc value */
    uintptr_t tval;         /* mtval / stval value */
    uintptr_t status_snap;  /* mstatus/sstatus snapshot at trap entry */
    uintptr_t return_addr;  /* for instruction faults: where to resume */
#ifdef ENABLE_HYP
    uintptr_t htval;        /* mtval2 (M-mode) / htval (S-mode), hardware value >> 2 */
    uintptr_t htinst;       /* mtinst / htinst, hardware value */
    bool      gva;          /* mstatus.GVA / hstatus.GVA captured at trap time */
    bool      spv;          /* hstatus.SPV captured at trap time */
#endif
} trap_record_t;

trap_record_t trap_record;

/* Automatic snapshot of a double-trap (cause 16) record taken in the
 * M-mode handler before any later delivery can overwrite trap_record.
 * Tests reset trap_dt_snap_valid before triggering and inspect the
 * snapshot afterwards. */
trap_snapshot_t trap_dt_snap;
bool trap_dt_snap_valid;

/* Ssdbltrp probe robustness state (used by the trap handlers below):
 * a probe is in flight while ssdbltrp_probe_active is set, and
 * ssdbltrp_s_probe_repeats counts how many times the probe trap was
 * delivered, so repeated deliveries cannot loop forever. */
__attribute__((weak)) volatile bool ssdbltrp_probe_active;
__attribute__((weak)) char ssdbltrp_probe_rearm[0];
__attribute__((weak)) char ssdbltrp_run_s_probe[0];
__attribute__((weak)) char ssdbltrp_probe_return[0];
__attribute__((weak)) char ssdbltrp_probe_mtval2_ld[0];
unsigned ssdbltrp_s_probe_repeats;
unsigned ssdbltrp_dt_entries;

/* True when linked into a suite that defines the Ssdbltrp probe
 * symbols (Ssdbltrp suite).  The address check is required because
 * the weak references are NULL in every other suite; the pragma
 * silences -Waddress in the suite where the symbols are strong. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
static inline bool _ssdbltrp_probe_present(void) {
    return &ssdbltrp_probe_active != 0 && &ssdbltrp_probe_rearm != 0;
}
#pragma GCC diagnostic pop

/* ===================================================================
 * Trap entry trace ring (diagnostic)
 *
 * Records the last few trap entries (M and S side) so unexpected
 * traps can dump the sequence that led to them.  Zero overhead when
 * trap_trace_enabled is left at 0.
 * =================================================================== */
#define TRAP_TRACE_DEPTH 8
typedef struct {
    unsigned      priv;    /* PRIV_M / PRIV_S handler side */
    uintptr_t     cause;
    uintptr_t     epc;
    uintptr_t     status;
    uintptr_t     tval;
    uintptr_t     mtval2;
    int           armed;
} trap_trace_entry_t;
static volatile trap_trace_entry_t _trace_ring[TRAP_TRACE_DEPTH];
static volatile unsigned _trace_idx;
static volatile bool trap_trace_enabled;

void trap_trace_on(void)  { trap_trace_enabled = true; }
void trap_trace_off(void) { trap_trace_enabled = false; }

/* ===================================================================
 * mtval2 CSR availability detection
 *
 * mtval2 (0x34B) exists when the Hypervisor extension is implemented
 * (H "adds CSRs mtval2 and mtinst") OR when Ssdbltrp is implemented
 * (norm:mtval2_Ssdbltrap: "The Ssdbltrap extension requires the
 * implementation of the mtval2 CSR" -- independent of H).  On builds
 * whose ISA has neither (e.g. Ss_CSR), reading mtval2 raises an
 * illegal instruction, which inside m_trap_handler would nest into an
 * infinite trap loop.  A compile-time macro cannot express this: it
 * depends on each suite's effective ISA (suites run with restricted
 * ISA strings even on max-capability platform configs), so probe
 * once at reset (M-mode, trap vectors already installed) and read
 * mtval2 in the handler only when present. */
static bool trap_mtval2_present;

/* Trap-arming API (defined further down in this file) */
extern void trap_expect_begin(void);
extern void trap_expect_end(void);
extern bool trap_was_triggered(void);

void trap_probe_mtval2(void) {
    trap_expect_begin();
    (void)CSRR(CSR_MTVAL2);
    trap_mtval2_present = !trap_was_triggered();
    trap_expect_end();
}

static inline void _trace_add(unsigned priv, uintptr_t cause,
                              uintptr_t epc, uintptr_t status,
                              uintptr_t tval, uintptr_t mtval2,
                              int armed) {
    if (!trap_trace_enabled)
        return;
    unsigned i = _trace_idx % TRAP_TRACE_DEPTH;
    _trace_ring[i].priv   = priv;
    _trace_ring[i].cause  = cause;
    _trace_ring[i].epc    = epc;
    _trace_ring[i].status = status;
    _trace_ring[i].tval   = tval;
    _trace_ring[i].mtval2 = mtval2;
    _trace_ring[i].armed  = armed;
    _trace_idx++;
}

static void _trace_dump(void) {
    if (!trap_trace_enabled)
        return;
    printf("  [TRACE] last trap entries (old -> new):\n");
    unsigned start = (_trace_idx < TRAP_TRACE_DEPTH)
                         ? 0 : _trace_idx;
    for (unsigned k = 0; k < TRAP_TRACE_DEPTH && k < _trace_idx; k++) {
        unsigned i = (start + k) % TRAP_TRACE_DEPTH;
        printf("    priv=%c cause=0x%lx epc=0x%lx status=0x%lx "
               "tval=0x%lx mtval2=0x%lx armed=%d\n",
               _trace_ring[i].priv == 3 ? 'M' : 'S',
               (unsigned long)_trace_ring[i].cause,
               (unsigned long)_trace_ring[i].epc,
               (unsigned long)_trace_ring[i].status,
               (unsigned long)_trace_ring[i].tval,
               (unsigned long)_trace_ring[i].mtval2,
               _trace_ring[i].armed);
    }
}

#ifdef ENABLE_HYP
/* Snapshot of mstatus.MPV/MPP captured at M-mode trap entry, before
 * the handler modifies them.  Tests use trap_get_mpv/mpp() to verify
 * hardware auto-writes on trap entry (e.g. TENT-12/13/14). */
static volatile bool      _m_trap_mpv_snap;
static volatile uintptr_t _m_trap_mpp_snap;
#endif

/* CFI (Zicfilp) PELP control flags for Hypervisor_Zicfilp tests.
 * Default 0 preserves existing behavior (clear MPELP/SPELP on
 * software-check trap). Tests set these to verify hardware xRET
 * xpelp handling:
 * - g_trap_preserve_pelp=1: skip clearing MPELP/SPELP so xRET
 *   restores ELP (LP-16/19).
 * - g_trap_force_pelp=1: force MPELP/SPELP=1 before xRET to verify
 *   hardware clears xpelp on xRET (LP-17/20).
 * Default 0 = no impact on existing suites (cfi.Zicfilp, etc.). */
bool g_trap_preserve_pelp = 0;
bool g_trap_force_pelp = 0;

/* Bridge to _exec_return_addr (weak symbol, default=0) */
uintptr_t _exec_return_addr __attribute__((weak)) = 0;

/* ===================================================================
 * ENABLE_TRAP_ARM_DIAG: optional trap-arm state-trace counters.
 *
 * Enabled only when ENABLE_TRAP_ARM_DIAG is #defined at the top of
 * this file (Makefile.common does not propagate CFLAGS_EXTRA, so do
 * not rely on a -D on the command line). Counters and prints are
 * compiled out by default — zero overhead in normal builds.
 * Originally added to confirm the GVALID-03 fetch-into-V=0 trap-arm
 * race; kept around as a turnkey diagnostic for similar issues.
 * =================================================================== */
#ifdef ENABLE_TRAP_ARM_DIAG
unsigned long _diag_arm_begin_count       = 0;
unsigned long _diag_arm_end_count         = 0;
unsigned long _diag_arm_handler_clear     = 0;
unsigned long _diag_arm_last_clear_epc    = 0;
unsigned long _diag_arm_last_clear_cause  = 0;
#endif

/* ===================================================================
 * Ecall convention for privilege switching
 *
 * a0 = ECALL_GOTO_PRIV (1)
 * a1 = target privilege level
 * =================================================================== */
/* Ecall argument storage (set by caller before ecall instruction) */
uintptr_t ecall_args[2];

/* Forward declaration */
extern void goto_priv(unsigned target);

/* Current tracked privilege level */
extern unsigned current_priv;

/* ===================================================================
 * Helper: is this cause an instruction fetch fault?
 * =================================================================== */
static bool is_inst_fault(uintptr_t cause) {
    return (cause == CAUSE_INST_ADDR_MISALIGN ||
            cause == CAUSE_INST_ACCESS_FAULT  ||
            cause == CAUSE_INST_PAGE_FAULT
#ifdef ENABLE_HYP
            /* H-ext: VS-mode fetch into a V=0 leaf (or any G-stage
             * fetch fault) is also an instruction fault from the
             * recovery-anchor perspective: skipping epc+2/4 lands
             * inside the same V=0 page and re-faults forever. The
             * caller (test_vs_exec_expect_fault) installs a recovery
             * label via _exec_return_addr / trap_record.return_addr
             * to escape. */
            || cause == CAUSE_INST_GUEST_PAGE_FAULT
#endif
            );
}

/* ===================================================================
 * Helper: is this cause an ecall?
 * =================================================================== */
static bool is_ecall(uintptr_t cause) {
    /* Interrupts are never ecalls. Must check the interrupt bit
     * BEFORE the CLIC-mode 0xFFF mask, because on RV32 the mask
     * strips bit 31, causing SEI (0x80000009) to collide with
     * CAUSE_ECALL_FROM_S (9) and MEI (0x8000000b) to collide
     * with CAUSE_ECALL_FROM_M (11). */
    if (cause & CAUSE_INTERRUPT_BIT) return false;
    /* In CLIC mode, mcause has extended fields (mpp, mpie, mpil)
     * in the high bits. Mask to exccode only (bits 11:0) before
     * comparing with standard cause codes. */
    cause &= 0xFFF;
    return (cause == CAUSE_ECALL_FROM_U ||
            cause == CAUSE_ECALL_FROM_S ||
            cause == CAUSE_ECALL_FROM_M
#ifdef ENABLE_HYP
            || cause == CAUSE_ECALL_FROM_VS
#endif
            );
}

#ifdef ENABLE_HYP
/* ===================================================================
 * Helper: is this cause a guest-page-fault? (cause 20/21/23)
 * =================================================================== */
static inline bool is_guest_page_fault(uintptr_t cause) {
    return (cause == CAUSE_INST_GUEST_PAGE_FAULT  ||
            cause == CAUSE_LOAD_GUEST_PAGE_FAULT  ||
            cause == CAUSE_STORE_GUEST_PAGE_FAULT);
}

/* ===================================================================
 * QEMU quirk: TVM-protected hgatp/satp access cause normalization
 *
 * Per the privileged spec, when mstatus.TVM=1 a HS-mode CSR access
 * to satp / hgatp / hfence.* must raise an *Illegal Instruction*
 * exception (cause 2). Some QEMU versions instead raise an
 * *Instruction Access Fault* (cause 1) for this case. To keep test
 * assertions spec-strict we transparently rewrite the captured
 * cause when, and only when, the trap context unambiguously matches
 * the TVM-protected pattern:
 *
 *   1. mcause == 1                                  (Inst Access Fault)
 *   2. trap_record.armed                            (test arms a trap)
 *   3. mstatus.MPP == S  &&  mstatus.MPV == 0       (came from HS)
 *   4. mstatus.TVM == 1
 *   5. instruction at mepc is a 32-bit SYSTEM CSR op
 *      with funct3 ∈ {1,2,3,5,6,7} (csrrw/s/c[i])
 *      and csr field ∈ {0x680 hgatp, 0x180 satp}
 *
 * Any other instruction-access fault is left untouched. This is a
 * scoped software shim, equivalent to a HW errata workaround.
 * =================================================================== */
static uintptr_t normalize_qemu_tvm_cause(uintptr_t cause, uintptr_t epc) {
    if (cause != (uintptr_t)CAUSE_INST_ACCESS_FAULT)
        return cause;

    uintptr_t ms  = CSRR(mstatus);
    unsigned  mpp = (unsigned)((ms >> MSTATUS_MPP_OFF) & 0x3UL);
    unsigned  mpv = (unsigned)(((uint64_t)ms >> 39) & 0x1UL);
    unsigned  tvm = (unsigned)((ms >> 20) & 0x1UL);
    if (!(mpp == PRIV_S && mpv == 0 && tvm == 1))
        return cause;

    /* Decode the trapping instruction. Must be a 32-bit SYSTEM CSR
     * op against satp or hgatp. We tolerate epc accesses because
     * the kernel is identity-mapped in M-mode here (no MPRV/satp
     * indirection that would change the effective fetch address). */
    uint16_t lo = *(volatile uint16_t *)epc;
    if ((lo & 0x3) != 0x3)                    /* must be 32-bit */
        return cause;
    uint32_t inst = *(volatile uint32_t *)epc;
    if ((inst & 0x7F) != 0x73)                /* SYSTEM major opcode */
        return cause;
    uint32_t funct3 = (inst >> 12) & 0x7;
    /* csrrw=1, csrrs=2, csrrc=3, csrrwi=5, csrrsi=6, csrrci=7;
     * funct3 == 0 → ECALL/EBREAK/MRET/SRET/WFI/SFENCE — not CSR ops. */
    if (funct3 == 0 || funct3 == 4)
        return cause;
    uint32_t csr = (inst >> 20) & 0xFFF;
    if (csr != 0x680 /* hgatp */ && csr != 0x180 /* satp */)
        return cause;

    return (uintptr_t)CAUSE_ILLEGAL_INST;
}

/* Capture mtval2/mtinst (M-mode side) into trap_record.
 * Also capture mstatus.GVA into trap_record.gva.
 *
 * Called on EVERY trap into M-mode, not only guest-page faults:
 * norm:mtval2_trapval mandates mtval2 = 0 for other traps and
 * norm:H_trap_xtinst_interrupt mandates mtinst = 0 on interrupts,
 * so tests must observe the real hardware-written values instead of
 * software-forced zeros. */
static inline void hyp_capture_m(void) {
    /* mtval2: the faulting guest physical address >> 2 on a
     * guest-page-fault; hardware must write 0 for other traps. */
    trap_record.htval  = CSRR(CSR_MTVAL2);
    trap_record.htinst = CSRR(CSR_MTINST);
    /* mstatus.GVA = bit 38 (RV64 only) */
    uintptr_t ms = CSRR(mstatus);
    trap_record.gva = (((uint64_t)ms >> 38) & 0x1UL) != 0;
}

/* Capture htval/htinst (HS-side) when trap is taken in HS-mode. */
static inline void hyp_capture_s(void) {
    trap_record.htval  = CSRR(CSR_HTVAL);
    trap_record.htinst = CSRR(CSR_HTINST);
    uintptr_t hs = CSRR(CSR_HSTATUS);
    trap_record.gva = ((hs >> 6) & 0x1UL) != 0;  /* HSTATUS_GVA bit 6 */
    /* Record whether the trap came from a virtualized mode. For
     * HS-mode delivery use hstatus.SPVP (written like sstatus.SPP
     * when V was 1, norm:H_trap_hs_csrwrites) falling back to SPV:
     * both are cleared by sret, so they must be snapshotted here. */
    trap_record.spv = (((hs >> 7) & 0x1UL) != 0) ||   /* SPVP */
                      ((hs & HSTATUS_SPV) != 0);      /* SPV  */
}
#endif /* ENABLE_HYP */

/* ===================================================================
 * Helper: compute next instruction address (skip current instruction)
 *
 * Checks if current instruction is compressed (2 bytes) or not (4 bytes).
 * =================================================================== */
static inline uintptr_t next_instruction(uintptr_t epc) {
    uint16_t inst_low = *(volatile uint16_t *)epc;
    /* Compressed instructions have bits [1:0] != 0b11 */
    if ((inst_low & 0x3) == 0x3)
        return epc + 4;
    else
        return epc + 2;
}

/* ===================================================================
 * M-mode trap handler (called from trap.S)
 *
 * Returns the privilege level to return to (for mret MPP setup).
 * =================================================================== */
/* Weak hook: called at every M-mode trap entry with the mtval2 value
 * read at the earliest point of the C handler, where the hardware
 * trap-entry write is guaranteed to be visible (an assembly-side
 * capture right after the MDT-clearing MRET may run too early).
 * Suites that need the entry-time mtval2 (Ssdbltrp) override this
 * hook. */
__attribute__((weak)) void trap_m_entry_mtval2_hook(uintptr_t mtval2) {
    (void)mtval2;
}

unsigned m_trap_handler(void) {
    uintptr_t cause = CSRR(mcause);
    uintptr_t epc   = CSRR(mepc);
    uintptr_t tval  = CSRR(mtval);

    /* mtval2 exists only with the H extension or Ssdbltrp
     * (norm:mtval2_Ssdbltrap); reading it when unimplemented traps
     * into this very handler and loops forever.  Only read it when
     * the reset-time probe found it present (pass 0 otherwise,
     * matching the hardware-write-zero rule for other traps). */
    uintptr_t mtval2_val = trap_mtval2_present ? CSRR(CSR_MTVAL2) : 0;
    _trace_add(PRIV_M, cause, epc, CSRR(mstatus), tval,
               mtval2_val, trap_record.armed);
    /* Feed the mtval2 capture hook on eligible entries; the hook
     * itself applies any suite-specific gating (e.g. one-shot). */
    trap_m_entry_mtval2_hook(mtval2_val);

    /* Ssdbltrp probe tolerance (M side): a cause-16 arrival may hit
     * the handler after the armed flag was already consumed by an
     * earlier record.  Re-arm it while a probe is in flight so the
     * escalated record is taken instead of halting the suite. */
    if (_ssdbltrp_probe_present() && ssdbltrp_probe_active &&
        cause == CAUSE_DOUBLE_TRAP && !trap_record.armed &&
        ssdbltrp_s_probe_repeats < 16) {
        trap_record.armed = true;
    }

    /* ---- Handle ecall for privilege switching ---- */
    if (is_ecall(cause) && ecall_args[0] == ECALL_GOTO_PRIV) {
        unsigned target = (unsigned)ecall_args[1];
        /* Advance past ecall instruction */
        CSRW(mepc, next_instruction(epc));
        /* Update current_priv tracking variable */
        current_priv = target;
        /* Set mstatus.MPP to target privilege level so mret returns there.
         * mstatus.MPP is bits [12:11]. Values: U=0, S=1, M=3.
         * Also clear MPRV (bit 17): if MPP=M, mret does NOT clear MPRV,
         * which would cause M-mode load/store to use U-mode PMP rules
         * (since mret clears MPP to U after returning).
         *
         * For virtualized targets (PRIV_VS=5, PRIV_VU=4), bit 2 marks V=1:
         *   - low 2 bits become MPP (S or U)
         *   - mstatus.MPV (bit 39, RV64) must be set so mret enters V=1
         */
        uintptr_t ms = CSRR(mstatus);
        ms &= ~((3UL << 11) | (1UL << 17)); /* clear MPP and MPRV */
        ms |= ((uintptr_t)(target & 3) << 11); /* set MPP = target & 3 */
#ifdef ENABLE_HYP
        ms &= ~MSTATUS_MPV;                  /* clear V=1 indicator */
        if (target & 0x4)                    /* virtualized target */
            ms |= MSTATUS_MPV;
#endif
        CSRW(mstatus, ms);
        /* Return PRIV_M so _trap_return uses mret (which uses MPP above) */
        return PRIV_M;
    }

    /* ---- Handle asynchronous interrupts ---- */
    if (cause & CAUSE_INTERRUPT_BIT) {
        uintptr_t irq = cause & ~CAUSE_INTERRUPT_BIT;
        if (irq == IRQ_LCOFI) {
            /* Clear LCOFIP to prevent infinite interrupt loop */
            CSRC(mip, MIP_LCOFIP);
        }
        if (irq == IRQ_S_TIMER) {
            /* S-mode timer interrupt: write stimecmp to max value
             * to clear the interrupt source and prevent re-entry */
            CSRW(CSR_STIMECMP, (uintptr_t)-1);
        }
        if (irq == IRQ_M_TIMER) {
            /* M-mode timer interrupt (ACLINT MTIMER): write MTIMECMP[0]
             * to max value to clear the interrupt source */
            uintptr_t mtimecmp_addr = PLATFORM_CLINT_BASE + 0x4000UL;
            asm volatile(
                "li t0, -1\n\t"
                "sw t0, 0(%0)\n\t"
                "sw t0, 4(%0)\n\t"
                "fence\n\t"
                :: "r"(mtimecmp_addr) : "t0", "memory"
            );
        }
        if (irq == IRQ_M_SOFTWARE) {
            /* M-mode software interrupt (ACLINT MSWI): write MSIP[0]
             * to 0 to clear the interrupt source */
            uintptr_t msip_addr = PLATFORM_CLINT_BASE + 0x0000UL;
            asm volatile(
                "sw zero, 0(%0)\n\t"
                "fence\n\t"
                :: "r"(msip_addr) : "memory"
            );
        }
        if (irq == IRQ_S_SOFTWARE) {
            /* S-mode software interrupt (SSIP): clear mip.SSIP to
             * prevent infinite re-entry. SSIP is software-writable. */
            CSRC(mip, (1UL << 1));
        }
        if (irq == IRQ_S_EXTERNAL) {
            /* S-mode external interrupt (SEIP): clear mip.SEIP software
             * bit to prevent infinite re-entry. */
            CSRC(mip, (1UL << 9));
        }
#ifdef ENABLE_HYP
        if (irq == IRQ_VS_SOFTWARE) {
            /* VS software interrupt (VSSIP): the only software-writable
             * source is hvip.VSSIP. Clear it to prevent infinite
             * re-entry when a test injects VSSIP without delegating it
             * to VS-mode (trap delivered to M-mode instead). */
            CSRC(CSR_HVIP, (1UL << 2));
        }
#endif
        /* Record interrupt in trap_record if armed */
        if (trap_record.armed) {
            trap_record.triggered   = true;
            trap_record.priv_level  = PRIV_M;
            trap_record.cause       = cause;
            trap_record.epc         = epc;
            trap_record.tval        = tval;
            trap_record.status_snap = CSRR(mstatus);
#ifdef ENABLE_HYP
            /* Interrupts also write mtval2/mtinst/mstatus.GVA on trap
             * entry (zero per norm:mtval2_trapval and
             * norm:H_trap_xtinst_interrupt); capture the hardware
             * values so tests can verify them. */
            hyp_capture_m();
            trap_record.spv = (CSRR(CSR_HSTATUS) & HSTATUS_SPV) ? true : false;
#endif
            trap_record.armed       = false;
        }
        /* Return to interrupted instruction (no epc advance for interrupts) */
        return PRIV_M;
    }

    /* ---- Handle expected (armed) exceptions ---- */
    if (trap_record.armed) {
#ifdef ENABLE_HYP
        /* QEMU quirk: TVM-protected hgatp/satp access in HS-mode is
         * raised as cause=1 instead of the spec-required cause=2 on
         * some QEMU versions. Normalize before recording so test
         * assertions stay spec-strict. Strict guard inside ensures
         * unrelated IAFs are untouched. */
        cause = normalize_qemu_tvm_cause(cause, epc);
#endif
        trap_record.triggered   = true;
        trap_record.priv_level  = PRIV_M;
        trap_record.cause       = cause;
        trap_record.epc         = epc;
        trap_record.tval        = tval;
        trap_record.status_snap = CSRR(mstatus);
#ifdef ENABLE_HYP
        /* Capture the hardware-written values unconditionally: for
         * non-guest-page-fault traps the spec still mandates specific
         * writes (mtval2/mtinst = 0, norm:mtval2_trapval), so tests
         * must observe the real hardware values rather than
         * software-forced zeros. */
        hyp_capture_m();
        /* Snapshot hstatus.SPV for tests that need to verify trap
         * came from VS-mode (LP-36/40). sret/mret clears SPV, so
         * capture it at trap entry. */
        trap_record.spv = (CSRR(CSR_HSTATUS) & HSTATUS_SPV) ? true : false;
        /* Snapshot mstatus.MPV/MPP before handler consumes them via mret. */
        {
            uintptr_t _ms = CSRR(mstatus);
            _m_trap_mpv_snap = (((uint64_t)_ms >> 39) & 0x1UL) != 0;
            _m_trap_mpp_snap = (_ms >> MSTATUS_MPP_OFF) & 0x3UL;
        }
#endif
#ifdef ENABLE_TRAP_ARM_DIAG
        _diag_arm_handler_clear++;
        _diag_arm_last_clear_epc   = (unsigned long)epc;
        _diag_arm_last_clear_cause = (unsigned long)cause;
#endif
        trap_record.armed      = false;

        /* Clear MPRV so that mret does not resume with translated
         * load/store semantics.  Without this, a test that sets MPRV=1
         * before a trap_expect-protected access will fault again on
         * the very next memory operation after mret. */
        CSRC(mstatus, MSTATUS_MPRV_BIT);

        /* Ssdbltrp double-trap recovery: a double-trap exception
         * (cause 16, norm:sstatus_sdt_trap) arrives with sstatus.SDT
         * still set and mstatus.MPP=S (written like the unexpected
         * trap would have).  mret back to S-mode would leave SDT=1,
         * and the framework's ecall-based return to M-mode would be
         * delegated to S-mode again, cascading into another unexpected
         * trap.  The SPEC note says a handler should clear SDT once
         * state is saved, so do it here and redirect mret to M-mode
         * where the test resumes.  Harmless on implementations without
         * Ssdbltrp (cause 16 cannot occur there). */
        if (cause == CAUSE_DOUBLE_TRAP) {
            /* Bound the escalation loop: if cause-16 entries keep
             * arriving while the probe is in flight, fail explicitly
             * instead of hanging. */
            if (_ssdbltrp_probe_present() && ssdbltrp_probe_active &&
                ++ssdbltrp_dt_entries > 8) {
                printf("\n!!! UNRECOVERABLE: double-trap escalation "
                       "does not terminate; aborting\n");
                _halt_fail();
            }
            /* Snapshot the record now: a later delivery may overwrite
             * trap_record. */
            trap_dt_snap_valid = true;
            trap_dt_snap.triggered   = trap_record.triggered;
            trap_dt_snap.priv_level  = trap_record.priv_level;
            trap_dt_snap.cause       = trap_record.cause;
            trap_dt_snap.epc         = trap_record.epc;
            trap_dt_snap.tval        = trap_record.tval;
            trap_dt_snap.status_snap = trap_record.status_snap;
            /* Probe-flow recovery (Ssdbltrp suite only): clear SDT,
             * redirect mret to M-mode and disable DTE so the ecall
             * return path cannot cascade.  Other suites that observe
             * cause 16 keep the architectural state untouched. */
            if (_ssdbltrp_probe_present() && ssdbltrp_probe_active) {
                CSRC(sstatus, (1UL << 24));       /* sstatus.SDT */
                uintptr_t _ms = CSRR(mstatus);
                _ms |= (3UL << 11);               /* MPP = M-mode */
                CSRW(mstatus, _ms);
                /* Force the resume point to the probe continuation on
                 * EVERY escalation (the generic _exec_return_addr is
                 * consumed after the first use). */
                _exec_return_addr = (uintptr_t)ssdbltrp_probe_rearm;
                /* The record and its mtval2 capture are already
                 * taken; disable DTE so the return path cannot
                 * re-enter the double-trap machinery.  The test
                 * restores menvcfg. */
                CSRC(menvcfg, (1UL << 59));      /* menvcfg.DTE */
            }
        }

        /* CFI (Zicfilp): Any synchronous exception taken while
         * ELP=LP_EXPECTED saves ELP into mstatus.MPELP (trap to M).
         * If we don't clear it before mret, the restored ELP=LP_EXPECTED
         * will cause the next non-LPAD instruction at the recovery PC
         * to fault again (with armed=false → UNEXPECTED TRAP).  This
         * applies to ALL sync exceptions, not just software-check —
         * e.g. instruction access-fault / page-fault during a JALR to
         * an unmapped address also saves ELP to MPELP (LP-29).
         * Note: MPELP(bit 41) and SPELP(bit 33) only exist on RV64.
         * g_trap_preserve_pelp=1 skips clearing so tests can verify
         * hardware xRET restores ELP (LP-16/19). */
#if __riscv_xlen > 32
        if (!g_trap_preserve_pelp) {
            uintptr_t pelp_bits = MSTATUS_MPELP_BIT | MSTATUS_SPELP_BIT;
            CSRC(mstatus, pelp_bits);
        }
#endif

        /* Recovery: if exec_at() set a recovery address, always use it.
         * exec_at() jumps to arbitrary code that may trigger any exception
         * (IAF, illegal instruction, etc.), not just instruction faults.
         * The recovery address returns control to exec_at's caller. */
        if (_exec_return_addr != 0) {
            CSRW(mepc, _exec_return_addr);
            _exec_return_addr = 0;
        } else if (is_inst_fault(cause) && trap_record.return_addr != 0) {
            CSRW(mepc, trap_record.return_addr);
            trap_record.return_addr = 0;
        } else if (is_inst_fault(cause)) {
            /* Instruction fault without recovery address: epc may point
             * to an invalid/tagged address that we cannot safely read
             * (e.g., PM-tagged PC in NEG-01).  Skip 4 bytes as a safe
             * default — the caller should check trap_was_triggered()
             * and not rely on the resumed PC being meaningful. */
            CSRW(mepc, epc + 4);
        } else {
            /* Skip the faulting instruction */
            CSRW(mepc, next_instruction(epc));
        }

        /* CFI (Zicfilp) test hook: force MPELP/SPELP=1 before mret so
         * tests can verify hardware clears xpelp on mret (LP-20).
         * No-op unless g_trap_force_pelp is set by the test. */
#if __riscv_xlen > 32
        if (g_trap_force_pelp) {
            CSRS(mstatus, MSTATUS_MPELP_BIT | MSTATUS_SPELP_BIT);
        }
#endif

        /* Always return PRIV_M so _trap_return uses mret.
         * mret uses mstatus.MPP (set by hardware on trap entry) to
         * return to the correct privilege level (S or U mode).
         * Using sret here would be wrong because sepc is not set. */
        return PRIV_M;
    }

    /* ---- Unexpected exception: fatal error ---- */
    printf("\n!!! UNEXPECTED TRAP in M-mode !!!\n");
#ifdef ENABLE_TRAP_ARM_DIAG
    printf("  [DIAG] begin=%lu end=%lu handler_clear=%lu\n",
           _diag_arm_begin_count, _diag_arm_end_count,
           _diag_arm_handler_clear);
    printf("  [DIAG] last_clear epc=0x%lx cause=0x%lx\n",
           _diag_arm_last_clear_epc, _diag_arm_last_clear_cause);
#endif
    printf("  mcause  = 0x%lx\n", (unsigned long)cause);
    printf("  mepc    = 0x%lx\n", (unsigned long)epc);
    printf("  mtval   = 0x%lx\n", (unsigned long)tval);
    printf("  mstatus = 0x%lx\n", (unsigned long)CSRR(mstatus));
    printf("  armed   = %d  current_priv = %u\n",
           (int)trap_record.armed, current_priv);
    _trace_dump();

    /* Halt with fail status */
    _halt_fail();

    return PRIV_M; /* unreachable */
}

/* ===================================================================
 * S-mode trap handler (called from trap.S)
 *
 * Returns the privilege level to return to (for sret SPP setup).
 * =================================================================== */
unsigned s_trap_handler(void) {
    uintptr_t cause = CSRR(scause);
    uintptr_t epc   = CSRR(sepc);
    uintptr_t tval  = CSRR(stval);

    /* NOTE: do NOT read mtval2 here: mtval2 is an M-mode CSR, so an
     * S-side read may raise an illegal-instruction trap inside the
     * handler itself and overwrite the armed record. Pass 0; the
     * M-side entry still captures mtval2. */
    _trace_add(PRIV_S, cause, epc, CSRR(sstatus), tval,
               0, trap_record.armed);

    /* Ssdbltrp probe tolerance (S side): the probe trap may be
     * redelivered after the armed flag was consumed by an earlier
     * record (e.g. an escalated double-trap entry in M-mode).  Re-arm
     * these traps so the armed branch below records them instead of
     * halting the suite.  Region A is the trampoline entry path plus
     * probe body; region B is the continuation's mtval2 read. */
    if (_ssdbltrp_probe_present() && ssdbltrp_probe_active &&
        cause == CAUSE_ILLEGAL_INST && !trap_record.armed &&
        ssdbltrp_s_probe_repeats < 16 &&
        ((epc >= (uintptr_t)ssdbltrp_run_s_probe &&
          epc < (uintptr_t)ssdbltrp_probe_rearm) ||
         (epc >= (uintptr_t)ssdbltrp_probe_mtval2_ld &&
          epc < (uintptr_t)ssdbltrp_probe_mtval2_ld + 8))) {
        trap_record.armed = true;
    }

    /* ---- Handle ecall for privilege switching ---- */
    if (is_ecall(cause) && ecall_args[0] == ECALL_GOTO_PRIV) {
        unsigned target = (unsigned)ecall_args[1];
        CSRW(sepc, next_instruction(epc));
        goto_priv(target);
        return current_priv;
    }

    /* ---- Handle asynchronous interrupts ---- */
    if (cause & CAUSE_INTERRUPT_BIT) {
        uintptr_t irq = cause & ~CAUSE_INTERRUPT_BIT;
        if (irq == IRQ_LCOFI) {
            /* Clear LCOFIP to prevent infinite interrupt loop */
            CSRC(sip, MIP_LCOFIP);
        }
        if (irq == IRQ_S_TIMER) {
            /* S-mode timer interrupt: write stimecmp to max value
             * to clear the interrupt source and prevent re-entry */
            CSRW(CSR_STIMECMP, (uintptr_t)-1);
        }
#ifdef ENABLE_HYP
        if (irq == IRQ_VS_SOFTWARE) {
            /* VS software interrupt delivered to HS-mode (mideleg
             * bit 2 is read-only 1 per norm:mideleg_acc_h, and
             * hideleg bit 2 is clear). The only software-writable
             * source is hvip.VSSIP; clear it to prevent infinite
             * re-entry. */
            CSRC(CSR_HVIP, (1UL << 2));
        }
#endif
        /* Record interrupt in trap_record if armed */
        if (trap_record.armed) {
            trap_record.triggered   = true;
            trap_record.priv_level  = PRIV_S;
            trap_record.cause       = cause;
            trap_record.epc         = epc;
            trap_record.tval        = tval;
            trap_record.status_snap = CSRR(sstatus);
#ifdef ENABLE_HYP
            /* Interrupts into HS-mode also write htval/htinst/GVA
             * (zero per norm:htval_trapval / H_trap_xtinst_interrupt);
             * capture the hardware values for verification. Also
             * snapshots hstatus.SPV/SPVP into trap_record.spv. */
            hyp_capture_s();
#endif
            trap_record.armed       = false;
        }
        /* Return to interrupted instruction (no epc advance for interrupts) */
        return PRIV_S;
    }

    /* ---- Handle expected (armed) exceptions ---- */
    if (trap_record.armed) {
        trap_record.triggered   = true;
        trap_record.priv_level  = PRIV_S;
        trap_record.cause       = cause;
        trap_record.epc         = epc;
        trap_record.tval        = tval;
        trap_record.status_snap = CSRR(sstatus);
#ifdef ENABLE_HYP
        /* Capture the hardware-written values unconditionally: for
         * non-guest-page-fault traps the spec still mandates specific
         * writes (htval/htinst = 0, norm:htval_trapval), so tests
         * must observe the real hardware values rather than
         * software-forced zeros. Also snapshots hstatus.SPV/SPVP
         * into trap_record.spv. */
        hyp_capture_s();
#endif
        trap_record.armed      = false;

        /* Ssdbltrp probe tolerance: the probe trap may be delivered
         * repeatedly, or resume in a mode where a C call is unsafe.
         * While a probe is in flight, keep the expectation armed so
         * repeated deliveries are recorded instead of halting the
         * suite, and redirect the resume point to the probe
         * continuation label (a mode-agnostic ecall-based return to
         * M-mode).  With a single delivery this only re-arms a harmless
         * leftover expectation. */
        if (_ssdbltrp_probe_present() && ssdbltrp_probe_active &&
            cause == CAUSE_ILLEGAL_INST &&
            ((epc >= (uintptr_t)ssdbltrp_run_s_probe &&
              epc < (uintptr_t)ssdbltrp_probe_rearm) ||
             (epc >= (uintptr_t)ssdbltrp_probe_mtval2_ld &&
              epc < (uintptr_t)ssdbltrp_probe_mtval2_ld + 8))) {
            ssdbltrp_s_probe_repeats++;
            if (ssdbltrp_s_probe_repeats > 4) {
                CSRW(sepc, (uintptr_t)ssdbltrp_probe_rearm);
                return PRIV_S;
            }
            trap_record.armed = true;
            CSRW(sepc, (uintptr_t)ssdbltrp_probe_rearm);
            return PRIV_S;
        }

        /* CFI (Zicfilp): clear SPELP on any synchronous exception for
         * clean recovery, same logic as m_trap_handler. Any sync trap
         * taken while ELP=LP_EXPECTED saves ELP into SPELP (trap to S).
         * Without clearing, sret restores ELP=LP_EXPECTED and the next
         * non-LPAD instruction faults again (LP-15/29).
         * g_trap_preserve_pelp=1 skips clearing so tests can verify
         * hardware xRET restores ELP (LP-16/19).
         * Note: SPELP(bit 23) only exists on RV64. Use sstatus (not
         * mstatus) because this is the S-mode handler. */
#if __riscv_xlen > 32
        if (!g_trap_preserve_pelp) {
            CSRC(sstatus, MSTATUS_SPELP_BIT);
        }
#endif

        /* Recovery: if exec_at() set a recovery address, always use it. */
        if (_exec_return_addr != 0) {
            CSRW(sepc, _exec_return_addr);
            _exec_return_addr = 0;
        } else if (is_inst_fault(cause) && trap_record.return_addr != 0) {
            CSRW(sepc, trap_record.return_addr);
            trap_record.return_addr = 0;
        } else if (is_inst_fault(cause)) {
            /* Instruction fault without recovery address: epc may point
             * to an invalid/tagged address that we cannot safely read.
             * Skip 4 bytes as a safe default. */
            CSRW(sepc, epc + 4);
        } else {
            CSRW(sepc, next_instruction(epc));
        }

        /* CFI (Zicfilp) test hook: force SPELP=1 before sret so tests
         * can verify hardware clears xpelp on sret (LP-17).
         * No-op unless g_trap_force_pelp is set by the test.
         * Use sstatus (not mstatus) because this is the S-mode handler. */
#if __riscv_xlen > 32
        if (g_trap_force_pelp) {
            CSRS(sstatus, MSTATUS_SPELP_BIT);
        }
#endif

        /* Return PRIV_S so _trap_return uses sret, which returns to
         * the correct privilege level using sstatus.SPP (set by hardware). */
        return PRIV_S;
    }

    /* ---- Unexpected exception ---- */
    printf("\n!!! UNEXPECTED TRAP in S-mode !!!\n");
    printf("  scause = 0x%lx\n", (unsigned long)cause);
    printf("  sepc   = 0x%lx\n", (unsigned long)epc);
    printf("  stval  = 0x%lx\n", (unsigned long)tval);
    _trace_dump();

    /* Halt with fail status */
    _halt_fail();

    return PRIV_S; /* unreachable */
}

/* ===================================================================
 * Trap API for test framework
 * =================================================================== */

void trap_expect_begin(void) {
    __sync_synchronize();
    trap_record.armed       = true;
    trap_record.triggered   = false;
    trap_record.return_addr = 0;
    trap_record.cause       = 0;
    trap_record.epc         = 0;
    trap_record.tval        = 0;
    trap_record.status_snap  = 0;
#ifdef ENABLE_TRAP_ARM_DIAG
    _diag_arm_begin_count++;
#endif
    __sync_synchronize();
}

void trap_expect_end(void) {
    __sync_synchronize();
    trap_record.armed = false;
#ifdef ENABLE_TRAP_ARM_DIAG
    _diag_arm_end_count++;
#endif
    __sync_synchronize();
}

/* ===================================================================
 * trap_clear_record - Clear all trap record fields and disarm.
 *
 * Unlike trap_expect_end() which only sets armed=false, this function
 * also clears triggered, cause, epc, tval, and all other fields.
 * Used by reset_state() to ensure a clean slate between tests.
 * =================================================================== */
void trap_clear_record(void) {
    __sync_synchronize();
    trap_record.armed       = false;
    trap_record.triggered   = false;
    trap_record.priv_level  = 0;
    trap_record.cause       = 0;
    trap_record.epc         = 0;
    trap_record.tval        = 0;
    trap_record.status_snap = 0;
    trap_record.return_addr = 0;
#ifdef ENABLE_HYP
    trap_record.htval       = 0;
    trap_record.htinst      = 0;
    trap_record.gva         = false;
    trap_record.spv         = false;
#endif
    __sync_synchronize();
}

bool trap_was_triggered(void) {
    return trap_record.triggered;
}

uintptr_t trap_get_cause(void) {
    return trap_record.cause;
}

uintptr_t trap_get_epc(void) {
    return trap_record.epc;
}

uintptr_t trap_get_tval(void) {
    return trap_record.tval;
}

uintptr_t trap_get_status_snap(void) {
    return trap_record.status_snap;
}

/* Snapshot of the current trap record into caller storage.  Needed by
 * flows where a single hardware event produces more than one handler
 * record (e.g. broken Ssdbltrp implementations delivering both the
 * escalated double-trap to M-mode and the original trap to S-mode):
 * the continuation code snapshots the first record before the second
 * delivery overwrites trap_record. */
void trap_snapshot(trap_snapshot_t *snap) {
    snap->triggered   = trap_record.triggered;
    snap->priv_level  = trap_record.priv_level;
    snap->cause       = trap_record.cause;
    snap->epc         = trap_record.epc;
    snap->tval        = trap_record.tval;
    snap->status_snap = trap_record.status_snap;
}

#ifdef ENABLE_HYP
uintptr_t trap_get_htval(void) {
    return trap_record.htval;
}

uintptr_t trap_get_htinst(void) {
    return trap_record.htinst;
}

bool trap_get_gva(void) {
    return trap_record.gva;
}

bool trap_get_mpv(void) {
    return _m_trap_mpv_snap;
}

uintptr_t trap_get_mpp(void) {
    return _m_trap_mpp_snap;
}

/* hstatus.SPV snapshot captured at trap entry (M or S mode).
 * Used by trap_get_spv() when the trap went through s_trap_handler
 * or m_trap_handler instead of hs_trap_handler. */
bool trap_get_spv_snap(void) {
    return trap_record.spv;
}
#endif
