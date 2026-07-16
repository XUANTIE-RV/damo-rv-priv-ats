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

    /* Return to whatever mode was interrupted (sret uses sstatus.SPP
     * and hstatus.SPV to determine the target). */
    return PRIV_S; /* nominal S = HS-mode handler level */
}

/* ===================================================================
 * Accessor for SPV from the HS-mode trap record
 * =================================================================== */

bool trap_get_spv(void) {
    return _hs_trap_record.spv;
}
