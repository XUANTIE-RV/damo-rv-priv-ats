/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Svnapot test cases
 *
 * Provides NAPOT PTE construction/installation functions, S-mode test
 * functions, and utility macros used by all Svnapot test files.
 *
 * Design: All test files are #included into test_register.c,
 * so static functions and variables are visible across all tests
 * within the same compilation unit.
 */

#ifndef SVNAPOT_TEST_HELPERS_H
#define SVNAPOT_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "mem_ops.h"

/* ===================================================================
 * NAPOT PTE Constants
 * =================================================================== */

/* PTE N bit (bit 63) - enables NAPOT translation contiguity */
#define PTE_N               (1UL << 63)

/* NAPOT 64 KiB region parameters */
#define NAPOT_64K_PAGES     16                          /* 16 x 4 KiB pages */
#define NAPOT_64K_SIZE      (64 * 1024)                 /* 64 KiB = 0x10000 */
#define NAPOT_64K_MASK      (NAPOT_64K_SIZE - 1)        /* 0xFFFF */
#define NAPOT_BITS_64K      4                           /* napot_bits = 4 */

/* ppn[0] encoding for 64 KiB NAPOT: low 4 bits = 1000 (0x8) */
#define NAPOT_64K_PPN0_ENC  0x8UL

/* PTE RSW field (bits 9-8) */
#define PTE_RSW_SHIFT       8
#define PTE_RSW_MASK        (0x3UL << PTE_RSW_SHIFT)

/* PTE flags mask (bits 9:0, includes RSW) */
#define PTE_ALL_FLAGS_MASK  0x3FFUL

/* ===================================================================
 * Test data regions
 *
 * These are placed in the .vm_test_region section by the linker
 * script (64 KiB aligned, 128 KiB total = 2 x 64 KiB regions).
 * =================================================================== */
extern uintptr_t __vm_test_region_start;
extern uintptr_t __vm_test_region_end;

/* First 64 KiB NAPOT region (pages 0-15) */
#define NAPOT_TEST_REGION_0     ((uintptr_t)&__vm_test_region_start)

/* Second 64 KiB NAPOT region (pages 16-31) */
#define NAPOT_TEST_REGION_1     (NAPOT_TEST_REGION_0 + NAPOT_64K_SIZE)

/* Magic values for verification */
#define MAGIC_WRITE         0xDEADBEEF12345678UL
#define MAGIC_READ          0xCAFEBABE87654321UL

/* ===================================================================
 * NAPOT PTE Construction Functions
 * =================================================================== */

/**
 * napot_make_pte - Construct a valid 64 KiB NAPOT PTE
 * @pa:    Physical address (must be 64 KiB aligned)
 * @flags: PTE flags (PTE_V | PTE_R | PTE_W | etc.)
 *
 * Constructs a NAPOT PTE with N=1 and ppn[0] low 4 bits = 1000.
 * The caller must ensure PA is 64 KiB aligned.
 *
 * PTE layout:
 *   bit 63:     N = 1
 *   bits 53-10: PPN (with ppn[0][3:0] = 1000)
 *   bits 9-0:   flags (RSW + DAGUXWRV)
 */
static inline uintptr_t napot_make_pte(uintptr_t pa, uintptr_t flags) {
    uintptr_t ppn = pa >> PAGE_SHIFT;
    /* Set ppn[0] low 4 bits to 1000 for 64 KiB NAPOT encoding */
    ppn = (ppn & ~0xFUL) | NAPOT_64K_PPN0_ENC;
    return PTE_N | (ppn << PTE_PPN_SHIFT) | (flags & PTE_ALL_FLAGS_MASK);
}

/**
 * napot_make_reserved_pte - Construct a NAPOT PTE with reserved encoding
 * @pa:        Physical address
 * @flags:     PTE flags
 * @ppn0_low4: Low 4 bits of ppn[0] (must be a reserved encoding)
 *
 * Used for Group 3 (RSVD-01~07) reserved encoding tests.
 */
