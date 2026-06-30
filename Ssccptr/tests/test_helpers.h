/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Ssccptr extension test cases
 *
 * Provides shared test data areas, S-mode test functions, code mapping
 * helpers for Sv39/Sv48/Sv57, and PTE inspection/modification utilities.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 *
 * Ssccptr semantics:
 *   Main memory regions with cacheability and coherence PMA must support
 *   hardware page-table reads. We verify this indirectly by setting up
 *   page tables in main memory and confirming page walks succeed.
 */

#ifndef SSCCPTR_TEST_HELPERS_H
#define SSCCPTR_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "pmp_cfg.h"
#include "mem_ops.h"

/* ===================================================================
 * Test data and executable regions
 *
 * .vm_test_region    : 3 x 4KB pages (data / fault / exec) - 4K tests
 * .vm_test_region_2m : 2 MiB region - megapage tests
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
#define MAGIC_WRITE_A  0xDEADBEEF12345678UL
#define MAGIC_WRITE_B  0xCAFEBABE87654321UL

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
 * setup_code_mapping - Sv39 identity mapping for code execution
 *
 * Maps memory at PLATFORM_MEM_BASE with full RWX permissions using
 * 2 MiB megapages, but SKIPS the 2 MiB region(s) containing test
 * pages so individual tests can install custom mappings.
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
 * setup_code_mapping_sv48 / _sv57 - Sv48/Sv57 identity mapping
 *
 * Same logic as setup_code_mapping but for higher-level page tables.
 * The page table context must already be initialized with the correct
 * satp mode.
 * =================================================================== */
static int setup_code_mapping_sv48(pt_context_t *ctx) {
    return setup_code_mapping(ctx);
}

static int setup_code_mapping_sv57(pt_context_t *ctx) {
    return setup_code_mapping(ctx);
}

/* ===================================================================
 * U-mode code mapping variant: adds PTE_U to all pages
 * =================================================================== */
static int setup_code_mapping_umode(pt_context_t *ctx) {
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D;
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

    uintptr_t uart_flags = PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D;
    pt_map_page(ctx, PLATFORM_UART0_BASE, PLATFORM_UART0_BASE,
                uart_flags, PT_LEVEL_4K);

    return 0;
}

/* ===================================================================
 * S-mode test functions
 *
 * Execute in S-mode with VM enabled. Use trap_expect_begin/end to
 * detect faults. Return scause on trap, 0 on success.
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
    *(volatile uintptr_t *)arg = MAGIC_WRITE_A;
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

/* ===================================================================
 * S-mode function that loads and returns the value read
 * =================================================================== */
static uintptr_t test_smode_load_value(uintptr_t arg) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    trap_expect_end();
    if (trap_was_triggered())
        return (uintptr_t)-1;
    return val;
}

/* ===================================================================
 * Sv mode support detection
 *
 * Attempts to write satp with the specified mode. If the hardware
 * does not support that mode, satp.MODE reads back as Bare (0).
 * =================================================================== */
static bool sv_mode_detected[16] = {false};
static bool sv_mode_checked[16]  = {false};

static bool is_sv_mode_supported(int mode) {
    if (mode >= 16) return false;
    if (sv_mode_checked[mode])
        return sv_mode_detected[mode];

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, mode);

    /* Try to enable VM with this mode */
    uintptr_t root_ppn = (uintptr_t)ctx.root_pt >> PAGE_SHIFT;
    uintptr_t satp_val = MAKE_SATP(mode, 0, root_ppn);

    CSRW(satp, satp_val);
    uintptr_t readback = CSRR(satp);
    CSRW(satp, 0);  /* restore Bare mode */
    vm_sfence_vma(0, 0);

    int readback_mode = (int)(readback >> SATP_MODE_SHIFT);
    sv_mode_detected[mode] = (readback_mode == mode);
    sv_mode_checked[mode] = true;

    pt_pool_reset();
    return sv_mode_detected[mode];
}

static bool is_sv48_supported(void) {
    return is_sv_mode_supported(SATP_MODE_SV48);
}

static bool is_sv57_supported(void) {
    return is_sv_mode_supported(SATP_MODE_SV57);
}

/* ===================================================================
 * Superpage alignment feasibility check
 * =================================================================== */
static bool is_superpage_feasible(int pt_level) {
    uintptr_t pgsz;
    switch (pt_level) {
    case PT_LEVEL_4K:  return true;
    case PT_LEVEL_2M:  return true;
    case PT_LEVEL_1G:
        pgsz = PAGE_SIZE_1G;
        break;
    case PT_LEVEL_512G:
        pgsz = (uintptr_t)1 << 39;  /* 512 GiB */
        break;
    case PT_LEVEL_256T:
        pgsz = (uintptr_t)1 << 48;  /* 256 TiB */
        break;
    default:
        return false;
    }
    return (PLATFORM_MEM_BASE & (pgsz - 1)) == 0;
}

/* ===================================================================
 * PTE inspection / modification helpers
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

static void pte_clear_bits(pt_context_t *ctx, uintptr_t va, int level,
                           uintptr_t bits) {
    uintptr_t *p = pt_get_pte(ctx, va, level);
    if (p) {
        *p &= ~bits;
        vm_sfence_vma(0, 0);
    }
}

static void pte_write(pt_context_t *ctx, uintptr_t va, int level,
                      uintptr_t value) {
    uintptr_t *p = pt_get_pte(ctx, va, level);
    if (p) {
        *p = value;
        vm_sfence_vma(0, 0);
    }
}

#endif /* SSCCPTR_TEST_HELPERS_H */
