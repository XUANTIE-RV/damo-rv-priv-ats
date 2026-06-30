/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mapping.c - Group 1: Basic Page Table Mapping (MAP-01~03)
 *                + Group 2: Virtual Address Sign Extension (SIGN-03)
 *
 * Tests:
 *   MAP-01:  1GB gigapage identity mapping
 *   MAP-02:  2MB megapage identity mapping
 *   MAP-03:  4KB page identity mapping
 *   SIGN-03: Non-canonical VA triggers page fault
 */

/* ===================================================================
 * Group 1: Basic Page Table Mapping Verification (MAP-01 ~ MAP-03)
 * =================================================================== */

TEST_REGISTER(test_sv39_map01_1g);
bool test_sv39_map01_1g(void) {
    TEST_BEGIN("MAP-01: Sv39 1GB gigapage identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("identity mapping setup", ret == 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("S-mode read/write with 1G pages", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv39_map02_2m);
bool test_sv39_map02_2m(void) {
    TEST_BEGIN("MAP-02: Sv39 2MB megapage identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE;
    uintptr_t size = 16 * PAGE_SIZE_2M;
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, size,
                                        flags, PT_LEVEL_2M);
    TEST_ASSERT("identity mapping setup", ret == 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("S-mode read/write with 2M pages", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv39_map03_4k);
bool test_sv39_map03_4k(void) {
    TEST_BEGIN("MAP-03: Sv39 4KB page identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE;
    uintptr_t size = 2 * PAGE_SIZE_2M;
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, size,
                                        flags, PT_LEVEL_4K);
    TEST_ASSERT("identity mapping setup", ret == 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("S-mode read/write with 4K pages", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * Group 2: Virtual Address Sign Extension (SIGN-03)
 * =================================================================== */

TEST_REGISTER(test_sv39_sign03);
bool test_sv39_sign03(void) {
    TEST_BEGIN("SIGN-03: Sv39 non-canonical VA triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Non-canonical address: bit 38=0 but bit 39=1 */
    uintptr_t bad_va = 0x0000004000000000UL;
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, bad_va);
    TEST_ASSERT("non-canonical VA triggers page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
