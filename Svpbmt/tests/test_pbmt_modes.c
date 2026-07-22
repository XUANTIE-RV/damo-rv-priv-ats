/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pbmt_modes.c - Group 9: Multi-mode Compatibility
 *
 * Tests MODE-01 through MODE-03
 *
 * Verifies that PBMT attributes work correctly in Sv39, Sv48,
 * and Sv57 modes. PBMT field (bits 62-61) is defined identically
 * across all three PTE formats.
 *
 * Design: code/stack is identity-mapped with PBMT=PMA so the CPU
 * can fetch instructions normally. A separate 1GB gigapage at a
 * high VA carries the PBMT=NC attribute for data-only load/store.
 * This avoids hanging on hardware that cannot fetch from NC pages.
 */

/* 1GB-aligned VA for the PBMT=NC data gigapage, must not overlap
 * the code identity mapping. Compute the next 1GB region after
 * PLATFORM_MEM_BASE to guarantee no conflict on all platforms. */
#define PBMT_TEST_GIGAPAGE_VA \
    ((PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1)) + PAGE_SIZE_1G)

/* ===================================================================
 * MODE-01: Sv39 + PBMT=NC
 * =================================================================== */
TEST_REGISTER(test_pbmt_sv39_nc);
bool test_pbmt_sv39_nc(void) {
    TEST_BEGIN("MODE-01: Sv39 + PBMT=NC load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* Code region: identity-mapped with PMA (normal cacheable fetch) */
    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("Sv39 code mapping (PMA) setup", ret == 0);

    /* Data region: 1GB gigapage at high VA with PBMT=NC */
    uintptr_t data_pa_base = (uintptr_t)test_data_area
                             & ~(PAGE_SIZE_1G - 1);
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    ret = pt_map_page(&ctx, PBMT_TEST_GIGAPAGE_VA, data_pa_base,
                      pte_flags, PT_LEVEL_1G);
    TEST_ASSERT("Sv39 1GB gigapage NC data mapping", ret == 0);

    uintptr_t data_offset = (uintptr_t)test_data_area - data_pa_base;
    uintptr_t test_va = PBMT_TEST_GIGAPAGE_VA + data_offset;

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("Sv39 + PBMT=NC works", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * MODE-02: Sv48 + PBMT=NC
 * =================================================================== */
TEST_REGISTER(test_pbmt_sv48_nc);
bool test_pbmt_sv48_nc(void) {
    TEST_BEGIN("MODE-02: Sv48 + PBMT=NC load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

    /* Code region: identity-mapped with PMA (normal cacheable fetch) */
    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("Sv48 code mapping (PMA) setup", ret == 0);

    /* Data region: 1GB gigapage at high VA with PBMT=NC */
    uintptr_t data_pa_base = (uintptr_t)test_data_area
                             & ~(PAGE_SIZE_1G - 1);
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    ret = pt_map_page(&ctx, PBMT_TEST_GIGAPAGE_VA, data_pa_base,
                      pte_flags, PT_LEVEL_1G);
    TEST_ASSERT("Sv48 1GB gigapage NC data mapping", ret == 0);

    uintptr_t data_offset = (uintptr_t)test_data_area - data_pa_base;
    uintptr_t test_va = PBMT_TEST_GIGAPAGE_VA + data_offset;

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("Sv48 + PBMT=NC works", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * MODE-03: Sv57 + PBMT=NC
 * =================================================================== */
TEST_REGISTER(test_pbmt_sv57_nc);
bool test_pbmt_sv57_nc(void) {
    TEST_BEGIN("MODE-03: Sv57 + PBMT=NC load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);

    /* Code region: identity-mapped with PMA (normal cacheable fetch) */
    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("Sv57 code mapping (PMA) setup", ret == 0);

    /* Data region: 1GB gigapage at high VA with PBMT=NC */
    uintptr_t data_pa_base = (uintptr_t)test_data_area
                             & ~(PAGE_SIZE_1G - 1);
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    ret = pt_map_page(&ctx, PBMT_TEST_GIGAPAGE_VA, data_pa_base,
                      pte_flags, PT_LEVEL_1G);
    TEST_ASSERT("Sv57 1GB gigapage NC data mapping", ret == 0);

    uintptr_t data_offset = (uintptr_t)test_data_area - data_pa_base;
    uintptr_t test_va = PBMT_TEST_GIGAPAGE_VA + data_offset;

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("Sv57 + PBMT=NC works", result == 0);

    pt_pool_reset();
    TEST_END();
}