static inline uintptr_t napot_make_reserved_pte(uintptr_t pa,
                                                 uintptr_t flags,
                                                 unsigned ppn0_low4) {
    uintptr_t ppn = pa >> PAGE_SHIFT;
    ppn = (ppn & ~0xFUL) | (ppn0_low4 & 0xFUL);
    return PTE_N | (ppn << PTE_PPN_SHIFT) | (flags & PTE_ALL_FLAGS_MASK);
}

/**
 * napot_make_pte_at_level - Construct a NAPOT PTE at a specified level
 * @pa:    Physical address (must be aligned to level page size)
 * @flags: PTE flags
 * @level: Page table level (PT_LEVEL_2M=1, PT_LEVEL_1G=2)
 *
 * Used for RSVD-08/09: N=1 at level >= 1 (all encodings reserved).
 */
static inline uintptr_t napot_make_pte_at_level(uintptr_t pa,
                                                  uintptr_t flags,
                                                  int level) {
    uintptr_t ppn = pa >> PAGE_SHIFT;
    /* At level >= 1, all ppn encodings with N=1 are reserved.
     * We use the standard ppn encoding (no special low bits). */
    return PTE_N | (ppn << PTE_PPN_SHIFT) | (flags & PTE_ALL_FLAGS_MASK);
}

/* ===================================================================
 * NAPOT PTE Installation Functions
 * =================================================================== */

/**
 * napot_install_pte - Install a NAPOT PTE into the page table at level 0
 * @ctx:  Page table context
 * @va:   Virtual address (must be 64 KiB aligned for valid NAPOT)
 * @pte:  Pre-constructed NAPOT PTE value
 *
 * Ensures intermediate page table pages exist by creating a dummy
 * 4KB mapping first, then overwrites the level-0 PTE slot with the
 * NAPOT PTE. This single NAPOT PTE at the base VA's slot is sufficient
 * because QEMU's TLB will use the NAPOT encoding to cover the entire
 * 64 KiB region from any access within it.
 *
 * For implementations that split NAPOT into 16 individual TLB entries,
 * we also install the same NAPOT PTE value into all 16 level-0 slots
 * covering the 64 KiB region, ensuring correct behavior regardless
 * of implementation strategy.
 */
static void napot_install_pte(pt_context_t *ctx, uintptr_t va,
                               uintptr_t pte_val) {
    if (!ctx->root_pt) {
        printf("ERROR: napot_install_pte: root page table is NULL\n");
        return;
    }

    /* Ensure VA is 64 KiB aligned */
    uintptr_t base_va = va & ~NAPOT_64K_MASK;

    /* Use pt_map_page to create the page table path (allocates
     * intermediate PT pages as needed). We map the base VA first. */
    int ret = pt_map_page(ctx, base_va, base_va,
                          PTE_V | PTE_R, PT_LEVEL_4K);
    if (ret != 0) {
        printf("ERROR: napot_install_pte: pt_map_page failed\n");
        return;
    }

    /* Find the level-0 page table page */
    uintptr_t pt_page_addr = get_pt_page_addr(ctx, base_va, PT_LEVEL_4K);
    if (pt_page_addr == 0) {
        printf("ERROR: napot_install_pte: could not find L0 PT page\n");
        return;
    }
    uintptr_t *l0_pt = (uintptr_t *)pt_page_addr;

    /* Install the NAPOT PTE into the base VA's slot */
    uintptr_t vpn0_base = VA_VPN0(base_va);
    l0_pt[vpn0_base] = pte_val;

    /* Also install NAPOT PTE aliases for all 16 slots in the 64 KiB region.
     * This ensures that any access within the region hits a NAPOT PTE,
     * regardless of which 4 KiB page within the region is accessed.
     *
     * All 16 slots share the same level-0 page table page because they
     * are within the same 2MB region (VPN[1] is the same).
     *
     * We need to ensure the other 15 pages also have their PT paths.
     * Since they share VPN[1] with base_va, the intermediate pages
     * already exist. We just need to check they're in the same L0 page. */
    for (int i = 1; i < 16; i++) {
        uintptr_t offset_va = base_va + (uintptr_t)i * PAGE_SIZE_4K;
        uintptr_t vpn0 = VA_VPN0(offset_va);

        /* Verify this offset uses the same L0 page table page.
         * Within a 64 KiB region (< 2MB), VPN[1] and above are the same,
         * so VPN[0] just indexes into the same L0 page. */
        l0_pt[vpn0] = pte_val;
    }
}

