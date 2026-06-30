/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_permissions.c - Group 4: NAPOT PTE Permission Control
 *
 * Tests PERM-01 through PERM-07
 *
 * Verifies R, W, X, U permission bits on NAPOT pages.
 */

/* ===================================================================
 * PERM-01: NAPOT read-only page (R only), write faults
 * =================================================================== */
TEST_REGISTER(test_napot_perm_read_only);
bool test_napot_perm_read_only(void) {
    TEST_BEGIN("PERM-01: NAPOT 64 KiB page R-only, write faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Read should succeed */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load, test_va);
    TEST_ASSERT("NAPOT R-only: read succeeds", result == 0);

    /* Write should trigger store page-fault */
    result = vm_run_in_smode(&ctx, smode_store_expect_fault, test_va);
    TEST_ASSERT("NAPOT R-only: write faults",
                result == CAUSE_STORE_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-02: NAPOT read-write page (RW)
 * =================================================================== */
TEST_REGISTER(test_napot_perm_rw);
bool test_napot_perm_rw(void) {
    TEST_BEGIN("PERM-02: NAPOT 64 KiB page RW, read/write succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("NAPOT RW: read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-03: NAPOT executable page (RX), execute succeeds
 * =================================================================== */
TEST_REGISTER(test_napot_perm_rx);
bool test_napot_perm_rx(void) {
    TEST_BEGIN("PERM-03: NAPOT 64 KiB page RX, execute succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;

    /* Fill test region with nop;ret for execution test */
    fill_exec_page(test_va);

    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_X | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Execute at the test VA - should succeed (nop;ret) */
    uintptr_t result = vm_run_in_smode(&ctx, smode_exec_expect_fault,
                                        test_va);
    TEST_ASSERT("NAPOT RX: execute succeeds (no fault)", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-04: NAPOT non-executable page (RW, no X), execute faults
 * =================================================================== */
TEST_REGISTER(test_napot_perm_no_exec);
bool test_napot_perm_no_exec(void) {
    TEST_BEGIN("PERM-04: NAPOT 64 KiB page RW (no X), execute faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;

    fill_exec_page(test_va);

    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_exec_expect_fault,
                                        test_va);
    TEST_ASSERT("NAPOT RW: execute faults",
                result == CAUSE_INST_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-05: NAPOT U=1, S-mode (SUM=0) access faults
 * =================================================================== */
TEST_REGISTER(test_napot_perm_u_bit);
bool test_napot_perm_u_bit(void) {
    TEST_BEGIN("PERM-05: NAPOT U=1, S-mode (SUM=0) access faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_U |
                                          PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* S-mode with SUM=0 accessing U-page should fault */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("NAPOT U=1 S-mode (SUM=0): load faults",
                result == CAUSE_LOAD_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-06: NAPOT U=1, S-mode (SUM=1) read/write succeeds
 * =================================================================== */
TEST_REGISTER(test_napot_perm_u_sum);
bool test_napot_perm_u_sum(void) {
    TEST_BEGIN("PERM-06: NAPOT U=1, S-mode (SUM=1) read succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_U |
                                          PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* S-mode with SUM=1 accessing U-page should succeed */
    uintptr_t result = vm_run_in_smode(&ctx, smode_read_with_sum,
                                        test_va);
    TEST_ASSERT("NAPOT U=1 S-mode (SUM=1): read succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PERM-07: NAPOT permission consistency across region
 * =================================================================== */
TEST_REGISTER(test_napot_perm_consistency);
bool test_napot_perm_consistency(void) {
    TEST_BEGIN("PERM-07: NAPOT permission consistency across 64 KiB");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* R-only NAPOT mapping */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* All offsets should be readable */
    uintptr_t result = vm_run_in_smode(&ctx, smode_check_perm_consistency,
                                        test_va);
    TEST_ASSERT("read permission consistent across region", result == 0);

    /* All offsets should fault on write */
    result = vm_run_in_smode(&ctx, smode_store_all_expect_fault, test_va);
    TEST_ASSERT("write fault consistent across region", result == 0);

    pt_pool_reset();
    TEST_END();
}
