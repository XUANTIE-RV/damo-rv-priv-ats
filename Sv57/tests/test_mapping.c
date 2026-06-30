/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mapping.c - Group 1: Basic Page Table Mapping (MAP-08~10)
 *                + Group 2: Virtual Address Sign Extension (SIGN-07)
 *
 * Tests:
 *   MAP-08:  1GB gigapage identity mapping
 *   MAP-09:  2MB megapage identity mapping
 *   MAP-10:  4KB page identity mapping
 *   SIGN-07: Non-canonical VA triggers page fault
 */

/* ===================================================================
 * Group 1: Basic Page Table Mapping Verification (MAP-08 ~ MAP-10)
 * =================================================================== */

TEST_REGISTER(test_sv57_map08_1g);
bool test_sv57_map08_1g(void) {
    TEST_BEGIN("MAP-08: Sv57 1GB gigapage identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);

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

TEST_REGISTER(test_sv57_map09_2m);
bool test_sv57_map09_2m(void) {
    TEST_BEGIN("MAP-09: Sv57 2MB megapage identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);

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

TEST_REGISTER(test_sv57_map10_4k);
bool test_sv57_map10_4k(void) {
    TEST_BEGIN("MAP-10: Sv57 4KB page identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);

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
 * Group 2: Virtual Address Sign Extension (SIGN-07)
 * =================================================================== */

TEST_REGISTER(test_sv57_sign07);
bool test_sv57_sign07(void) {
    TEST_BEGIN("SIGN-07: Sv57 non-canonical VA triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /*
     * Non-canonical address for Sv57:
     * Sv57 requires bits 63-57 to equal bit 56.
     * 0x0100000000000000UL has bit 56=0 but bit 57=1, so it's non-canonical.
     */
    uintptr_t bad_va = 0x0100000000000000UL;
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, bad_va);
    TEST_ASSERT("non-canonical VA triggers page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
