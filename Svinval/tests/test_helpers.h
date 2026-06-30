/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Svinval test cases
 *
 * Provides shared test data areas, S-mode test functions, and
 * utility functions used by all Svinval test files.
 *
 * Design: All test files are #included into test_register.c,
 * so static functions and variables are visible across all tests
 * within the same compilation unit.
 */

#ifndef SVINVAL_TEST_HELPERS_H
#define SVINVAL_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "pmp_cfg.h"
#include "mem_ops.h"
#include "svinval_insn.h"

/* ===================================================================
 * Test data and executable regions
 *
 * These pages are placed in a separate 2MB-aligned region by the
 * linker script (.vm_test_region section). This ensures they are
 * NOT covered by setup_code_mapping()'s 2MB megapages, allowing
 * individual tests to create 4KB mappings with custom permissions.
 * =================================================================== */
extern uintptr_t __vm_test_region_start;

/* Page 0: test data area for read/write verification */
#define test_data_area  ((volatile uintptr_t *)&__vm_test_region_start)

/* Page 1: test page for permission/fault tests */
#define test_fault_page ((volatile uint8_t *)((uintptr_t)&__vm_test_region_start + PAGE_SIZE_4K))

/* Page 2: test page for additional tests */
#define test_extra_page ((volatile uint8_t *)((uintptr_t)&__vm_test_region_start + 2 * PAGE_SIZE_4K))

/* Magic values for verification */
#define MAGIC_WRITE  0xDEADBEEF12345678UL
#define MAGIC_READ   0xCAFEBABE87654321UL

/* ===================================================================
 * Common helper: set up identity mapping for code execution
 *
 * Maps memory at MEM_BASE with full RWX permissions using 2MB
 * megapages, but SKIPS the 2MB region containing the test pages
 * (.vm_test_region). This allows individual tests to create 4KB
 * mappings in that region with custom permissions.
 *
 * Also maps the UART I/O region for S-mode printf support.
 * =================================================================== */
static int setup_code_mapping(pt_context_t *ctx) {
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);

    /* Determine which 2MB region contains the test pages */
    uintptr_t test_region = (uintptr_t)&__vm_test_region_start;
    uintptr_t test_region_2m = test_region & ~(PAGE_SIZE_2M - 1);

    /* Map enough 2MB pages to cover code, stack, and page table pool.
     * We map from base up to test_region_2m (exclusive), then skip
     * the test region, then map one more 2MB page after it. */
    for (uintptr_t addr = base; addr < test_region_2m; addr += PAGE_SIZE_2M) {
        int ret = pt_map_page(ctx, addr, addr, flags, PT_LEVEL_2M);
        if (ret != 0) return ret;
    }

    /* Map one 2MB page after the test region (for safety) */
    uintptr_t after_test = test_region_2m + PAGE_SIZE_2M;
    pt_map_page(ctx, after_test, after_test, flags, PT_LEVEL_2M);

    /* Map UART I/O region for S-mode printf support */
    uintptr_t uart_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;
    pt_map_page(ctx, PLATFORM_UART0_BASE, PLATFORM_UART0_BASE,
                uart_flags, PT_LEVEL_4K);

    return 0;
}

/* ===================================================================
 * S-mode test functions
 *
 * These functions execute in S-mode with VM enabled via
 * vm_run_in_smode(). They use trap_expect_begin/end to detect faults.
 * =================================================================== */

/**
 * smode_load - Execute a load, return 0 on success or scause on fault
 */
static uintptr_t smode_load(uintptr_t addr) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)addr;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * smode_store - Execute a store, return 0 on success or scause on fault
 */
static uintptr_t smode_store(uintptr_t addr) {
    trap_expect_begin();
    *(volatile uintptr_t *)addr = MAGIC_WRITE;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * smode_store_expect_fault - Execute a store, expecting a fault.
 * Returns scause if fault occurred, 0 if no fault.
 */
static uintptr_t smode_store_expect_fault(uintptr_t addr) {
    trap_expect_begin();
    *(volatile uintptr_t *)addr = 0xDEADBEEF;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * smode_load_expect_fault - Execute a load, expecting a fault.
 * Returns scause if fault occurred, 0 if no fault.
 */
static uintptr_t smode_load_expect_fault(uintptr_t addr) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)addr;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * smode_read_write - Write magic value and read back to verify.
 * Returns 0 on success, 1 on mismatch, or scause on fault.
 */
static uintptr_t smode_read_write(uintptr_t addr) {
    trap_expect_begin();
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = MAGIC_WRITE;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();

    trap_expect_begin();
    uintptr_t val = *ptr;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();

    if (val != MAGIC_WRITE)
        return 1;
    return 0;
}

/* ===================================================================
 * Global variables for S-mode PTE manipulation
 *
 * Used by tests that need to modify PTEs from within S-mode.
 * =================================================================== */
static volatile uintptr_t *g_pte_addr __attribute__((unused));
static volatile uintptr_t g_test_va __attribute__((unused));

#endif /* SVINVAL_TEST_HELPERS_H */
