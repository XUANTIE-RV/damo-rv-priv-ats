/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_upriv.c - Group 5: U-bit and Privilege Access Control (UPRIV-03~07)
 *              + Group 6: SUM Bit Control (SUM-01~05)
 *
 * Tests:
 *   UPRIV-03: U=1 page, S-mode read with SUM=0 faults
 *   UPRIV-04: U=1 page, S-mode write with SUM=0 faults
 *   UPRIV-05: U=1 page, S-mode exec always faults (even SUM=1)
 *   UPRIV-06: U=0 page, S-mode read/write succeeds
 *   UPRIV-07: U=0 page, S-mode exec succeeds
 *   SUM-01:   SUM=1, S-mode read U-page succeeds
 *   SUM-02:   SUM=1, S-mode write U-page succeeds
 *   SUM-03:   SUM=0, S-mode read U-page faults
 *   SUM-04:   SUM=0, S-mode write U-page faults
 *   SUM-05:   SUM=1, S-mode exec U-page still faults
 */

/* ===================================================================
 * Group 5: U-bit and Privilege Access Control (UPRIV-03 ~ UPRIV-07)
 * =================================================================== */

TEST_REGISTER(test_sv57_upriv03);
bool test_sv57_upriv03(void) {
    TEST_BEGIN("UPRIV-03: U=1 page, S-mode read with SUM=0 faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("U=1, SUM=0: S-mode read faults", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_upriv04);
bool test_sv57_upriv04(void) {
    TEST_BEGIN("UPRIV-04: U=1 page, S-mode write with SUM=0 faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("U=1, SUM=0: S-mode write faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_upriv05);
bool test_sv57_upriv05(void) {
    TEST_BEGIN("UPRIV-05: U=1 page, S-mode exec always faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    init_exec_page();
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_X | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Even with SUM=1, S-mode exec of U=1 page should fault */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec_with_sum,
                                        test_va);
    TEST_ASSERT("U=1: S-mode exec faults (even SUM=1)", result == CAUSE_IPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_upriv06);
bool test_sv57_upriv06(void) {
    TEST_BEGIN("UPRIV-06: U=0 page, S-mode read/write succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,  /* U=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_and_store,
                                        test_va);
    TEST_ASSERT("U=0: S-mode read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_upriv07);
bool test_sv57_upriv07(void) {
    TEST_BEGIN("UPRIV-07: U=0 page, S-mode exec succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    init_exec_page();
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_X | PTE_A | PTE_D,  /* U=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT("U=0: S-mode exec succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * Group 6: SUM Bit Control (SUM-01 ~ SUM-05)
 * =================================================================== */

TEST_REGISTER(test_sv57_sum01);
bool test_sv57_sum01(void) {
    TEST_BEGIN("SUM-01: SUM=1, S-mode read U-page succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_with_sum,
                                        test_va);
    TEST_ASSERT("SUM=1: S-mode read U-page succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_sum02);
bool test_sv57_sum02(void) {
    TEST_BEGIN("SUM-02: SUM=1, S-mode write U-page succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_with_sum,
                                        test_va);
    TEST_ASSERT("SUM=1: S-mode write U-page succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_sum03);
bool test_sv57_sum03(void) {
    TEST_BEGIN("SUM-03: SUM=0, S-mode read U-page faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* SUM=0 by default */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("SUM=0: S-mode read U-page faults", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_sum04);
bool test_sv57_sum04(void) {
    TEST_BEGIN("SUM-04: SUM=0, S-mode write U-page faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("SUM=0: S-mode write U-page faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_sum05);
bool test_sv57_sum05(void) {
    TEST_BEGIN("SUM-05: SUM=1, S-mode exec U-page still faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    init_exec_page();
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_X | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec_with_sum,
                                        test_va);
    TEST_ASSERT("SUM=1: S-mode exec U-page still faults", result == CAUSE_IPF);

    pt_pool_reset();
    TEST_END();
}
