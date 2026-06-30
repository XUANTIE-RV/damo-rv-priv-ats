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

void vs_trap_setup_vectored(uintptr_t handler_base, int num_entries) {
    vs_trap_config_t cfg = {
        .handler_base = handler_base,
        .mode = VSTVEC_MODE_VECTORED,
        .num_entries = num_entries,
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
 * Delegation helpers
 * =================================================================== */

void vs_delegate_exceptions(uintptr_t exc_mask, uintptr_t int_mask) {
    /* Ensure M-mode delegates to HS first */
    uintptr_t medeleg, mideleg;
    asm volatile ("csrr %0, 0x302" : "=r"(medeleg) :: "memory");
    asm volatile ("csrr %0, 0x303" : "=r"(mideleg) :: "memory");
    medeleg |= exc_mask;
    mideleg |= int_mask;
    asm volatile ("csrw 0x302, %0" :: "r"(medeleg) : "memory");
    asm volatile ("csrw 0x303, %0" :: "r"(mideleg) : "memory");

    /* Then delegate from HS to VS via hedeleg/hideleg */
    hedeleg_write(hedeleg_read() | exc_mask);
    hideleg_write(hideleg_read() | int_mask);
}

void vs_undelegate_all(void) {
    hedeleg_write(0);
    hideleg_write(0);
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

/* ===================================================================
 * Vectored table generator
 *
 * Each entry is a single RISC-V instruction that jumps to the
 * common handler. For simplicity, we use a "j" (JAL x0, offset)
 * instruction pattern. The common handler stub will call
 * _vs_trap_record_entry with the entry index.
 *
 * NOTE: This is a simplified implementation. Real vectored tables
 * need proper assembly stubs. This function writes NOP placeholders
 * that should be replaced by actual jump instructions generated at
 * link time or by assembly code.
 *
 * A full implementation would place:
 *   entry[0]: j common_handler_0
 *   entry[1]: j common_handler_1
 *   ...
 * For now, we provide the infrastructure and leave the actual
 * instruction encoding to the test-specific assembly file.
 * =================================================================== */

void vs_vectored_table_init(uintptr_t base, int num_entries) {
    /* Each vectored entry slot is 4 bytes (one instruction width).
     * We fill with UNIMP (0x0000) as placeholder — test-specific
     * assembly will overwrite with actual jump instructions. */
    uint32_t *table = (uint32_t *)base;
    for (int i = 0; i < num_entries; i++) {
        /* NOP = 0x00000013 (addi x0, x0, 0) as safe placeholder */
        table[i] = 0x00000013;
    }
}
