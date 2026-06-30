/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_tlb.c - Group 6: TLB flush + repeated page walk verification
 *
 * Verifies that page walks succeed consistently after TLB flushes,
 * ruling out TLB caching masking page walk failures.
 *
 * Tests: SSCCPTR-TLB-01 ~ SSCCPTR-TLB-04
 */

#define TLB_REPEAT_COUNT 10

/* --- SSCCPTR-TLB-01: Repeated TLB flush + load --- */
TEST_REGISTER(test_ssccptr_tlb_repeated_load);
bool test_ssccptr_tlb_repeated_load(void) {
    TEST_BEGIN("SSCCPTR-TLB-01: Repeated page walk after TLB flush (load)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    for (int i = 0; i < TLB_REPEAT_COUNT; i++) {
        uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
        TEST_ASSERT_EQ("repeated load page walk succeeds", result, 0);
    }

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-TLB-02: Repeated TLB flush + store --- */
TEST_REGISTER(test_ssccptr_tlb_repeated_store);
bool test_ssccptr_tlb_repeated_store(void) {
    TEST_BEGIN("SSCCPTR-TLB-02: Repeated page walk after TLB flush (store)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);

    for (int i = 0; i < TLB_REPEAT_COUNT; i++) {
        uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
        TEST_ASSERT_EQ("repeated store page walk succeeds", result, 0);
    }

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-TLB-03: Repeated TLB flush + fetch --- */
TEST_REGISTER(test_ssccptr_tlb_repeated_fetch);
bool test_ssccptr_tlb_repeated_fetch(void) {
    TEST_BEGIN("SSCCPTR-TLB-03: Repeated page walk after TLB flush (fetch)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);
    init_exec_page();

    uintptr_t test_va = (uintptr_t)test_exec_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_X | PTE_A | PTE_D, PT_LEVEL_4K);

    for (int i = 0; i < TLB_REPEAT_COUNT; i++) {
        uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
        TEST_ASSERT_EQ("repeated fetch page walk succeeds", result, 0);
    }

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-TLB-04: Alternating page walk on two different VAs --- */
TEST_REGISTER(test_ssccptr_tlb_alternating);
bool test_ssccptr_tlb_alternating(void) {
    TEST_BEGIN("SSCCPTR-TLB-04: Alternating page walk on two VAs");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t va1 = (uintptr_t)test_data_area;
    uintptr_t va2 = (uintptr_t)test_fault_page;

    pt_map_page(&ctx, va1, va1,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va2, va2,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    for (int i = 0; i < TLB_REPEAT_COUNT; i++) {
        uintptr_t target = (i % 2 == 0) ? va1 : va2;
        uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, target);
        TEST_ASSERT_EQ("alternating page walk succeeds", result, 0);
    }

    pt_pool_reset();
    TEST_END();
}
