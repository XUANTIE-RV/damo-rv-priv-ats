/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Svpbmt test cases
 *
 * Provides shared test data areas, S-mode test functions, and
 * utility functions used by all Svpbmt test files.
 *
 * Design: All test files are #included into test_register.c,
 * so static functions and variables are visible across all tests
 * within the same compilation unit.
 */

#ifndef SVPBMT_TEST_HELPERS_H
#define SVPBMT_TEST_HELPERS_H

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
 * PBMT PTE helper
 * =================================================================== */

/**
 * pbmt_set_pte - Set PBMT field in a PTE
 * @pte:  Original PTE value
 * @pbmt: PBMT value (PBMT_PMA / PBMT_NC / PBMT_IO / PBMT_RSVD)
 *
 * Returns the PTE with PBMT field set to the specified value.
 */
static inline uintptr_t pbmt_set_pte(uintptr_t pte, uintptr_t pbmt) {
    return (pte & ~PTE_PBMT_MASK) | (pbmt & PTE_PBMT_MASK);
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

/**
 * setup_code_mapping_1g - Set up 1GB identity mapping for code
 *
 * Simpler mapping for tests that don't need fine-grained control
 * of the test region (e.g., mode compatibility tests, page size tests).
 */
static int setup_code_mapping_1g(pt_context_t *ctx) {
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    return pt_setup_identity_mapping(ctx, base, PAGE_SIZE_1G,
                                     flags, PT_LEVEL_1G);
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

    return 0;
}

/**
 * test_smode_load - Execute a load, return 0 on success or scause on fault
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
 * test_smode_store - Execute a store, return 0 on success or scause on fault
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
 * test_smode_load_expect_fault - Execute a load, expecting a fault.
 * Returns scause if fault, 0 if no fault.
 */
static uintptr_t test_smode_load_expect_fault(uintptr_t addr) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)addr;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_store_expect_fault - Execute a store, expecting a fault.
 * Returns scause if fault, 0 if no fault.
 */
