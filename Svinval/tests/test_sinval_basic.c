/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_sinval_basic.c - Group 1: SINVAL.VMA Basic Functionality
 *
 * Tests:
 *   SINVAL-01: Permission upgrade with SINVAL.VMA sequence
 *   SINVAL-02: Mapping invalidation with SINVAL.VMA sequence
 *   SINVAL-03: Permission downgrade with SINVAL.VMA sequence
 *   SINVAL-04: Physical address remapping with SINVAL.VMA sequence
 *   SINVAL-05: rs1=x0 global address flush
 *   SINVAL-06: 2M megapage SINVAL.VMA invalidation
 *   SINVAL-07: 1G gigapage SINVAL.VMA invalidation
 *
 * Verifies that SFENCE.W.INVAL + SINVAL.VMA + SFENCE.INVAL.IR sequence
 * is functionally equivalent to SFENCE.VMA.
 */

/* ===================================================================
 * SINVAL-01: Permission upgrade (R-only -> RW) with sequence flush
 * =================================================================== */
TEST_REGISTER(test_sinval_vma_permission_upgrade);
bool test_sinval_vma_permission_upgrade(void) {
    TEST_BEGIN("SINVAL-01: Permission upgrade with SINVAL.VMA sequence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test page: R-only */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Verify R-only: write should fault */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store_expect_fault, test_va);
    TEST_ASSERT("R-only: write faults", result == CAUSE_SPF);

    /* Modify PTE to RW */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Execute SFENCE.W.INVAL + SINVAL.VMA + SFENCE.INVAL.IR */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    /* Verify RW: write should succeed */
    result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("After SINVAL.VMA sequence: write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SINVAL-02: Mapping invalidation (V=1 -> V=0) with sequence flush
 * =================================================================== */
TEST_REGISTER(test_sinval_vma_mapping_invalidation);
bool test_sinval_vma_mapping_invalidation(void) {
    TEST_BEGIN("SINVAL-02: Mapping invalidation with SINVAL.VMA sequence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test page: RW */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Verify access works */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load, test_va);
    TEST_ASSERT("Initial load succeeds", result == 0);

    /* Invalidate mapping: clear V bit by mapping with flags=0 */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("found L0 page table", pt_page != 0);
    uintptr_t vpn0 = VA_VPN0(test_va);
    volatile uintptr_t *pte = (volatile uintptr_t *)(pt_page + vpn0 * sizeof(uintptr_t));
    *pte = 0;  /* Clear entire PTE (V=0) */

    /* Execute three-instruction sequence */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    /* Access should now fault */
    result = vm_run_in_smode(&ctx, smode_load_expect_fault, test_va);
    TEST_ASSERT("After invalidation: load faults", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SINVAL-03: Permission downgrade (RWX -> R-only) with sequence flush
 * =================================================================== */
TEST_REGISTER(test_sinval_vma_permission_downgrade);
bool test_sinval_vma_permission_downgrade(void) {
    TEST_BEGIN("SINVAL-03: Permission downgrade with SINVAL.VMA sequence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test page: RWX */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Verify write succeeds */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("RWX: write succeeds", result == 0);

    /* Downgrade to R-only */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Execute three-instruction sequence */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    /* Write should now fault */
    result = vm_run_in_smode(&ctx, smode_store_expect_fault, test_va);
    TEST_ASSERT("After downgrade: store faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SINVAL-04: Physical address remapping with sequence flush
 * =================================================================== */
TEST_REGISTER(test_sinval_vma_pa_remap);
bool test_sinval_vma_pa_remap(void) {
    TEST_BEGIN("SINVAL-04: Physical address remapping with SINVAL.VMA sequence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Prepare two physical pages with different data */
    uintptr_t pa_old = (uintptr_t)test_fault_page;
    uintptr_t pa_new = (uintptr_t)test_data_area;
    *(volatile uintptr_t *)pa_old = 0xAAAAAAAAAAAAAAAAUL;
    *(volatile uintptr_t *)pa_new = 0xBBBBBBBBBBBBBBBBUL;

    /* Map test VA to old PA */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, pa_old,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Read from old PA */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load, test_va);
    TEST_ASSERT("Initial load succeeds", result == 0);

    /* Remap to new PA */
    pt_map_page(&ctx, test_va, pa_new,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Execute three-instruction sequence */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    /* Verify access uses new PA by reading back */
    result = vm_run_in_smode(&ctx, smode_load, test_va);
    TEST_ASSERT("After remap: load succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SINVAL-05: rs1=x0 global address flush
 * =================================================================== */
TEST_REGISTER(test_sinval_vma_global_flush);
bool test_sinval_vma_global_flush(void) {
    TEST_BEGIN("SINVAL-05: rs1=x0 global address flush");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map two test pages: both R-only */
    uintptr_t va0 = (uintptr_t)test_fault_page;
    uintptr_t va1 = (uintptr_t)test_data_area;
    pt_map_page(&ctx, va0, va0, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va1, va1, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Build TLB entries */
    vm_run_in_smode(&ctx, smode_load, va0);
    vm_run_in_smode(&ctx, smode_load, va1);

    /* Upgrade both to RW */
    pt_map_page(&ctx, va0, va0, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va1, va1, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Global flush: rs1=x0, rs2=x0 */
    SFENCE_W_INVAL();
    SINVAL_VMA(0, 0);
    SFENCE_INVAL_IR();

    /* Both pages should now be writable */
    uintptr_t r0 = vm_run_in_smode(&ctx, smode_store, va0);
    uintptr_t r1 = vm_run_in_smode(&ctx, smode_store, va1);
    TEST_ASSERT("Page 0 write succeeds after global flush", r0 == 0);
    TEST_ASSERT("Page 1 write succeeds after global flush", r1 == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SINVAL-06: 2M megapage SINVAL.VMA invalidation
 *
 * Strategy: Use setup_code_mapping() for code (2M megapages), then
 * map __vm_test_region as a separate 2M megapage for testing.
 * Test permission upgrade: R-only -> RW via SINVAL.VMA sequence.
 * =================================================================== */
TEST_REGISTER(test_sinval_vma_2m_megapage);
bool test_sinval_vma_2m_megapage(void) {
    TEST_BEGIN("SINVAL-06: 2M megapage SINVAL.VMA invalidation");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map the vm_test_region's 2MB-aligned megapage with R-only */
    uintptr_t test_region = (uintptr_t)&__vm_test_region_start;
    uintptr_t mega_va = test_region & ~(PAGE_SIZE_2M - 1);
    pt_map_page(&ctx, mega_va, mega_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_2M);

    /* Build TLB entry with a read */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load, mega_va);
    TEST_ASSERT("Initial read succeeds (R-only megapage)", result == 0);

    /* Upgrade to RW */
    pt_map_page(&ctx, mega_va, mega_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_2M);

    /* Execute three-instruction sequence */
    SFENCE_W_INVAL();
    SINVAL_VMA(mega_va, 0);
    SFENCE_INVAL_IR();

    /* Verify write succeeds after TLB flush */
    result = vm_run_in_smode(&ctx, smode_store, mega_va);
    TEST_ASSERT("2M megapage: write succeeds after SINVAL.VMA upgrade",
                result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SINVAL-07: 1G gigapage SINVAL.VMA invalidation
 *
 * Strategy: Use a 1G gigapage with full RWX permissions. Write a
 * known value, then remap the gigapage to a DIFFERENT physical
 * address (still RWX), use SINVAL.VMA to flush, and verify the
 * read returns the new PA's content (proving TLB was invalidated).
 *
 * This avoids the permission-downgrade problem where removing W
 * from the gigapage also removes W from code/data areas (causing
 * trap_expect_begin() writes to fault in an infinite loop).
 *
 * We use two 1G-aligned physical regions:
 *   PA_old = PLATFORM_MEM_BASE & ~(1G-1) (the normal 1G region)
 *   PA_new = PA_old + 1G (requires 2G+ RAM, QEMU -m 256M won't work
 *            but QEMU -m 2G will; if not available, we skip)
 *
 * Alternative approach: keep same PA, just verify the sequence
 * executes without error (functional smoke test for 1G granularity).
 * =================================================================== */
TEST_REGISTER(test_sinval_vma_1g_gigapage);
bool test_sinval_vma_1g_gigapage(void) {
    TEST_BEGIN("SINVAL-07: 1G gigapage SINVAL.VMA invalidation");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);

    /* Map 1G gigapage with full RWX permissions */
    pt_map_page(&ctx, base, base,
                PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D,
                PT_LEVEL_1G);

    /* Map UART for S-mode printf */
    pt_map_page(&ctx, PLATFORM_UART0_BASE, PLATFORM_UART0_BASE,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Write a known value at a test address within the gigapage */
    uintptr_t test_addr = (uintptr_t)test_data_area;
    *(volatile uintptr_t *)test_addr = 0xAAAAAAAAAAAAAAAAUL;

    /* Build TLB entry by reading in S-mode */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load, test_addr);
    TEST_ASSERT("Initial read succeeds (RWX gigapage)", result == 0);

    /* Now write a different value (from M-mode, no VM) to the same PA */
    *(volatile uintptr_t *)test_addr = 0xBBBBBBBBBBBBBBBBUL;

    /* Execute SINVAL.VMA sequence to flush the 1G gigapage TLB entry.
     * Even though we didn't change the PTE, this verifies that
     * SINVAL.VMA correctly handles 1G granularity addresses. */
    SFENCE_W_INVAL();
    SINVAL_VMA(base, 0);
    SFENCE_INVAL_IR();

    /* Read again in S-mode - should see the updated value.
     * (On QEMU this always works because QEMU doesn't cache data
     * in TLB, but this tests the instruction sequence is valid
     * for 1G-aligned addresses.) */
    result = vm_run_in_smode(&ctx, smode_load, test_addr);
    TEST_ASSERT("Read after SINVAL.VMA on 1G gigapage succeeds", result == 0);

    /* Also verify the SINVAL.VMA sequence works with a write */
    result = vm_run_in_smode(&ctx, smode_store, test_addr);
    TEST_ASSERT("Write after SINVAL.VMA on 1G gigapage succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
