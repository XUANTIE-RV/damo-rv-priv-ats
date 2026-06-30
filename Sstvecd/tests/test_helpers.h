/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Sstvecd test suite.
 *
 * Design: All test files (test_mode.c / test_base.c / test_direct.c)
 * are #included into test_register.c, so static functions and globals
 * defined here are visible across the whole compilation unit.
 *
 * Sstvecd semantics (per SPEC/sstvecd.adoc):
 *   - stvec.MODE must be capable of holding 0 (Direct).
 *   - When stvec.MODE=Direct, stvec.BASE must be capable of holding any
 *     valid four-byte-aligned address.
 */

#ifndef SSTVECD_TEST_HELPERS_H
#define SSTVECD_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "mem_ops.h"

/* ===================================================================
 * stvec field layout / mode encoding
 * =================================================================== */
#define STVEC_MODE_MASK       0x3UL
#define STVEC_MODE_DIRECT     0x0UL
#define STVEC_MODE_VECTORED   0x1UL

#define STVEC_BASE_MASK       (~STVEC_MODE_MASK)

/* SSIE / SSIP bit position in mideleg / sie / sip (= bit 1) */
#define BIT_SSI               (1UL << 1)

/* SIE bit in sstatus */
#define SSTATUS_SIE_BIT_LOCAL (1UL << 1)

/* ===================================================================
 * Globals provided by tests/sstvecd_strap.S
 * =================================================================== */
extern volatile uintptr_t g_sstvecd_trap_pc;
extern volatile uintptr_t g_sstvecd_trap_cause;
extern void               sstvecd_trap_entry(void);
extern unsigned long      sstvecd_trap_scratch[];

/* ===================================================================
 * Linker-provided regions (used by Group 3 STVEC-DIR-03 as a
 * deliberately-unmapped VA target)
 * =================================================================== */
extern uintptr_t __vm_test_region_start;

/* ===================================================================
 * stvec read / write helpers (M-mode)
 * =================================================================== */
static inline uintptr_t stvec_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, stvec" : "=r"(v));
    return v;
}

static inline void stvec_write(uintptr_t v) {
    asm volatile ("csrw stvec, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * Per-test save / restore stvec
 *
 * Each Sstvecd test case must save the inbound stvec, freely rewrite
 * it during the test, and restore it before returning to keep
 * subsequent tests using the framework's default s_trap_entry.
 * Use as:
 *
 *   STVEC_SAVE();
 *   ... test body ...
 *   STVEC_RESTORE();
 *
 * The macro pair declares a single uintptr_t local and is safe to
 * place anywhere a declaration is permitted (we use C99 scoping).
 * =================================================================== */
#define STVEC_SAVE()        uintptr_t __saved_stvec = stvec_read()
#define STVEC_RESTORE()     stvec_write(__saved_stvec)

/* ===================================================================
 * Reset trap-record globals (called from M-mode before each Group 3
 * case enters S-mode).
 * =================================================================== */
static inline void sstvecd_reset_trap_record(void) {
    g_sstvecd_trap_pc    = 0;
    g_sstvecd_trap_cause = 0;
    /* Re-arm sscratch so the asm trap entry can spill t0/t1. */
    asm volatile ("csrw sscratch, %0"
                  :: "r"(sstvecd_trap_scratch) : "memory");
}

/* ===================================================================
 * Identity mapping for code + UART, used by Group 3.
 *
 * Maps memory at PLATFORM_MEM_BASE with full RWX permissions using
 * 2 MiB megapages, but skips the 2 MiB region containing
 * __vm_test_region_start so STVEC-DIR-03 can leave a 4K VA inside
 * that region deliberately unmapped (-> load page-fault).
 * Also maps the UART page (4K) so printf survives accidental S-mode
 * I/O (we never actually print from S-mode here).
 * =================================================================== */
static int sstvecd_setup_code_mapping(pt_context_t *ctx) {
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    uintptr_t base  = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);

    uintptr_t test_4k_2m = (uintptr_t)&__vm_test_region_start
                           & ~(PAGE_SIZE_2M - 1);
    uintptr_t end        = test_4k_2m + 2 * PAGE_SIZE_2M;

    for (uintptr_t addr = base; addr < end; addr += PAGE_SIZE_2M) {
        if (addr == test_4k_2m)
            continue;   /* leave this 2MB region unmapped for fault tests */
        int ret = pt_map_page(ctx, addr, addr, flags, PT_LEVEL_2M);
        if (ret != 0)
            return ret;
    }

    /* UART (4K page) */
    uintptr_t uart_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;
    pt_map_page(ctx, PLATFORM_UART0_BASE, PLATFORM_UART0_BASE,
                uart_flags, PT_LEVEL_4K);

    return 0;
}

/* ===================================================================
 * S-mode trampoline that swaps stvec to the local sstvecd_trap_entry,
 * runs the supplied test function, and restores stvec on return.
 *
 * vm_run_in_smode() expects a `uintptr_t (*)(uintptr_t)` callback;
 * we route the real test fn through a static file-scope pointer
 * (the file-scope variable is unique per .c after #include because
 * each test_*.c is compiled into the single test_register.o TU --
 * we just need *one* shared definition here, since helpers.h is
 * #included exactly once in test_register.c).
 * =================================================================== */
static uintptr_t (*g_sstvecd_target_fn)(uintptr_t);

static uintptr_t _sstvecd_smode_trampoline(uintptr_t arg) {
    /* Re-arm sscratch on every S-mode entry. */
    asm volatile ("csrw sscratch, %0"
                  :: "r"(sstvecd_trap_scratch) : "memory");

    /* Switch stvec to the local trap entry; vm_run_in_smode set it
     * to s_trap_entry on the M->S transition and we now override.
     * On return from S-mode, vm_run_in_smode does not actively
     * restore stvec; the M-mode test body restores it from
     * STVEC_SAVE/RESTORE macros around the whole vm_run_in_smode
     * call. */
    asm volatile ("csrw stvec, %0"
                  :: "r"((uintptr_t)sstvecd_trap_entry) : "memory");

    return g_sstvecd_target_fn(arg);
}

static inline uintptr_t sstvecd_run_in_smode(pt_context_t *ctx,
                                             uintptr_t (*fn)(uintptr_t),
                                             uintptr_t arg) {
    g_sstvecd_target_fn = fn;
    return vm_run_in_smode(ctx, _sstvecd_smode_trampoline, arg);
}

/* ===================================================================
 * scause helpers
 * =================================================================== */
#define SCAUSE_INTERRUPT_BIT  (1UL << ((sizeof(uintptr_t) * 8) - 1))

static inline int scause_is_interrupt(uintptr_t cause) {
    return (cause & SCAUSE_INTERRUPT_BIT) != 0;
}

static inline uintptr_t scause_code(uintptr_t cause) {
    return cause & ~SCAUSE_INTERRUPT_BIT;
}

#endif /* SSTVECD_TEST_HELPERS_H */
