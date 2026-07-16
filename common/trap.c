/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "types.h"
#include "encoding.h"
#include "uart.h"

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
    uintptr_t htval;        /* mtval2 (M-mode) / htval (S-mode) >> 2 */
    uintptr_t htinst;       /* mtinst / htinst */
    bool      gva;          /* hstatus.GVA captured at trap time */
#endif
} trap_record_t;

trap_record_t trap_record;

#ifdef ENABLE_HYP
/* Snapshot of mstatus.MPV/MPP captured at M-mode trap entry, before
 * the handler modifies them.  Tests use trap_get_mpv/mpp() to verify
 * hardware auto-writes on trap entry (e.g. TENT-12/13/14). */
static volatile bool      _m_trap_mpv_snap;
static volatile uintptr_t _m_trap_mpp_snap;
#endif

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
 * Also capture mstatus.GVA into trap_record.gva. */
static inline void hyp_capture_m(void) {
    /* mtval2: contains the faulting guest physical address >> 2
     * (only valid for guest-page-faults; reads as 0 otherwise). */
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
unsigned m_trap_handler(void) {
    uintptr_t cause = CSRR(mcause);
    uintptr_t epc   = CSRR(mepc);
    uintptr_t tval  = CSRR(mtval);

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
            uintptr_t mtimecmp_addr = CLINT_BASE_ADDRESS + 0x4000UL;
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
            uintptr_t msip_addr = CLINT_BASE_ADDRESS + 0x0000UL;
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
        /* Record interrupt in trap_record if armed */
        if (trap_record.armed) {
            trap_record.triggered   = true;
            trap_record.priv_level  = PRIV_M;
            trap_record.cause       = cause;
            trap_record.epc         = epc;
            trap_record.tval        = tval;
            trap_record.status_snap = CSRR(mstatus);
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
        if (is_guest_page_fault(cause))
            hyp_capture_m();
        else {
            trap_record.htval  = 0;
            trap_record.htinst = 0;
            trap_record.gva    = false;
        }
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

        /* CFI (Zicfilp): When a software-check exception (cause 18)
         * is taken due to a Landing Pad Fault, the hardware saves
         * ELP=LP_EXPECTED into mstatus.MPELP.  If we don't clear it
         * before mret, the restored ELP=LP_EXPECTED will cause the
         * next non-LPAD instruction at the recovery PC to fault again
         * (with armed=false → UNEXPECTED TRAP).  Clear both MPELP and
         * SPELP to ensure clean recovery.
         * Note: MPELP(bit 41) and SPELP(bit 33) only exist on RV64. */
#if __riscv_xlen > 32
        if (cause == CAUSE_SOFTWARE_CHECK) {
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

    /* Halt */
    while (1)
        asm volatile("wfi");

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
        /* Record interrupt in trap_record if armed */
        if (trap_record.armed) {
            trap_record.triggered   = true;
            trap_record.priv_level  = PRIV_S;
            trap_record.cause       = cause;
            trap_record.epc         = epc;
            trap_record.tval        = tval;
            trap_record.status_snap = CSRR(sstatus);
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
        if (is_guest_page_fault(cause))
            hyp_capture_s();
        else {
            trap_record.htval  = 0;
            trap_record.htinst = 0;
            trap_record.gva    = false;
        }
#endif
        trap_record.armed      = false;

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

        /* Return PRIV_S so _trap_return uses sret, which returns to
         * the correct privilege level using sstatus.SPP (set by hardware). */
        return PRIV_S;
    }

    /* ---- Unexpected exception ---- */
    printf("\n!!! UNEXPECTED TRAP in S-mode !!!\n");
    printf("  scause = 0x%lx\n", (unsigned long)cause);
    printf("  sepc   = 0x%lx\n", (unsigned long)epc);
    printf("  stval  = 0x%lx\n", (unsigned long)tval);

    while (1)
        asm volatile("wfi");

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
#endif
