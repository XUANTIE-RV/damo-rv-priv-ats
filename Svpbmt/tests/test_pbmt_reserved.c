/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pbmt_reserved.c - Group 2: PBMT Reserved Value Exceptions
 *
 * Tests RSVD-01 through RSVD-04
 *
 * Verifies that leaf PTE with PBMT=3 (reserved, bits 62-61 = 11)
 * triggers page-fault on any access type.
 */

/* ===================================================================
 * RSVD-01: PBMT=3 load fault
 * =================================================================== */
TEST_REGISTER(test_pbmt_reserved_load_fault);
bool test_pbmt_reserved_load_fault(void) {
    TEST_BEGIN("RSVD-01: PBMT=3 (reserved) load triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map test page: PBMT=3 (reserved, bits 62-61 = 11) */
    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_RSVD;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=3 triggers load page-fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * RSVD-02: PBMT=3 store fault
 * =================================================================== */
TEST_REGISTER(test_pbmt_reserved_store_fault);
bool test_pbmt_reserved_store_fault(void) {
    TEST_BEGIN("RSVD-02: PBMT=3 (reserved) store triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_RSVD;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=3 triggers store page-fault", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * RSVD-03: PBMT=3 exec fault
 * =================================================================== */
TEST_REGISTER(test_pbmt_reserved_exec_fault);
bool test_pbmt_reserved_exec_fault(void) {
    TEST_BEGIN("RSVD-03: PBMT=3 (reserved) exec triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    fill_exec_page(test_va);

    uintptr_t pte_flags = PTE_V | PTE_R | PTE_X | PTE_A | PTE_D
                          | PBMT_RSVD;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=3 triggers instruction page-fault",
                result == CAUSE_IPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * RSVD-04: PBMT=3 superpage fault
 * =================================================================== */
TEST_REGISTER(test_pbmt_reserved_superpage_fault);
bool test_pbmt_reserved_superpage_fault(void) {
    TEST_BEGIN("RSVD-04: PBMT=3 on 2MB superpage triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* Use 1GB identity mapping for code */
    int ret = setup_code_mapping_1g(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map a separate 2MB superpage with PBMT=3 (reserved).
     * Use the test region base aligned to 2MB. */
    uintptr_t test_va = (uintptr_t)test_data_area & ~(PAGE_SIZE_2M - 1);

    /* We need to avoid conflict with the 1GB mapping.
     * Since 1GB mapping covers the entire region, we need to use
     * a 2MB mapping approach instead. Reset and use 2MB code mapping. */
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup (2MB)", ret == 0);

    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_RSVD;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=3 superpage triggers load page-fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