/**
 * napot_install_pte_at_level - Install a PTE at a specified level
 * @ctx:   Page table context
 * @va:    Virtual address (aligned to page size at @level)
 * @pte:   Pre-constructed PTE value
 * @level: Target level (PT_LEVEL_2M=1, PT_LEVEL_1G=2)
 *
 * Directly writes the PTE at the specified level by walking the page
 * table. For level 2 (1GB, root level in Sv39), directly writes to
 * the root page table. For level 1 (2MB), ensures intermediate page
 * tables exist.
 *
 * Used for RSVD-08/09 to install N=1 PTE at superpage levels.
 */
static void napot_install_pte_at_level(pt_context_t *ctx, uintptr_t va,
                                        uintptr_t pte_val, int level) {
    if (!ctx->root_pt) {
        printf("ERROR: napot_install_pte_at_level: root PT is NULL\n");
        return;
    }

    int top_level = ctx->levels - 1;

    if (level == top_level) {
        /* Target is the root level - write directly to root PT */
        uintptr_t vpn = VA_VPN(va, level);
        ctx->root_pt[vpn] = pte_val;
        return;
    }

    if (level >= top_level) {
        printf("ERROR: napot_install_pte_at_level: level %d >= top %d\n",
               level, top_level);
        return;
    }

    /* For levels below root, we need intermediate page tables.
     * Walk from root, creating non-leaf entries as needed. */
    uintptr_t *pt = ctx->root_pt;

    for (int cur_level = top_level; cur_level > level; cur_level--) {
        uintptr_t vpn = VA_VPN(va, cur_level);
        uintptr_t entry = pt[vpn];

        if (entry & PTE_V) {
            if (PTE_IS_LEAF(entry)) {
                /* Existing leaf at higher level - we need to break it
                 * down. Clear it and create a non-leaf pointing to a
                 * new page table page. Use pt_map_page for a nearby
                 * address that won't conflict. */
                pt[vpn] = 0;  /* Clear existing leaf */
                /* Fall through to allocate new intermediate page */
            } else {
                /* Non-leaf: follow to next level */
                pt = (uintptr_t *)PTE_TO_PA(entry);
                continue;
            }
        }

        /* Need to allocate intermediate page tables.
         * Use pt_map_page with a dummy mapping at the target level
         * to force creation of intermediate pages. */
        int ret = pt_map_page(ctx, va, va, PTE_V | PTE_R, level);
        if (ret != 0) {
            printf("ERROR: napot_install_pte_at_level: alloc failed\n");
            return;
        }

        /* Restart walk from root since pt_map_page may have changed things */
        pt = ctx->root_pt;
        cur_level = top_level + 1;  /* Will be decremented to top_level */
        continue;
    }

    /* Install PTE at target level */
    uintptr_t vpn = VA_VPN(va, level);
    pt[vpn] = pte_val;
}

/* ===================================================================
 * Code Mapping Helper
 *
 * Sets up 2MB megapage identity mappings for code execution,
 * skipping the 2MB region containing the test pages.
 * =================================================================== */
