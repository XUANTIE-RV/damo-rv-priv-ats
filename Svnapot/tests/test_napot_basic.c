/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_napot_basic.c - Group 1: 64 KiB NAPOT Page Basic Mapping
 *
 * Tests NAPOT64-01 through NAPOT64-06
 *
 * Verifies basic 64 KiB NAPOT page address translation:
 *   - Read/write access to NAPOT region
 *   - End-of-region access
 *   - Multi-offset access within region
 *   - Out-of-region access triggers page-fault
 *   - Multiple NAPOT regions coexist
 */

/* ===================================================================
 * NAPOT64-01: 64 KiB NAPOT page read access
 * =================================================================== */
TEST_REGISTER(test_napot64_read);
bool test_napot64_read(void) {
    TEST_BEGIN("NAPOT64-01: 64 KiB NAPOT page read access");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map 64 KiB NAPOT region: identity mapping with R permission */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_load, test_va);
    TEST_ASSERT("64 KiB NAPOT read succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NAPOT64-02: 64 KiB NAPOT page write access
 * =================================================================== */
TEST_REGISTER(test_napot64_write);
bool test_napot64_write(void) {
    TEST_BEGIN("NAPOT64-02: 64 KiB NAPOT page write access");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("64 KiB NAPOT write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NAPOT64-03: 64 KiB NAPOT page end address access
 * =================================================================== */
TEST_REGISTER(test_napot64_end_addr);
bool test_napot64_end_addr(void) {
    TEST_BEGIN("NAPOT64-03: 64 KiB NAPOT page end address access");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Read at the last valid address within the 64 KiB region
     * (last 8-byte aligned address = base + 0xFFF8) */
    uintptr_t end_addr = test_va + NAPOT_64K_SIZE - sizeof(uintptr_t);
    uintptr_t result = vm_run_in_smode(&ctx, smode_load, end_addr);
    TEST_ASSERT("64 KiB NAPOT end address read succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NAPOT64-04: 64 KiB NAPOT region multi-offset access
 * =================================================================== */
TEST_REGISTER(test_napot64_multi_offset);
bool test_napot64_multi_offset(void) {
    TEST_BEGIN("NAPOT64-04: 64 KiB NAPOT region multi-offset access");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Access all 16 x 4 KiB pages within the 64 KiB region */
    uintptr_t result = vm_run_in_smode(&ctx, smode_access_all_16_pages,
                                        test_va);
    TEST_ASSERT("all 16 pages accessible", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NAPOT64-05: 64 KiB NAPOT out-of-region access
 * =================================================================== */
TEST_REGISTER(test_napot64_out_of_region);
bool test_napot64_out_of_region(void) {
    TEST_BEGIN("NAPOT64-05: 64 KiB NAPOT out-of-region access");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Access first byte past the 64 KiB region - should fault */
    uintptr_t out_addr = test_va + NAPOT_64K_SIZE;
    uintptr_t result = vm_run_in_smode(&ctx, smode_load_expect_fault,
                                        out_addr);
    TEST_ASSERT("out-of-region triggers page fault",
                result == CAUSE_LOAD_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NAPOT64-06: Multiple NAPOT 64 KiB regions
 * =================================================================== */
TEST_REGISTER(test_napot64_multiple_regions);
bool test_napot64_multiple_regions(void) {
    TEST_BEGIN("NAPOT64-06: Multiple NAPOT 64 KiB regions");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map two adjacent 64 KiB NAPOT regions */
    uintptr_t region0 = NAPOT_TEST_REGION_0;
    uintptr_t region1 = NAPOT_TEST_REGION_1;

    uintptr_t pte0 = napot_make_pte(region0,
                                     PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    uintptr_t pte1 = napot_make_pte(region1,
                                     PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, region0, pte0);
    napot_install_pte(&ctx, region1, pte1);

    /* Access both regions */
    uintptr_t r0 = vm_run_in_smode(&ctx, smode_read_write, region0);
    TEST_ASSERT("region 0 read/write succeeds", r0 == 0);

    uintptr_t r1 = vm_run_in_smode(&ctx, smode_read_write, region1);
    TEST_ASSERT("region 1 read/write succeeds", r1 == 0);

    pt_pool_reset();
    TEST_END();
}
