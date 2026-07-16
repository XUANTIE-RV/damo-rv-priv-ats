/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/hyp_vs_trap.c — VS-mode custom trap entry support
 * =================================================================== */

#include "hyp_vs_trap.h"
#include "hyp_csr.h"

/* Internal VS trap record (singleton — one active test at a time) */
static vs_trap_record_t _vs_trap_record;

/* ===================================================================
 * VS trap handler configuration
 * =================================================================== */

void vs_trap_setup(vs_trap_config_t *cfg) {
    uintptr_t tvec_val = (cfg->handler_base & VSTVEC_BASE_MASK) |
                         ((uintptr_t)cfg->mode & VSTVEC_MODE_MASK);
    vstvec_write(tvec_val);
    vs_trap_clear();
}

void vs_trap_setup_direct(uintptr_t handler_addr) {
    vs_trap_config_t cfg = {
        .handler_base = handler_addr,
        .mode = VSTVEC_MODE_DIRECT,
        .num_entries = 0,
    };
    vs_trap_setup(&cfg);
}

/* ===================================================================
 * VS trap record access
 * =================================================================== */

vs_trap_record_t *vs_trap_get_last(void) {
    return &_vs_trap_record;
}

void vs_trap_clear(void) {
    _vs_trap_record.vs_cause = 0;
    _vs_trap_record.vs_epc = 0;
    _vs_trap_record.vs_tval = 0;
    _vs_trap_record.vs_tvec = 0;
    _vs_trap_record.entry_idx = -1;
    _vs_trap_record.triggered = false;
}

bool vs_trap_was_triggered(void) {
    return _vs_trap_record.triggered;
}

/* ===================================================================
 * VS trap record population (called by VS-mode trap handler stub)
 *
 * This function is called from the VS-mode trap handler assembly
 * after saving context. It reads vscause/vsepc/vstval and populates
 * the record. The entry_idx parameter is passed from the vectored
 * stub (or -1 for Direct mode).
 * =================================================================== */

void _vs_trap_record_entry(int entry_idx) {
    /* Read VS CSRs — from VS-mode these are the "native" names:
     * scause=vscause, sepc=vsepc, stval=vstval, stvec=vstvec */
    uintptr_t cause, epc, tval, tvec;
    asm volatile ("csrr %0, scause" : "=r"(cause) :: "memory");
    asm volatile ("csrr %0, sepc"   : "=r"(epc)   :: "memory");
    asm volatile ("csrr %0, stval"  : "=r"(tval)  :: "memory");
    asm volatile ("csrr %0, stvec"  : "=r"(tvec)  :: "memory");

    _vs_trap_record.vs_cause = cause;
    _vs_trap_record.vs_epc = epc;
    _vs_trap_record.vs_tval = tval;
    _vs_trap_record.vs_tvec = tvec;
    _vs_trap_record.entry_idx = entry_idx;
    _vs_trap_record.triggered = true;
}

/* ===================================================================
 * vstvec Vectored mode probe
 * =================================================================== */

bool vstvec_supports_vectored(void) {
    uintptr_t saved = vstvec_read();
    vstvec_write((saved & VSTVEC_BASE_MASK) | VSTVEC_MODE_VECTORED);
    int mode = vstvec_get_mode();
    vstvec_write(saved);
    return (mode == VSTVEC_MODE_VECTORED);
}
