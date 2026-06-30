/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Svadu extension test cases
 *
 * Provides shared test data areas, S-mode test functions, Svadu detection,
 * menvcfg.ADUE access helpers, and PTE inspection/modification utilities
 * used by Svadu test files.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 *
 * Svadu semantics (per RISC-V Privileged Spec):
 *   When Svadu is implemented AND menvcfg.ADUE=1, the hardware atomically
 *   sets PTE.A on access and PTE.D on store/AMO. When menvcfg.ADUE=0, the
 *   processor falls back to Svade behavior (page-fault instead of HW update).
 */

#ifndef SVADU_TEST_HELPERS_H
#define SVADU_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "mem_ops.h"

/* ===================================================================
 * Test data and executable regions
 *
 * Provided by svadu/kernel.ld (same layout as svade/kernel.ld):
 *   .vm_test_region    : 3 x 4KB pages (data / fault / exec) - 4K tests
 *   .vm_test_region_2m : 2 MiB region                         - 2M megapage
 * =================================================================== */
extern uintptr_t __vm_test_region_start;
extern uintptr_t __vm_test_region_2m_start;

/* 4K test pages (within .vm_test_region) */
#define test_data_area  ((volatile uintptr_t *)&__vm_test_region_start)
#define test_fault_page ((volatile uint8_t *)((uintptr_t)&__vm_test_region_start + PAGE_SIZE_4K))
#define test_exec_page  ((uint8_t *)((uintptr_t)&__vm_test_region_start + 2 * PAGE_SIZE_4K))

/* 2 MiB megapage test region */
#define test_region_2m_va ((uintptr_t)&__vm_test_region_2m_start)
#define test_region_2m_pa ((uintptr_t)&__vm_test_region_2m_start)

/* Magic values for verification */
#define MAGIC_WRITE  0xDEADBEEF12345678UL
#define MAGIC_READ   0xCAFEBABE87654321UL

/* ===================================================================
 * Snapshot of menvcfg at boot (populated by main.c before any test).
 * Used by SVADU-CSR-04 to read the reset value of ADUE.
 * =================================================================== */
extern uintptr_t g_menvcfg_reset_value;

/* ===================================================================
 * menvcfg CSR (0x30A) access helpers
 *
 * Inline asm uses the literal 0x30A because csrr/csrs/csrc require
 * the CSR number to be an immediate (not a macro that expands to one
 * at a different stage). common/encoding.h also defines:
 *   #define CSR_MENVCFG  0x30A
 *   #define MENVCFG_ADUE (1ULL << 61)
 * =================================================================== */
static inline uintptr_t menvcfg_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x30A" : "=r"(v));
    return v;
}

static inline void menvcfg_set(uintptr_t mask) {
    asm volatile ("csrs 0x30A, %0" :: "r"(mask) : "memory");
}

static inline void menvcfg_clear(uintptr_t mask) {
    asm volatile ("csrc 0x30A, %0" :: "r"(mask) : "memory");
}

/* Set or clear menvcfg.ADUE and issue the required SFENCE.VMA
 * (SPEC/hypervisor.adoc:2109-2111 requires synchronization after
 * modifying ADUE on non-Hyp platforms). */
static inline void set_menvcfg_adue(int enable) {
    if (enable) menvcfg_set(MENVCFG_ADUE);
    else        menvcfg_clear(MENVCFG_ADUE);
    asm volatile ("sfence.vma zero, zero" ::: "memory");
}

static inline int get_menvcfg_adue(void) {
    return (menvcfg_read() & MENVCFG_ADUE) ? 1 : 0;
}

/* ===================================================================
 * Initialization helper: fill exec page with nop;ret
 * =================================================================== */
static void init_exec_page(void) {
    uint32_t *p = (uint32_t *)test_exec_page;
    for (int i = 0; i < 1024 - 1; i++)
        p[i] = 0x00000013;  /* nop (addi x0, x0, 0) */
    p[1023] = 0x00008067;   /* ret (jalr x0, ra, 0) */
}

/* ===================================================================
 * Common helper: set up identity mapping for code execution
 *
 * Same behavior as svade helpers: maps memory at PLATFORM_MEM_BASE with
 * full RWX permissions using 2 MiB megapages, but SKIPS the 2 MiB
 * region(s) containing test pages. Also maps UART.
 * =================================================================== */
