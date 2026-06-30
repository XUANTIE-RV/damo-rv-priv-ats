/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pte_valid.c - Group 3: PTE Validity Check (VALID-01 ~ VALID-05)
 *
 * Tests:
 *   VALID-01: V=0 PTE triggers load page fault
 *   VALID-02: V=0 PTE triggers store page fault
 *   VALID-03: V=0 PTE triggers instruction page fault
 *   VALID-04: R=0,W=1 reserved encoding triggers load page fault
 *   VALID-05: R=0,W=1 reserved encoding triggers store page fault
 */

TEST_REGISTER(test_sv48_valid01);
bool test_sv48_valid01(void) {
    TEST_BEGIN("VALID-01: V=0 PTE triggers load page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_R | PTE_W | PTE_A | PTE_D,  /* no PTE_V */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("V=0 triggers load page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_valid02);
bool test_sv48_valid02(void) {
    TEST_BEGIN("VALID-02: V=0 PTE triggers store page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("V=0 triggers store page fault", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_valid03);
bool test_sv48_valid03(void) {
    TEST_BEGIN("VALID-03: V=0 PTE triggers instruction page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    init_exec_page();
    pt_map_page(&ctx, test_va, test_va,
                PTE_R | PTE_X | PTE_A | PTE_D,  /* no PTE_V */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT("V=0 triggers instruction page fault", result == CAUSE_IPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_valid04);
bool test_sv48_valid04(void) {
    TEST_BEGIN("VALID-04: R=0,W=1 reserved encoding triggers load page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_W | PTE_A | PTE_D,  /* R=0, W=1 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("R=0,W=1 triggers load page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_valid05);
bool test_sv48_valid05(void) {
    TEST_BEGIN("VALID-05: R=0,W=1 reserved encoding triggers store page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_W | PTE_A | PTE_D,  /* R=0, W=1 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("R=0,W=1 triggers store page fault", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}
