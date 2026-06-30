/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYP_TRAP_H
#define HYP_TRAP_H

/* ===================================================================
 * Hypervisor trap helpers
 *
 * The bulk of guest-page-fault capture logic lives in common/trap.c
 * (under #ifdef ENABLE_HYP), which records mtval2 / mtinst /
 * hstatus.GVA into the trap_record so that the existing
 * trap_get_*() accessors work uniformly.
 *
 * This header additionally provides:
 * - hyp_trap_record_t: extended trap record with hypervisor fields
 * - hs_trap_handler: HS-mode trap handler for hedeleg-delegated traps
 * - trap_get_spv: query whether trap came from V=1
 * =================================================================== */

#include "types.h"
#include "encoding.h"
#include "test_framework.h"  /* trap_get_htval / htinst / gva */

/* ===================================================================
 * Extended trap record for hypervisor traps
 *
 * Extends the base trap_record with fields specific to H-extension
 * traps (htval, htinst, GVA, SPV).
 * =================================================================== */

typedef struct {
    /* Base trap record fields */
    bool      armed;
    bool      triggered;
    unsigned  priv_level;    /* privilege level at trap entry */
    uintptr_t cause;         /* scause (HS-mode) or mcause (M-mode) */
    uintptr_t epc;           /* sepc / mepc */
    uintptr_t tval;          /* stval / mtval */
    uintptr_t return_addr;

    /* Hypervisor extension fields */
    uintptr_t htval;         /* guest physical address >> 2 */
    uintptr_t htinst;        /* transformed instruction / pseudoinstruction */
    bool      gva;           /* hstatus.GVA: stval is guest virtual address */
    bool      spv;           /* hstatus.SPV: trap came from V=1 */
} hyp_trap_record_t;

/* ===================================================================
 * Direct CSR readers
 * =================================================================== */

/* Read mtval2 directly (faulting GPA >> 2 for guest-page-faults,
 * 0 otherwise). Use trap_get_htval() in tests instead of calling
 * this from a trap-recovery context. */
static inline uintptr_t mtval2_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x34B" : "=r"(v) :: "memory");
    return v;
}

/* Read mtinst directly. */
static inline uintptr_t mtinst_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x34A" : "=r"(v) :: "memory");
    return v;
}

/* ===================================================================
 * HS-mode trap handler
 *
 * Called from hyp_trap_asm.S (_hs_trap_entry) when stvec points to
 * the HS-mode trap vector. Handles traps delegated via hedeleg from
 * VS/VU-mode to HS-mode:
 *   - VS-mode ecall (cause=10): privilege switching
 *   - Virtual-instruction exception (cause=22)
 *   - Guest-page fault (cause=20/21/23)
 * =================================================================== */

/**
 * hs_trap_handler - HS-mode C trap handler
 *
 * Returns the privilege level to resume at after sret.
 */
unsigned hs_trap_handler(void);

/**
 * trap_get_spv - Get hstatus.SPV captured at last trap
 *
 * Returns true if the last trap originated from V=1 (VS/VU-mode).
 */
bool trap_get_spv(void);

/* External reference to the HS-mode trap vector entry point (asm) */
extern void _hs_trap_entry(void);

#endif /* HYP_TRAP_H */
