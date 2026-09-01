/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/hyp_trap.c — HS-mode trap handler implementation
 *
 * Provides hs_trap_handler() for use when stvec points to
 * _hs_trap_entry (hyp_trap_asm.S). This handles traps delegated
 * from VS/VU-mode to HS-mode via hedeleg.
 *
 * The primary guest-page-fault capture (mtval2/mtinst/GVA) for the
 * M-mode path is still handled in common/trap.c (ENABLE_HYP).
 * This file handles the HS-mode path for hedeleg-delegated traps.
 * =================================================================== */

#include "hyp_trap.h"
#include "hyp_csr.h"
#include "encoding.h"

/* Provided by common/string.c (bare-metal libc replacement). */
extern void *memset(void *dst, int c, size_t n);

/* ===================================================================
 * HS-mode trap record (captures hypervisor-specific trap info)
 * =================================================================== */

static hyp_trap_record_t _hs_trap_record;

/* ===================================================================
 * Helper: determine instruction length from the first 2 bytes
 * =================================================================== */
static inline uintptr_t hs_next_instruction(uintptr_t epc) {
    /* Compressed instructions have bits [1:0] != 0b11 */
    uint16_t inst_lo = *(volatile uint16_t *)epc;
    return epc + ((inst_lo & 0x3) == 0x3 ? 4 : 2);
}

/* ===================================================================
 * hs_trap_handler — called from hyp_trap_asm.S
 *
 * Reads scause/sepc/stval (which in HS-mode refer to the HS-level
 * CSRs) plus hstatus.SPV/GVA and htval/htinst.
 *
 * Returns the privilege level for sret (unused by current asm, but
 * kept for future flexibility).
 * =================================================================== */

unsigned hs_trap_handler(void) {
    uintptr_t cause, epc, tval;
    cause = CSRR(scause);
    epc   = CSRR(sepc);
    tval  = CSRR(stval);

    /* Read hypervisor-extension trap info */
    uintptr_t hstat  = hstatus_read();
    uintptr_t htval_v   = CSRR(CSR_HTVAL);
    uintptr_t htinst_v  = CSRR(CSR_HTINST);

    bool spv = (hstat & HSTATUS_SPV) ? true : false;
    bool gva = (hstat & HSTATUS_GVA) ? true : false;

    /* Record trap info */
    _hs_trap_record.triggered  = true;
    _hs_trap_record.cause      = cause;
    _hs_trap_record.epc        = epc;
    _hs_trap_record.tval       = tval;
    _hs_trap_record.htval      = htval_v;
    _hs_trap_record.htinst     = htinst_v;
    _hs_trap_record.gva        = gva;
    _hs_trap_record.spv        = spv;

    /* Interrupts: do NOT advance sepc — the interrupted instruction
     * must be re-executed after sret.  Clear only the hvip bit of the
     * interrupt that fired so other pending virtual interrupts (e.g.
     * VSTIP pending alongside VSSIP) remain deliverable. */
    if (cause & CAUSE_INTERRUPT_BIT) {
        uintptr_t irq = cause & ~CAUSE_INTERRUPT_BIT;
        CSRC(CSR_HVIP, 1UL << irq);
        return PRIV_S;
    }

    /* Handle specific trap causes */
    uintptr_t cause_code = cause & ~CAUSE_INTERRUPT_BIT; /* mask interrupt bit */

    switch (cause_code) {
    case CAUSE_ECALL_FROM_VS:
        /* VS-mode ecall — advance past ecall and return to HS-mode.
         * SPP will be set to the mode before trap (VS), but we want
         * to return to VS-mode normally after sret. */
        CSRW(sepc, epc + 4);
        break;

    case CAUSE_VIRTUAL_INSTRUCTION:
        /* Virtual-instruction exception — skip the faulting instruction */
        CSRW(sepc, hs_next_instruction(epc));
        break;

    case CAUSE_INST_GUEST_PAGE_FAULT:
    case CAUSE_LOAD_GUEST_PAGE_FAULT:
    case CAUSE_STORE_GUEST_PAGE_FAULT:
        /* Guest-page fault — skip the faulting instruction.
         * htval contains the faulting GPA >> 2. */
        CSRW(sepc, hs_next_instruction(epc));
        break;

    default:
        /* For other traps, just advance past the instruction */
        CSRW(sepc, hs_next_instruction(epc));
        break;
    }

    /* CFI (Zicfilp) test hook: force SPELP=1 before sret so tests
     * can verify hardware clears xpelp on sret (LP-17).
     * No-op unless g_trap_force_pelp is set by the test.
     * Use sstatus (not mstatus) because this is the HS-mode handler. */
#if __riscv_xlen > 32
    if (g_trap_force_pelp) {
        CSRS(sstatus, MSTATUS_SPELP_BIT);
    }
#endif

    /* Return to whatever mode was interrupted (sret uses sstatus.SPP
     * and hstatus.SPV to determine the target). */
    return PRIV_S; /* nominal S = HS-mode handler level */
}

/* ===================================================================
 * Accessor for SPV from the HS-mode trap record
 * =================================================================== */

bool trap_get_spv(void) {
    /* Prefer the HS-mode trap record (set by hs_trap_handler) when
     * available. Fall back to the snapshot captured by s_trap_handler
     * or m_trap_handler (trap_record.spv) for traps that went through
     * the common S/M handler path (e.g. LP-36/40 via medeleg). */
    if (_hs_trap_record.triggered)
        return _hs_trap_record.spv;
    return trap_get_spv_snap();
}

/* ===================================================================
 * Accessors for the HS-mode trap record
 *
 * These allow tests that install _hs_trap_entry as stvec to read
 * trap fields (cause, htinst, htval) captured by hs_trap_handler.
 * =================================================================== */

bool hs_trap_was_triggered(void) {
    return _hs_trap_record.triggered;
}

uintptr_t hs_trap_get_cause(void) {
    return _hs_trap_record.cause;
}

uintptr_t hs_trap_get_htinst(void) {
    return _hs_trap_record.htinst;
}

uintptr_t hs_trap_get_htval(void) {
    return _hs_trap_record.htval;
}

void hs_trap_record_reset(void) {
    /* Zero the entire record (including armed, priv_level and
     * return_addr) so no stale field survives a reset. */
    memset(&_hs_trap_record, 0, sizeof(_hs_trap_record));
}
