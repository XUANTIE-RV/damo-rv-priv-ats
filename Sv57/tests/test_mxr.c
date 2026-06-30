/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mxr.c - Group 7: MXR Bit Control (MXR-01 ~ MXR-05)
 *
 * Tests:
 *   MXR-01: MXR=0, read X-only page faults
 *   MXR-02: MXR=1, read X-only page succeeds
 *   MXR-03: MXR=1, read R page succeeds (no change)
 *   MXR-04: MXR=1, write X-only page still faults
 *   MXR-05: MXR=0, read RX page succeeds (R=1 sufficient)
 */

TEST_REGISTER(test_sv57_mxr01);
bool test_sv57_mxr01(void) {
    TEST_BEGIN("MXR-01: MXR=0, read X-only page faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_X | PTE_A | PTE_D,  /* X-only */
                PT_LEVEL_4K);

    /* MXR=0 by default */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("MXR=0: read X-only page faults", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_mxr02);
bool test_sv57_mxr02(void) {
    TEST_BEGIN("MXR-02: MXR=1, read X-only page succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_X | PTE_A | PTE_D,  /* X-only */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_with_mxr,
                                        test_va);
    TEST_ASSERT("MXR=1: read X-only page succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_mxr03);
bool test_sv57_mxr03(void) {
    TEST_BEGIN("MXR-03: MXR=1, read R page succeeds (no change)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_with_mxr,
                                        test_va);
    TEST_ASSERT("MXR=1: read R page succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_mxr04);
bool test_sv57_mxr04(void) {
    TEST_BEGIN("MXR-04: MXR=1, write X-only page still faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_X | PTE_A | PTE_D,  /* X-only */
                PT_LEVEL_4K);

    /* MXR only affects loads, not stores */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("MXR=1: write X-only page faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_mxr05);
bool test_sv57_mxr05(void) {
    TEST_BEGIN("MXR-05: MXR=0, read RX page succeeds (R=1 sufficient)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_X | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* MXR=0, but R=1 so load should succeed */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("MXR=0: read RX page succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
