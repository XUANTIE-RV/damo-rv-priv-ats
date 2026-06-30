/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pte_perms.c - Group 4: PTE Permission Bits RWX (RWX-01 ~ RWX-05)
 *
 * Tests:
 *   RWX-01: R-only page: read succeeds, write faults
 *   RWX-02: RW page: read/write succeed, exec faults
 *   RWX-03: X-only page: exec succeeds, read/write fault
 *   RWX-04: RX page: read/exec succeed, write faults
 *   RWX-05: RWX page: all access succeeds
 */

TEST_REGISTER(test_sv48_rwx01);
bool test_sv48_rwx01(void) {
    TEST_BEGIN("RWX-01: R-only page: read succeeds, write faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("R-only: read succeeds", result == 0);

    result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("R-only: write faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_rwx02);
bool test_sv48_rwx02(void) {
    TEST_BEGIN("RWX-02: RW page: read/write succeed, exec faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    init_exec_page();
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("RW: read succeeds", result == 0);

    result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("RW: write succeeds", result == 0);

    result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT("RW: exec faults", result == CAUSE_IPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_rwx03);
bool test_sv48_rwx03(void) {
    TEST_BEGIN("RWX-03: X-only page: exec succeeds, read/write fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    init_exec_page();
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_X | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT("X-only: exec succeeds", result == 0);

    result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("X-only: read faults", result == CAUSE_LPF);

    result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("X-only: write faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_rwx04);
bool test_sv48_rwx04(void) {
    TEST_BEGIN("RWX-04: RX page: read/exec succeed, write faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    init_exec_page();
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_X | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("RX: read succeeds", result == 0);

    result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT("RX: exec succeeds", result == 0);

    result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("RX: write faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_rwx05);
bool test_sv48_rwx05(void) {
    TEST_BEGIN("RWX-05: RWX page: all access succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    init_exec_page();
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("RWX: read succeeds", result == 0);

    result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("RWX: write succeeds", result == 0);

    /* Re-initialize exec page: the store above overwrote the nop;ret */
    init_exec_page();
    result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT("RWX: exec succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