static int setup_code_mapping(pt_context_t *ctx) {
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);

    uintptr_t test_4k_2m  = (uintptr_t)&__vm_test_region_start    & ~(PAGE_SIZE_2M - 1);
    uintptr_t test_2m_2m  = (uintptr_t)&__vm_test_region_2m_start & ~(PAGE_SIZE_2M - 1);

    uintptr_t end = test_2m_2m + 2 * PAGE_SIZE_2M;
    if (test_4k_2m + 2 * PAGE_SIZE_2M > end)
        end = test_4k_2m + 2 * PAGE_SIZE_2M;

    for (uintptr_t addr = base; addr < end; addr += PAGE_SIZE_2M) {
        if (addr == test_4k_2m || addr == test_2m_2m)
            continue;
        int ret = pt_map_page(ctx, addr, addr, flags, PT_LEVEL_2M);
        if (ret != 0) return ret;
    }

    /* Map UART I/O region for S-mode printf support */
    uintptr_t uart_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;
    pt_map_page(ctx, PLATFORM_UART0_BASE, PLATFORM_UART0_BASE,
                uart_flags, PT_LEVEL_4K);

    return 0;
}

/* ===================================================================
 * S-mode test functions (identical to svade helpers)
 *
 * Return 0 on success, scause on trap.
 * =================================================================== */
static uintptr_t test_smode_load(uintptr_t arg) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

static uintptr_t test_smode_store(uintptr_t arg) {
    trap_expect_begin();
    *(volatile uintptr_t *)arg = MAGIC_WRITE;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

static uintptr_t test_smode_exec(uintptr_t arg) {
    trap_expect_begin();
    exec_at(arg);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

static uintptr_t test_smode_load_and_store(uintptr_t arg) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)arg;

    trap_expect_begin();
    uintptr_t val = ptr[0];
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();

    trap_expect_begin();
    ptr[0] = MAGIC_WRITE;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();

    return 0;
}

static uintptr_t test_smode_amoadd(uintptr_t arg) {
    uint32_t result;
    trap_expect_begin();
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.w %0, %1, (%2)\n\t"
        ".option pop\n\t"
        : "=r"(result)
        : "r"(1), "r"(arg)
        : "memory"
    );
    (void)result;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * PTE inspection / modification helpers (identical to svade helpers)
 * =================================================================== */
static uintptr_t pte_read(pt_context_t *ctx, uintptr_t va, int level) {
    uintptr_t *p = pt_get_pte(ctx, va, level);
    return p ? *p : 0;
}

static void pte_set_bits(pt_context_t *ctx, uintptr_t va, int level,
                         uintptr_t bits) {
    uintptr_t *p = pt_get_pte(ctx, va, level);
    if (p) {
        *p |= bits;
        vm_sfence_vma(0, 0);
    }
}

/* ===================================================================
 * Svadu detection (two-step)
 *
 * Step 1: Write menvcfg.ADUE=1 and read back. Per SPEC/machine.adoc:2247
 *         (norm:menvcfg_adue_rdonly0), if Svadu is not implemented then
 *         ADUE is read-only zero; readback==0 => not implemented.
 *
 * Step 2: Map a 4K leaf PTE with A=0 and perform an S-mode load.
 *         If Svadu is implemented and ADUE=1, the load must succeed
 *         and PTE.A must be set by HW. Any other outcome => not Svadu.
 * =================================================================== */
static bool svadu_detected = false;
static bool svadu_detection_done = false;

static bool detect_svadu(void) {
    if (svadu_detection_done)
        return svadu_detected;

    /* Step 1: writable check */
    set_menvcfg_adue(1);
    if (get_menvcfg_adue() == 0) {
        svadu_detected = false;
        svadu_detection_done = true;
        return false;
    }

    /* Step 2: A=0 load under ADUE=1 must succeed and set A */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    if (setup_code_mapping(&ctx) != 0) {
        svadu_detected = false;
        svadu_detection_done = true;
        pt_pool_reset();
        return false;
    }

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_D,  /* A=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    uintptr_t pte    = pte_read(&ctx, test_va, PT_LEVEL_4K);
    pt_pool_reset();

    svadu_detected = (result == 0) && ((pte & PTE_A) != 0);
    svadu_detection_done = true;
    return svadu_detected;
}

#define SVADU_REQUIRED_OR_SKIP() do { \
    if (!detect_svadu()) { \
        TEST_SKIP("Platform does not implement Svadu"); \
    } \
} while (0)

#endif /* SVADU_TEST_HELPERS_H */
