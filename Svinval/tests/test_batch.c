/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_batch.c - Group 3: Batch Invalidation Operations
 *
 * Tests:
 *   BATCH-01: Batch invalidation of 3 pages
 *   BATCH-02: Batch invalidation with different permission changes
 *   BATCH-03: Batch invalidation of 16 consecutive pages
 *   BATCH-04: Batch invalidation with mixed rs1 parameters
 *
 * Verifies that multiple SINVAL.VMA instructions can be batched
 * between SFENCE.W.INVAL and SFENCE.INVAL.IR.
 *
 * All tests use setup_code_mapping() (2M megapages) for code, and
 * map test pages in the __vm_test_region area (which is excluded
 * from setup_code_mapping) using 4K pages.
 */

/* ===================================================================
 * BATCH-01: Batch invalidation of 3 pages
 * =================================================================== */
TEST_REGISTER(test_sinval_batch_3pages);
bool test_sinval_batch_3pages(void) {
    TEST_BEGIN("BATCH-01: Batch invalidation of 3 pages");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Use the vm_test_region for 3 test pages */
    uintptr_t va0 = (uintptr_t)test_data_area;
    uintptr_t va1 = (uintptr_t)test_fault_page;
    uintptr_t va2 = (uintptr_t)test_extra_page;

    /* Map 3 test pages: R-only */
    pt_map_page(&ctx, va0, va0, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va1, va1, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va2, va2, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Build TLB entries */
    vm_run_in_smode(&ctx, smode_load, va0);
    vm_run_in_smode(&ctx, smode_load, va1);
    vm_run_in_smode(&ctx, smode_load, va2);

    /* Upgrade all 3 pages to RW */
    pt_map_page(&ctx, va0, va0, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va1, va1, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va2, va2, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Batch invalidation: W.INVAL + 3x SINVAL.VMA + INVAL.IR */
    SFENCE_W_INVAL();
    SINVAL_VMA(va0, 0);
    SINVAL_VMA(va1, 0);
    SINVAL_VMA(va2, 0);
    SFENCE_INVAL_IR();

    /* Verify all 3 pages are writable */
    uintptr_t r0 = vm_run_in_smode(&ctx, smode_store, va0);
    uintptr_t r1 = vm_run_in_smode(&ctx, smode_store, va1);
    uintptr_t r2 = vm_run_in_smode(&ctx, smode_store, va2);
    TEST_ASSERT("Page 0 write succeeds", r0 == 0);
    TEST_ASSERT("Page 1 write succeeds", r1 == 0);
    TEST_ASSERT("Page 2 write succeeds", r2 == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * BATCH-02: Batch invalidation with different permission changes
 *
 * Page A: upgrade R -> RW
 * Page B: downgrade RW -> R
 * Page C: invalidate V=1 -> V=0
 * =================================================================== */
TEST_REGISTER(test_sinval_batch_mixed_perms);
bool test_sinval_batch_mixed_perms(void) {
    TEST_BEGIN("BATCH-02: Batch invalidation with different permission changes");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Use the vm_test_region for 3 test pages */
    uintptr_t va_a = (uintptr_t)test_data_area;
    uintptr_t va_b = (uintptr_t)test_fault_page;
    uintptr_t va_c = (uintptr_t)test_extra_page;

    /* Page A: R-only, Page B: RW, Page C: RW */
    pt_map_page(&ctx, va_a, va_a, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va_b, va_b, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va_c, va_c, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Build TLB entries */
    vm_run_in_smode(&ctx, smode_load, va_a);
    vm_run_in_smode(&ctx, smode_store, va_b);
    vm_run_in_smode(&ctx, smode_load, va_c);

    /* Page A: upgrade R -> RW */
    pt_map_page(&ctx, va_a, va_a, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);
    /* Page B: downgrade RW -> R */
    pt_map_page(&ctx, va_b, va_b, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);
    /* Page C: invalidate (V=0) - directly modify PTE */
    uintptr_t pt_page_c = get_pt_page_addr(&ctx, va_c, 0);
    if (pt_page_c) {
        uintptr_t vpn0_c = VA_VPN0(va_c);
        volatile uintptr_t *pte_c = (volatile uintptr_t *)(pt_page_c + vpn0_c * sizeof(uintptr_t));
        *pte_c = 0;
    }

    /* Batch invalidation */
    SFENCE_W_INVAL();
    SINVAL_VMA(va_a, 0);
    SINVAL_VMA(va_b, 0);
    SINVAL_VMA(va_c, 0);
    SFENCE_INVAL_IR();

    /* Verify: A writable, B write faults, C access faults */
    uintptr_t ra = vm_run_in_smode(&ctx, smode_store, va_a);
    uintptr_t rb = vm_run_in_smode(&ctx, smode_store_expect_fault, va_b);
    uintptr_t rc = vm_run_in_smode(&ctx, smode_load_expect_fault, va_c);
    TEST_ASSERT("Page A: write succeeds (R->RW)", ra == 0);
    TEST_ASSERT("Page B: write faults (RW->R)", rb == CAUSE_SPF);
    TEST_ASSERT("Page C: load faults (V=0)", rc == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * BATCH-03: Batch invalidation of 16 consecutive pages
 *
 * Note: The vm_test_region only has 3 pages. For 16 pages, we
 * allocate them starting from __vm_test_region_start. The linker
 * script reserves enough space (we need 16 * 4K = 64K).
 * Since the region is 2MB-aligned and excluded from code mapping,
 * we can safely use it.
 * =================================================================== */
TEST_REGISTER(test_sinval_batch_16pages);
bool test_sinval_batch_16pages(void) {
    TEST_BEGIN("BATCH-03: Batch invalidation of 16 consecutive pages");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    #define BATCH_COUNT 16
    uintptr_t test_base = (uintptr_t)&__vm_test_region_start;
    uintptr_t vas[BATCH_COUNT];

    /* Map 16 pages: R-only */
    for (int i = 0; i < BATCH_COUNT; i++) {
        vas[i] = test_base + i * PAGE_SIZE_4K;
        pt_map_page(&ctx, vas[i], vas[i],
                    PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);
    }

    /* Build TLB entries */
    for (int i = 0; i < BATCH_COUNT; i++) {
        vm_run_in_smode(&ctx, smode_load, vas[i]);
    }

    /* Upgrade all to RW */
    for (int i = 0; i < BATCH_COUNT; i++) {
        pt_map_page(&ctx, vas[i], vas[i],
                    PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);
    }

    /* Batch invalidation */
    SFENCE_W_INVAL();
    for (int i = 0; i < BATCH_COUNT; i++) {
        SINVAL_VMA(vas[i], 0);
    }
    SFENCE_INVAL_IR();

    /* Verify all 16 pages are writable */
    bool all_pass = true;
    for (int i = 0; i < BATCH_COUNT; i++) {
        uintptr_t r = vm_run_in_smode(&ctx, smode_store, vas[i]);
        if (r != 0) {
            printf("  Page %d write failed (result=0x%lx)\n", i, (unsigned long)r);
            all_pass = false;
        }
    }
    TEST_ASSERT("All 16 pages writable after batch invalidation", all_pass);

    #undef BATCH_COUNT
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * BATCH-04: Batch invalidation with mixed rs1 parameters
 *
 * Some SINVAL.VMA specify concrete addresses, one uses rs1=x0.
 * =================================================================== */
TEST_REGISTER(test_sinval_batch_mixed_rs1);
bool test_sinval_batch_mixed_rs1(void) {
    TEST_BEGIN("BATCH-04: Batch invalidation with mixed rs1 parameters");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Use the vm_test_region for 3 test pages */
    uintptr_t va0 = (uintptr_t)test_data_area;
    uintptr_t va1 = (uintptr_t)test_fault_page;
    uintptr_t va2 = (uintptr_t)test_extra_page;

    /* Map 3 pages: R-only */
    pt_map_page(&ctx, va0, va0, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va1, va1, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va2, va2, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Build TLB entries */
    vm_run_in_smode(&ctx, smode_load, va0);
    vm_run_in_smode(&ctx, smode_load, va1);
    vm_run_in_smode(&ctx, smode_load, va2);

    /* Upgrade all to RW */
    pt_map_page(&ctx, va0, va0, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va1, va1, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va2, va2, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Mixed rs1: specific addresses + global flush */
    SFENCE_W_INVAL();
    SINVAL_VMA(va0, 0);          /* specific address */
    SINVAL_VMA(0, 0);            /* rs1=x0: global flush */
    SFENCE_INVAL_IR();

    /* All pages should be flushed (global flush covers everything) */
    uintptr_t r0 = vm_run_in_smode(&ctx, smode_store, va0);
    uintptr_t r1 = vm_run_in_smode(&ctx, smode_store, va1);
    uintptr_t r2 = vm_run_in_smode(&ctx, smode_store, va2);
    TEST_ASSERT("Page 0 write succeeds", r0 == 0);
    TEST_ASSERT("Page 1 write succeeds (global flush)", r1 == 0);
    TEST_ASSERT("Page 2 write succeeds (global flush)", r2 == 0);

    pt_pool_reset();
    TEST_END();
}