static int setup_code_mapping(pt_context_t *ctx) {
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);

    /* Determine which 2MB region contains the test pages */
    uintptr_t test_region = (uintptr_t)&__vm_test_region_start;
    uintptr_t test_region_2m = test_region & ~(PAGE_SIZE_2M - 1);

    /* Map 2MB pages up to (but not including) the test region */
    for (uintptr_t addr = base; addr < test_region_2m; addr += PAGE_SIZE_2M) {
        int ret = pt_map_page(ctx, addr, addr, flags, PT_LEVEL_2M);
        if (ret != 0) return ret;
    }

    /* Map one 2MB page after the test region */
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
 * of the test region (e.g., mode compatibility tests).
 */
static int setup_code_mapping_1g(pt_context_t *ctx) {
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    return pt_setup_identity_mapping(ctx, base, PAGE_SIZE_1G,
                                     flags, PT_LEVEL_1G);
}

/* ===================================================================
 * S-mode Test Functions
 *
 * Execute in S-mode with VM enabled via vm_run_in_smode().
 * Use trap_expect_begin/end to detect faults.
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

/**
 * smode_load_expect_fault - Execute a load, expecting a fault.
 * Returns scause if fault, 0 if no fault.
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
 * smode_store_expect_fault - Execute a store, expecting a fault.
 * Returns scause if fault, 0 if no fault.
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
 * smode_exec_expect_fault - Jump to addr, expecting instruction fault.
 * Returns scause if fault, 0 if no fault.
 */
