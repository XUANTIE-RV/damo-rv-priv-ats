/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Sv48 VM test cases
 *
 * Provides shared test data areas, S-mode test functions, and
 * utility functions used by all Sv48 test files.
 *
 * Design: All test files are #included into test_register.c,
 * so static functions and variables are visible across all tests
 * within the same compilation unit.
 */

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "mem_ops.h"

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

/* Page 2: test page filled with executable instructions (nop; ret) */
#define test_exec_page  ((uint8_t *)((uintptr_t)&__vm_test_region_start + 2 * PAGE_SIZE_4K))

/* Magic values for verification */
#define MAGIC_WRITE  0xDEADBEEF12345678UL
#define MAGIC_READ   0xCAFEBABE87654321UL

/* ===================================================================
 * Initialization helper: fill exec page with nop;ret
 * =================================================================== */
static void init_exec_page(void) {
    uint32_t *p = (uint32_t *)test_exec_page;
    /* Fill with nop instructions */
    for (int i = 0; i < 1024 - 1; i++)
        p[i] = 0x00000013;  /* nop (addi x0, x0, 0) */
    /* Last instruction: ret (jalr x0, ra, 0) */
    p[1023] = 0x00008067;   /* ret */
}

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
 * These functions execute in S-mode with VM enabled.
 * They use trap_expect_begin/end to detect faults.
 * =================================================================== */

/**
 * test_smode_read_write - Basic read/write test in S-mode
 * Returns 0 on success, non-zero on failure.
 */
static uintptr_t test_smode_read_write(uintptr_t arg) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)arg;

    ptr[0] = MAGIC_WRITE;
    if (ptr[0] != MAGIC_WRITE)
        return 1;

    ptr[1] = MAGIC_READ;
    if (ptr[1] != MAGIC_READ)
        return 2;

    for (int i = 0; i < 4; i++)
        ptr[i] = (uintptr_t)i * 0x1111111111111111UL;

    for (int i = 0; i < 4; i++) {
        if (ptr[i] != (uintptr_t)i * 0x1111111111111111UL)
            return 3;
    }

    return 0;
}

/**
 * test_smode_load - Execute a load, return 0 on success
 * arg = address to load from
 */
static uintptr_t test_smode_load(uintptr_t arg) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_store - Execute a store, return 0 on success
 * arg = address to store to
 */
static uintptr_t test_smode_store(uintptr_t arg) {
    trap_expect_begin();
    *(volatile uintptr_t *)arg = MAGIC_WRITE;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_exec - Jump to address and execute, return 0 on success
 * arg = address to execute (must contain nop;ret)
 *
 * Uses exec_at() which sets _exec_return_addr for trap recovery.
 * This is critical because if the target page has no read permission,
 * next_instruction() in the trap handler would cause a secondary fault.
 */
static uintptr_t test_smode_exec(uintptr_t arg) {
    trap_expect_begin();
    exec_at(arg);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_load_and_store - Execute load then store, return 0 on success
 * arg = address
 */
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

/* ===================================================================
 * S-mode test functions with SUM/MXR control
 * =================================================================== */

/**
 * test_smode_load_with_sum - Load with SUM=1
 */
static uintptr_t test_smode_load_with_sum(uintptr_t arg) {
    CSRS(sstatus, MSTATUS_SUM_BIT);
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    CSRC(sstatus, MSTATUS_SUM_BIT);
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_store_with_sum - Store with SUM=1
 */
static uintptr_t test_smode_store_with_sum(uintptr_t arg) {
    CSRS(sstatus, MSTATUS_SUM_BIT);
    trap_expect_begin();
    *(volatile uintptr_t *)arg = MAGIC_WRITE;
    trap_expect_end();
    CSRC(sstatus, MSTATUS_SUM_BIT);
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_exec_with_sum - Execute with SUM=1
 *
 * Uses exec_at() for safe trap recovery (see test_smode_exec).
 */
static uintptr_t test_smode_exec_with_sum(uintptr_t arg) {
    CSRS(sstatus, MSTATUS_SUM_BIT);
    trap_expect_begin();
    exec_at(arg);
    trap_expect_end();
    CSRC(sstatus, MSTATUS_SUM_BIT);
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_load_with_mxr - Load with MXR=1
 */
static uintptr_t test_smode_load_with_mxr(uintptr_t arg) {
    CSRS(sstatus, MSTATUS_MXR_BIT);
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    CSRC(sstatus, MSTATUS_MXR_BIT);
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

#endif /* TEST_HELPERS_H */
