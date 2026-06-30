/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHVSTVALA_HELPERS_H
#define SHVSTVALA_HELPERS_H

/*
 * shvstvala_helpers.h - Shared declarations for Shvstvala tests
 *
 * Provides:
 *   - External declarations for trap handler globals
 *   - VS-mode payload functions for triggering different exception types
 *   - Two-stage page table setup helpers
 *   - Common test addresses and constants
 */

#include "test_framework.h"
#include "encoding.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/two_stage.h"
#include "hyp/gstage_pt.h"
#include "hyp/test_vs_helpers.h"
#include "pmp/pmp_cfg.h"

/* ===================================================================
 * Trap handler globals (defined in shvstvala_strap.S)
 * =================================================================== */
extern volatile uintptr_t g_shvstvala_vstval;
extern volatile uintptr_t g_shvstvala_cause;
extern void shvstvala_trap_entry(void);
extern char shvstvala_trap_scratch[];

/* ===================================================================
 * Linker-provided test region symbols
 * =================================================================== */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t test_exec_page[];
extern uint8_t test_exec_target[];
extern uint8_t test_pmp_page[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

/* ===================================================================
 * Test address constants
 * =================================================================== */
/* Unmapped VAs for page-fault tests (within VS-stage Sv39 range,
 * but NOT mapped in VS-stage page tables). */
#define UNMAPPED_VA_1       0x40000000UL
#define UNMAPPED_VA_2       0x40001000UL

/* Read-only VA for store page-fault test */
#define READONLY_VA         ((uintptr_t)test_data_area)

/* PMP-restricted PA for access-fault tests */
#define PMP_RESTRICTED_PA   ((uintptr_t)test_pmp_page)

/* ===================================================================
 * VS-mode payload functions
 *
 * Each sets up vstvec via stvec (V=1), then triggers the fault.
 * The VS-mode trap handler records vstval and vscause.
 * =================================================================== */

/* Setup vstvec in VS-mode (must be called at start of each payload) */
static inline void vs_setup_trap_handler(void) {
    uintptr_t entry = (uintptr_t)&shvstvala_trap_entry;
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    uintptr_t scratch = (uintptr_t)shvstvala_trap_scratch;
    asm volatile ("csrw sscratch, %0" :: "r"(scratch));
}

/* --- Page-Fault payloads --- */
static uintptr_t vsmode_load_addr(uintptr_t addr) {
    vs_setup_trap_handler();
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    (void)*ptr;
    return 0;
}

static uintptr_t vsmode_store_addr(uintptr_t addr) {
    vs_setup_trap_handler();
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = 0xDEAD;
    return 0;
}

static uintptr_t vsmode_fetch_addr(uintptr_t addr) {
    vs_setup_trap_handler();
    void (*fn)(void) = (void (*)(void))addr;
    fn();
    return 0;
}

/* --- Misaligned payloads --- */
static uintptr_t vsmode_jump_misaligned(uintptr_t target) {
    vs_setup_trap_handler();
    /* Jump to odd address -> instruction address misaligned (cause=0) */
    asm volatile ("jalr zero, %0, 0" :: "r"(target) : "memory");
    return 0;
}

static uintptr_t vsmode_load_misaligned(uintptr_t addr) {
    vs_setup_trap_handler();
    /* Attempt non-aligned 8-byte load */
    uintptr_t val;
    asm volatile ("ld %0, 0(%1)" : "=r"(val) : "r"(addr));
    (void)val;
    return 0;
}

static uintptr_t vsmode_store_misaligned(uintptr_t addr) {
    vs_setup_trap_handler();
    /* Attempt non-aligned 8-byte store */
    asm volatile ("sd zero, 0(%0)" :: "r"(addr));
    return 0;
}

/* --- Illegal instruction payloads --- */
static uintptr_t vsmode_exec_at(uintptr_t addr) {
    vs_setup_trap_handler();
    void (*fn)(void) = (void (*)(void))addr;
    fn();
    return 0;
}

/* --- Transparent (vstval read/write) payloads --- */
static uintptr_t vsmode_write_stval(uintptr_t val) {
    /* V=1: csrw stval actually writes vstval */
    asm volatile ("csrw stval, %0" :: "r"(val));
    return 0;
}

static uintptr_t vsmode_read_stval(uintptr_t dummy) {
    (void)dummy;
    uintptr_t val;
    /* V=1: csrr stval actually reads vstval */
    asm volatile ("csrr %0, stval" : "=r"(val));
    return val;
}

/* --- Breakpoint payloads --- */
static uintptr_t vsmode_exec_target(uintptr_t addr) {
    vs_setup_trap_handler();
    void (*fn)(void) = (void (*)(void))addr;
    fn();
    return 0;
}

static uintptr_t vsmode_ebreak(uintptr_t dummy) {
    (void)dummy;
    vs_setup_trap_handler();
    /* Set ra to point past ebreak so the trap handler (cause==3, uses ra)
     * returns here instead of re-executing ebreak in an infinite loop. */
    asm volatile (
        "la ra, 1f\n\t"
        "ebreak\n\t"
        "1:"
        ::: "ra"
    );
    return 0;
}

#endif /* SHVSTVALA_HELPERS_H */