static uintptr_t smode_exec_expect_fault(uintptr_t addr) {
    trap_expect_begin();
    exec_at(addr);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * Multi-offset Access S-mode Functions
 *
 * Used by tests that need to access multiple offsets within a
 * NAPOT region from S-mode.
 * =================================================================== */

/**
 * smode_access_all_16_pages - Access all 16 x 4 KiB pages in a 64 KiB region
 * @base: Base address of the 64 KiB NAPOT region
 * Returns 0 on success, offset of first failure on error.
 */
static uintptr_t smode_access_all_16_pages(uintptr_t base) {
    for (int i = 0; i < 16; i++) {
        uintptr_t addr = base + (uintptr_t)i * PAGE_SIZE_4K;
        trap_expect_begin();
        volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
        *ptr = MAGIC_WRITE + (uintptr_t)i;
        trap_expect_end();
        if (trap_was_triggered())
            return (uintptr_t)i * PAGE_SIZE_4K + 1;

        trap_expect_begin();
        uintptr_t val = *ptr;
        trap_expect_end();
        if (trap_was_triggered())
            return (uintptr_t)i * PAGE_SIZE_4K + 2;

        if (val != MAGIC_WRITE + (uintptr_t)i)
            return (uintptr_t)i * PAGE_SIZE_4K + 3;
    }
    return 0;
}

/**
 * smode_ppn_subst_verify - Verify PPN substitution across 16 pages
 * @base: Base address of the 64 KiB NAPOT region
 *
 * Writes a unique value to each 4 KiB page, then reads back all
 * values to verify each page maps to a distinct physical location.
 * Returns 0 on success, non-zero on failure.
 */
static uintptr_t smode_ppn_subst_verify(uintptr_t base) {
    /* Phase 1: Write unique values to all 16 pages */
    for (int i = 0; i < 16; i++) {
        uintptr_t addr = base + (uintptr_t)i * PAGE_SIZE_4K;
        trap_expect_begin();
        *(volatile uintptr_t *)addr = 0xA5A5000000000000UL | (uintptr_t)i;
        trap_expect_end();
        if (trap_was_triggered())
            return 0x100 | (uintptr_t)i;
    }

    /* Phase 2: Read back and verify each page has its unique value */
    for (int i = 0; i < 16; i++) {
        uintptr_t addr = base + (uintptr_t)i * PAGE_SIZE_4K;
        trap_expect_begin();
        uintptr_t val = *(volatile uintptr_t *)addr;
        trap_expect_end();
        if (trap_was_triggered())
            return 0x200 | (uintptr_t)i;

        uintptr_t expected = 0xA5A5000000000000UL | (uintptr_t)i;
        if (val != expected)
            return 0x300 | (uintptr_t)i;
    }
    return 0;
}

/**
 * smode_check_perm_consistency - Verify permission consistency across region
 * @base: Base address of the 64 KiB NAPOT region
 *
 * Tests that read succeeds at multiple offsets within the region.
 * Returns 0 on success.
 */
static uintptr_t smode_check_perm_consistency(uintptr_t base) {
    uintptr_t offsets[] = {0x0000, 0x1000, 0x4000, 0x8000, 0xC000, 0xF000};
    int n = sizeof(offsets) / sizeof(offsets[0]);

    for (int i = 0; i < n; i++) {
        uintptr_t addr = base + offsets[i];
        trap_expect_begin();
        volatile uintptr_t val = *(volatile uintptr_t *)addr;
        (void)val;
        trap_expect_end();
        if (trap_was_triggered())
            return offsets[i] + 1;
    }
    return 0;
}

/**
 * smode_store_all_expect_fault - Store to multiple offsets, expect all fault
 * @base: Base address
 * Returns 0 if all stores faulted, non-zero on first non-fault.
 */
static uintptr_t smode_store_all_expect_fault(uintptr_t base) {
    uintptr_t offsets[] = {0x0000, 0x4000, 0x8000, 0xF000};
    int n = sizeof(offsets) / sizeof(offsets[0]);

    for (int i = 0; i < n; i++) {
        uintptr_t addr = base + offsets[i];
        trap_expect_begin();
        *(volatile uintptr_t *)addr = 0xDEADBEEF;
        trap_expect_end();
        if (!trap_was_triggered())
            return offsets[i] + 1;  /* Expected fault but didn't get one */
        if (trap_get_cause() != CAUSE_STORE_PAGE_FAULT)
            return offsets[i] + 2;  /* Wrong fault type */
    }
    return 0;
}

/**
 * smode_read_at_offset - Read at base + offset
 * Returns 0 on success, scause on fault.
 */
static uintptr_t smode_read_at_offset(uintptr_t arg) {
    /* arg encodes both base and offset: base in upper bits, offset in lower 20 bits */
    uintptr_t base = arg & ~0xFFFFFUL;
    uintptr_t offset = arg & 0xFFFFFUL;
    uintptr_t addr = base + offset;

    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)addr;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * SUM bit control helper
 * =================================================================== */

/**
 * smode_read_with_sum - Read with sstatus.SUM=1
 * Returns 0 on success, scause on fault.
 */
static uintptr_t smode_read_with_sum(uintptr_t addr) {
    /* Set SUM bit */
    CSRS(sstatus, MSTATUS_SUM_BIT);

    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)addr;
    (void)val;
    trap_expect_end();

    /* Clear SUM bit */
    CSRC(sstatus, MSTATUS_SUM_BIT);

    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * smode_write_with_sum - Write with sstatus.SUM=1
 * Returns 0 on success, scause on fault.
 */
static uintptr_t smode_write_with_sum(uintptr_t addr) {
    CSRS(sstatus, MSTATUS_SUM_BIT);

    trap_expect_begin();
    *(volatile uintptr_t *)addr = MAGIC_WRITE;
    trap_expect_end();

    CSRC(sstatus, MSTATUS_SUM_BIT);

    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * Exec page helper
 *
 * Fill a page with NOP;RET instructions for exec tests.
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

    /* Ensure stores are visible to instruction fetch.
     * fence: complete all prior stores to memory
     * fence.i: synchronize I-cache with D-cache
     * Without this, real hardware (e.g., FPGA/C908) may fetch stale
     * instructions from I-cache instead of the newly written nop;ret. */
    asm volatile("fence" ::: "memory");
    asm volatile("fence.i" ::: "memory");
}

#endif /* SVNAPOT_TEST_HELPERS_H */