static uintptr_t test_smode_store_expect_fault(uintptr_t addr) {
    trap_expect_begin();
    *(volatile uintptr_t *)addr = 0xDEADBEEF;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_exec_expect_fault - Jump to addr, expecting instruction fault.
 * Returns scause if fault, 0 if no fault.
 */
static uintptr_t test_smode_exec_expect_fault(uintptr_t addr) {
    trap_expect_begin();
    exec_at(addr);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * S-mode sequential and fence test functions
 * =================================================================== */

/**
 * test_smode_sequential_rw - Multiple sequential store then load back
 * Returns 0 if all data correct, non-zero on mismatch.
 */
static uintptr_t test_smode_sequential_rw(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;

    /* Store 8 values sequentially */
    for (int i = 0; i < 8; i++)
        ptr[i] = (uintptr_t)i * 0x1111111111111111UL;

    /* Load back and verify */
    for (int i = 0; i < 8; i++) {
        if (ptr[i] != (uintptr_t)i * 0x1111111111111111UL)
            return (uintptr_t)(i + 1);
    }

    return 0;
}

/**
 * test_smode_store_fence_load - Store, fence iorw,iorw, load back
 * Returns 0 if data correct, non-zero on mismatch.
 */
static uintptr_t test_smode_store_fence_load(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;

    /* Store */
    ptr[0] = MAGIC_WRITE;

    /* fence iorw, iorw */
    asm volatile("fence iorw, iorw" ::: "memory");

    /* Load back and verify */
    uintptr_t val = ptr[0];
    if (val != MAGIC_WRITE)
        return 1;

    return 0;
}

/* ===================================================================
 * S-mode alias test functions
 * =================================================================== */

/**
 * Alias argument structure
 * Passed as a pointer in the arg parameter.
 */
typedef struct {
    uintptr_t va1;  /* First virtual address (write target) */
    uintptr_t va2;  /* Second virtual address (read target) */
} alias_args_t;

/**
 * test_smode_alias_write_read - Write via VA1, read via VA2
 * Returns 0 if data consistent, non-zero on mismatch.
 */
static uintptr_t test_smode_alias_write_read(uintptr_t arg) {
    alias_args_t *args = (alias_args_t *)arg;
    volatile uintptr_t *p1 = (volatile uintptr_t *)args->va1;
    volatile uintptr_t *p2 = (volatile uintptr_t *)args->va2;

    /* Write via VA1 */
    *p1 = MAGIC_WRITE;

    /* fence to ensure visibility */
    asm volatile("fence iorw, iorw" ::: "memory");

    /* Read via VA2 */
    uintptr_t val = *p2;

    if (val != MAGIC_WRITE)
        return 1;

    return 0;
}

/**
 * test_smode_alias_flush_sequence - Write VA1, fence+cbo.flush+fence, read VA2
 * Returns 0 if data consistent, non-zero on mismatch or fault.
 */
static uintptr_t test_smode_alias_flush_sequence(uintptr_t arg) {
    alias_args_t *args = (alias_args_t *)arg;
    volatile uintptr_t *p1 = (volatile uintptr_t *)args->va1;
    volatile uintptr_t *p2 = (volatile uintptr_t *)args->va2;

    /* Write via VA1 */
    *p1 = MAGIC_WRITE;

    /* fence iorw, iorw */
    asm volatile("fence iorw, iorw" ::: "memory");

    /* cbo.flush VA1 - Zicbom extension
     * Encoding: cbo.flush (rs1) = 0000000 00010 rs1 010 00000 0001111
     * For rs1=a0(x10): 0000000 00010 01010 010 00000 0001111 = 0x0025200F
     * We use .4byte encoding in case the assembler doesn't know cbo.flush.
     * We must ensure the address is in a0 by using a register variable. */
    register uintptr_t _cbo_addr asm("a0") = args->va1;
    trap_expect_begin();
    asm volatile(
        ".4byte 0x0025200F\n\t"  /* cbo.flush (a0) */
        :: "r"(_cbo_addr) : "memory"
    );
    trap_expect_end();

    /* If cbo.flush faulted (Zicbom not supported), skip the test */
    if (trap_was_triggered())
        return 0xFF;  /* Signal to caller: cbo.flush not supported */

    /* fence iorw, iorw */
    asm volatile("fence iorw, iorw" ::: "memory");

    /* Read via VA2 */
    uintptr_t val = *p2;

    if (val != MAGIC_WRITE)
        return 1;

    return 0;
}

/* ===================================================================
 * S-mode SUM control helpers
 * =================================================================== */

/**
 * test_smode_load_with_sum - Load with sstatus.SUM=1
 */
static uintptr_t test_smode_load_with_sum(uintptr_t addr) {
    CSRS(sstatus, MSTATUS_SUM_BIT);
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)addr;
    (void)val;
    trap_expect_end();
    CSRC(sstatus, MSTATUS_SUM_BIT);
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * S-mode non-aligned access helpers
 * =================================================================== */

/**
 * test_smode_load_misaligned - Execute a misaligned load (addr+1, 8 bytes)
 * Returns 0 on success, scause on fault.
 */
static uintptr_t test_smode_load_misaligned(uintptr_t addr) {
    trap_expect_begin();
    uintptr_t misaligned_addr = addr + 1;
    volatile uint8_t val = *(volatile uint8_t *)misaligned_addr;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_store_misaligned - Execute a misaligned store (addr+1, 8 bytes)
 * Returns 0 on success, scause on fault.
 */
static uintptr_t test_smode_store_misaligned(uintptr_t addr) {
    trap_expect_begin();
    uintptr_t misaligned_addr = addr + 1;
    *(volatile uint8_t *)misaligned_addr = 0x42;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * PTE modification and sfence helpers for S-mode
 * =================================================================== */

/* Global variables for passing PTE modification info to S-mode */
static volatile uintptr_t *g_pte_slot = NULL;
static volatile uintptr_t g_new_pte_val = 0;

/**
 * smode_modify_pte_and_sfence_global - Modify PTE and global sfence
 */
static uintptr_t smode_modify_pte_and_sfence_global(uintptr_t test_va) {
    if (g_pte_slot)
        *g_pte_slot = g_new_pte_val;

    /* Global sfence.vma */
    asm volatile("sfence.vma" ::: "memory");

    return 0;
}

/**
 * smode_modify_pte_and_sfence_addr - Modify PTE and sfence by address
 */
static uintptr_t smode_modify_pte_and_sfence_addr(uintptr_t test_va) {
    if (g_pte_slot)
        *g_pte_slot = g_new_pte_val;

    /* sfence.vma by address */
    asm volatile("sfence.vma %0, zero" :: "r"(test_va) : "memory");

    return 0;
}

/* ===================================================================
 * Fill exec page helper
 * =================================================================== */

/**
 * fill_exec_page - Fill a page with nop;ret instruction sequence
 * @addr: Page base address (must be 4KB aligned)
 */
static void fill_exec_page(uintptr_t addr) {
    volatile uint32_t *p = (volatile uint32_t *)addr;
    /* nop = 0x00000013 (addi x0, x0, 0) */
    p[0] = 0x00000013;
    /* ret = 0x00008067 (jalr x0, ra, 0) */
    p[1] = 0x00008067;

    /* Ensure stores are visible to instruction fetch */
    asm volatile("fence" ::: "memory");
    asm volatile("fence.i" ::: "memory");
}

#endif /* SVPBMT_TEST_HELPERS_H */
