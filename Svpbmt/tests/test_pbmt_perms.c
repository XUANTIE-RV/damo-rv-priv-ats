/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pbmt_perms.c - Group 4: PBMT and Page Permission Interaction
 *
 * Tests PERM-01 through PERM-06, ALIGN-01 through ALIGN-03
 *
 * Verifies that PBMT settings do not affect RWX permission checks,
 * and tests PBMT interaction with U bit and non-aligned access.
 */

/* ===================================================================
 * PERM-01: PBMT=NC + R-only, store triggers page-fault
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_readonly_store_fault);
bool test_pbmt_nc_readonly_store_fault(void) {
    TEST_BEGIN("PERM-01: PBMT=NC + R-only, store triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map test page: PBMT=NC, R-only (no W permission) */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=NC + R-only: store faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-02: PBMT=IO + R-only, store triggers page-fault
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_readonly_store_fault);
bool test_pbmt_io_readonly_store_fault(void) {
    TEST_BEGIN("PERM-02: PBMT=IO + R-only, store triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=IO + R-only: store faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-03: PBMT=NC + no-exec, exec triggers page-fault
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_noexec_fault);
bool test_pbmt_nc_noexec_fault(void) {
    TEST_BEGIN("PERM-03: PBMT=NC + no-exec, exec triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    fill_exec_page(test_va);

    /* PBMT=NC, RW but no X */
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=NC + no-exec: exec faults", result == CAUSE_IPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-04: PBMT=IO + no-exec, exec triggers page-fault
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_noexec_fault);
bool test_pbmt_io_noexec_fault(void) {
    TEST_BEGIN("PERM-04: PBMT=IO + no-exec, exec triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    fill_exec_page(test_va);

    /* PBMT=IO, RW but no X */
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=IO + no-exec: exec faults", result == CAUSE_IPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-05: PBMT=NC + U-bit, S-mode (SUM=0) access faults
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_ubit_smode_fault);
bool test_pbmt_nc_ubit_smode_fault(void) {
    TEST_BEGIN("PERM-05: PBMT=NC + U=1, S-mode (SUM=0) access faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map test page: PBMT=NC, U=1 */
    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* S-mode with SUM=0 accessing U-page should fault */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=NC + U=1 S-mode (SUM=0): load faults",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-06: PBMT=NC + U-bit, S-mode (SUM=1) access succeeds
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_ubit_sum_ok);
bool test_pbmt_nc_ubit_sum_ok(void) {
    TEST_BEGIN("PERM-06: PBMT=NC + U=1, S-mode (SUM=1) access succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map test page: PBMT=NC, U=1 */
    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* S-mode with SUM=1 accessing U-page should succeed */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_with_sum,
                                        test_va);
    TEST_ASSERT("PBMT=NC + U=1 S-mode (SUM=1): load succeeds",
                result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ALIGN-01: PBMT=IO non-aligned load (implementation-defined)
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_misaligned_load);
bool test_pbmt_io_misaligned_load(void) {
    TEST_BEGIN("ALIGN-01: PBMT=IO non-aligned load (impl-defined)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* Misaligned load on PBMT=IO page: may succeed or fault */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_misaligned,
                                        test_va);
    TEST_ASSERT("PBMT=IO misaligned load: succeeds or access fault",
                result == 0 || result == CAUSE_LAF ||
                result == CAUSE_LOAD_ADDR_MISALIGN);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ALIGN-02: PBMT=IO non-aligned store (implementation-defined)
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_misaligned_store);
bool test_pbmt_io_misaligned_store(void) {
    TEST_BEGIN("ALIGN-02: PBMT=IO non-aligned store (impl-defined)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_misaligned,
                                        test_va);
    TEST_ASSERT("PBMT=IO misaligned store: succeeds or access fault",
                result == 0 || result == CAUSE_SAF ||
                result == CAUSE_STORE_ADDR_MISALIGN);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ALIGN-03: PBMT=PMA non-aligned load (baseline)
 * =================================================================== */
TEST_REGISTER(test_pbmt_pma_misaligned_load);
bool test_pbmt_pma_misaligned_load(void) {
    TEST_BEGIN("ALIGN-03: PBMT=PMA non-aligned load (baseline)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_PMA;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* Misaligned load on PBMT=PMA page: should succeed */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_misaligned,
                                        test_va);
    TEST_ASSERT("PBMT=PMA misaligned load succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
