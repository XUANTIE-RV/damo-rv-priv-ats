/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Sstvala extension test cases
 *
 * Provides shared test data areas, S-mode test functions, and
 * page table setup utilities used by all Sstvala test files.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 *
 * Sstvala semantics (Trap Value Reporting, Version 1.0):
 *   - Address-type exceptions (page-fault, access-fault, misaligned,
 *     non-EBREAK breakpoint): stval = faulting virtual address
 *   - Instruction-type exceptions (illegal-instruction,
 *     virtual-instruction): stval = faulting instruction encoding
 */

#ifndef SSTVALA_TEST_HELPERS_H
#define SSTVALA_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "pmp/pmp_cfg.h"
#include "mem_ops.h"

/* ===================================================================
 * Test data and executable regions
 *
 * .vm_test_region    : 3 x 4KB pages (data / fault / exec) - 4K tests
 * .vm_test_region_2m : 2 MiB region (for setup_code_mapping skip)
 * =================================================================== */
extern uintptr_t __vm_test_region_start;
extern uintptr_t __vm_test_region_2m_start;

/* 4K test pages (within .vm_test_region) */
#define test_data_area  ((volatile uintptr_t *)&__vm_test_region_start)
#define test_fault_page ((volatile uint8_t *)((uintptr_t)&__vm_test_region_start + PAGE_SIZE_4K))
#define test_exec_page  ((uint8_t *)((uintptr_t)&__vm_test_region_start + 2 * PAGE_SIZE_4K))

/* ===================================================================
 * Delegation helper: delegate Sstvala-relevant exceptions to S-mode
 *
 * Sstvala specifies stval behavior for these exception causes:
 *   cause 0: instruction address misaligned
 *   cause 2: illegal instruction
 *   cause 3: breakpoint
 *   cause 4: load address misaligned
 *   cause 6: store address misaligned
 *
 * Page-fault and access-fault delegation is handled per-test.
 * =================================================================== */
static void sstvala_delegate_exceptions(void) {
    uintptr_t deleg = BIT(CAUSE_INST_ADDR_MISALIGN)   /* cause 0 */
                    | BIT(CAUSE_ILLEGAL_INST)          /* cause 2 */
                    | BIT(CAUSE_BREAKPOINT)            /* cause 3 */
                    | BIT(CAUSE_LOAD_ADDR_MISALIGN)    /* cause 4 */
                    | BIT(CAUSE_STORE_ADDR_MISALIGN);  /* cause 6 */
    CSRW(medeleg, CSRR(medeleg) | deleg);
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
 * Maps memory at PLATFORM_MEM_BASE with full RWX permissions using
 * 2 MiB megapages, but SKIPS the 2 MiB region(s) containing test
 * pages so that individual tests can install their own mappings with
 * custom permissions.
 *
 * Also maps the UART I/O region for S-mode printf support.
 * =================================================================== */
static int setup_code_mapping(pt_context_t *ctx) {
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);

    uintptr_t test_4k_2m  = (uintptr_t)&__vm_test_region_start    & ~(PAGE_SIZE_2M - 1);
    uintptr_t test_2m_2m  = (uintptr_t)&__vm_test_region_2m_start & ~(PAGE_SIZE_2M - 1);

    /* Map 2 MiB pages from base, skipping the two test 2 MiB regions */
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
 * S-mode payload functions
 *
 * These functions execute in S-mode with VM enabled. They use
 * trap_expect_begin/end to detect faults. The trap_record (including
 * tval) is captured by the M-mode/S-mode trap handler and accessible
 * after returning to M-mode via trap_get_tval().
 * =================================================================== */

/**
 * smode_load_addr - Execute a load at arg. Returns scause on trap, 0 on success.
 */
static uintptr_t smode_load_addr(uintptr_t arg) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * smode_store_addr - Execute a store at arg. Returns scause on trap, 0 on success.
 */
static uintptr_t smode_store_addr(uintptr_t arg) {
    trap_expect_begin();
    *(volatile uintptr_t *)arg = 0xDEADBEEFUL;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * smode_exec_addr - Jump to arg and execute. Returns scause on trap, 0 on success.
 */
static uintptr_t smode_exec_addr(uintptr_t arg) {
    trap_expect_begin();
    exec_at(arg);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * PMP helper: ensure S-mode has full memory access
 *
 * Some tests (e.g., access-fault) reconfigure or clear PMP entries.
 * Call this before goto_priv(PRIV_S) in tests that need S-mode
 * access without custom PMP restrictions.
 * Uses PMP entry 0 with NAPOT covering the entire address space.
 * =================================================================== */
static void ensure_smode_pmp(void) {
    pmp_clear_all();
    pmp_entry_t pmp_all = PMP_ENTRY_FULL(PMP_RWX);
    pmp_set_entry(0, &pmp_all);
}

#endif /* SSTVALA_TEST_HELPERS_H */
