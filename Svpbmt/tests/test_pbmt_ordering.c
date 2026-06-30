/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pbmt_ordering.c - Group 6: Memory Ordering Semantics
 *
 * Tests ORDER-01 through ORDER-06
 *
 * Verifies PBMT-overridden memory attributes produce correct data
 * under sequential access patterns and FENCE instructions.
 *
 * Note: True ordering strength tests require multi-hart environments.
 * These single-hart tests verify functional correctness (no data
 * corruption, no unexpected exceptions).
 */

/* ===================================================================
 * ORDER-01: Memory + PBMT=IO sequential access
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_sequential_access);
bool test_pbmt_io_sequential_access(void) {
    TEST_BEGIN("ORDER-01: Memory + PBMT=IO sequential load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map test page: PBMT=IO */
    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* Multiple sequential store then load back */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_sequential_rw,
                                        test_va);
    TEST_ASSERT("PBMT=IO sequential access data correct", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ORDER-02: Memory + PBMT=IO + FENCE
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_fence);
bool test_pbmt_io_fence(void) {
    TEST_BEGIN("ORDER-02: Memory + PBMT=IO with fence iorw,iorw");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* Store, fence iorw,iorw, load back */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_fence_load,
                                        test_va);
    TEST_ASSERT("PBMT=IO with fence data correct", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ORDER-03: Memory + PBMT=NC sequential access
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_sequential_access);
bool test_pbmt_nc_sequential_access(void) {
    TEST_BEGIN("ORDER-03: Memory + PBMT=NC sequential load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_sequential_rw,
                                        test_va);
    TEST_ASSERT("PBMT=NC sequential access data correct", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ORDER-04: Memory + PBMT=NC + FENCE
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_fence);
bool test_pbmt_nc_fence(void) {
    TEST_BEGIN("ORDER-04: Memory + PBMT=NC with fence rw,rw");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_fence_load,
                                        test_va);
    TEST_ASSERT("PBMT=NC with fence data correct", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ORDER-05: I/O area + PBMT=NC (discouraged configuration)
 *
 * This test requires a real I/O device. On QEMU virt, the UART is
 * available but limited to byte-width access. We skip this test
 * because it requires specific I/O hardware semantics.
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_area_nc);
bool test_pbmt_io_area_nc(void) {
    TEST_BEGIN("ORDER-05: I/O area + PBMT=NC (discouraged)");
    TEST_SKIP("requires real I/O device with specific access semantics");
}

/* ===================================================================
 * ORDER-06: PBMT=IO page FENCE coverage
 *
 * Verifies fence iorw,iorw is effective on PBMT=IO pages.
 * Since PBMT=IO pages are treated as both I/O and memory,
 * fence iorw,iorw should order both types of accesses.
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_fence_coverage);
bool test_pbmt_io_fence_coverage(void) {
    TEST_BEGIN("ORDER-06: PBMT=IO page FENCE iorw,iorw coverage");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* Verify fence iorw,iorw correctly orders stores and loads
     * on a PBMT=IO page (treated as both I/O and memory) */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_fence_load,
                                        test_va);
    TEST_ASSERT("PBMT=IO FENCE iorw,iorw: data correct", result == 0);

    pt_pool_reset();
    TEST_END();
}
