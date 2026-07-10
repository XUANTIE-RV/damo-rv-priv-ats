/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common definitions for the Shvstvecd test suite.
 *
 * All test files (test_mode.c / test_base.c / test_direct.c /
 * test_transparent.c / test_vectored.c) are #included into
 * test_register.c, so static functions and globals defined here are
 * visible across the whole compilation unit.
 *
 * Shvstvecd semantics (per SPEC/shvstvecd.adoc):
 *   - vstvec.MODE must be capable of holding 0 (Direct).
 *   - When vstvec.MODE=Direct, vstvec.BASE must be capable of holding
 *     any valid four-byte-aligned address.
 */

#ifndef SHVSTVECD_TEST_HELPERS_H
#define SHVSTVECD_TEST_HELPERS_H

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/two_stage_helpers.h"

/* ===================================================================
 * vstvec field layout / mode encoding
 * (use encoding.h definitions if available, provide fallbacks)
 * =================================================================== */
#ifndef VSTVEC_MODE_MASK
#define VSTVEC_MODE_MASK       0x3UL
#endif
#ifndef VSTVEC_MODE_DIRECT
#define VSTVEC_MODE_DIRECT     0x0UL
#endif
#ifndef VSTVEC_MODE_VECTORED
#define VSTVEC_MODE_VECTORED   0x1UL
#endif
#ifndef VSTVEC_BASE_MASK
#define VSTVEC_BASE_MASK       (~0x3UL)
#endif

/* SSIE / SSIP bit position (= bit 1) */
#define BIT_SSI                (1UL << 1)

/* SIE bit in sstatus */
#define SSTATUS_SIE_BIT        (1UL << 1)

/* scause interrupt bit (MSB) */
#define SCAUSE_INTERRUPT_BIT   (1UL << ((sizeof(uintptr_t) * 8) - 1))

/* ===================================================================
 * Globals provided by tests/shvstvecd_strap.S
 * =================================================================== */
extern volatile uintptr_t g_shvstvecd_trap_pc;
extern volatile uintptr_t g_shvstvecd_trap_cause;
extern void               shvstvecd_trap_entry(void);
extern unsigned long      shvstvecd_trap_scratch[];
extern void               shvstvecd_vectored_table(void);
extern void               shvstvecd_vectored_handler(void);
extern volatile uintptr_t g_shvstvecd_vec_marker;

/* ===================================================================
 * vstvec read/write helpers via CSR 0x205 (M/HS-mode)
 *
 * We use the framework's hyp_csr.h APIs:
 *   vstvec_read(), vstvec_write()
 * For local clarity, also provide direct inline CSR ops.
 * =================================================================== */
static inline uintptr_t vstvec_read_raw(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x205" : "=r"(v));
    return v;
}

static inline void vstvec_write_raw(uintptr_t v) {
    asm volatile ("csrw 0x205, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * Per-test save / restore vstvec
 * =================================================================== */
#define VSTVEC_SAVE()        uintptr_t __saved_vstvec = vstvec_read_raw()
#define VSTVEC_RESTORE()     vstvec_write_raw(__saved_vstvec)

/* ===================================================================
 * Reset trap-record globals
 * =================================================================== */
static inline void shvstvecd_reset_trap_record(void) {
    g_shvstvecd_trap_pc    = 0;
    g_shvstvecd_trap_cause = 0;
    /* Arm vsscratch so the asm trap entry can spill t0/t1.
     * Write to CSR 0x240 (vsscratch) from M/HS-mode. */
    asm volatile ("csrw 0x240, %0"
                  :: "r"(shvstvecd_trap_scratch) : "memory");
}

/* ===================================================================
 * scause helpers
 * =================================================================== */
static inline int shvstvecd_is_interrupt(uintptr_t cause) {
    return (cause & SCAUSE_INTERRUPT_BIT) != 0;
}

static inline uintptr_t shvstvecd_cause_code(uintptr_t cause) {
    return cause & ~SCAUSE_INTERRUPT_BIT;
}

/* ===================================================================
 * Unmapped VA for page-fault tests (DIR-03).
 * This address is deliberately NOT mapped in VS-stage page tables.
 * =================================================================== */
#define UNMAPPED_VA_1  0x40000000UL

#endif /* SHVSTVECD_TEST_HELPERS_H */
