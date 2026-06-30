/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mapping.c - Group 1: Basic Page Table Mapping (MAP-05~07)
 *                + Group 2: Virtual Address Sign Extension (SIGN-05)
 *
 * Tests:
 *   MAP-05:  Sv48 1GB gigapage identity mapping
 *   MAP-06:  Sv48 2MB megapage identity mapping
 *   MAP-07:  Sv48 4KB page identity mapping
 *   SIGN-05: Sv48 non-canonical VA triggers page fault
 */

/* ===================================================================
 * Group 1: Basic Page Table Mapping Verification (MAP-05 ~ MAP-07)
 * =================================================================== */

TEST_REGISTER(test_sv48_map05_1g);
bool test_sv48_map05_1g(void) {
    TEST_BEGIN("MAP-05: Sv48 1GB gigapage identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

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

TEST_REGISTER(test_sv48_map06_2m);
bool test_sv48_map06_2m(void) {
    TEST_BEGIN("MAP-06: Sv48 2MB megapage identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

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

TEST_REGISTER(test_sv48_map07_4k);
bool test_sv48_map07_4k(void) {
    TEST_BEGIN("MAP-07: Sv48 4KB page identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

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
 * Group 2: Virtual Address Sign Extension (SIGN-05)
 *
 * Sv48 requires bits 63-48 to all equal bit 47.
 * 0x0000800000000000 has bit 47=0 but bit 48=1 => non-canonical.
 * =================================================================== */

TEST_REGISTER(test_sv48_sign05);
bool test_sv48_sign05(void) {
    TEST_BEGIN("SIGN-05: Sv48 non-canonical VA triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Non-canonical address: bit 47=0 but bit 48=1 */
    uintptr_t bad_va = 0x0000800000000000UL;
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, bad_va);
    TEST_ASSERT("non-canonical VA triggers page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
