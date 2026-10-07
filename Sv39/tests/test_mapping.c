/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mapping.c - Group 1: Basic Page Table Mapping
 *                + Group 2: Virtual Address Sign Extension
 *
 * Shared by Sv39/Sv48/Sv57; per-mode test IDs, function names, the
 * report mode name and the non-canonical VA boundary are derived from
 * SUITE_SATP_MODE in test_helpers.h. ID mapping (vm_test_plan.md):
 *   Sv39: MAP-01/02/03, SIGN-03
 *   Sv48: MAP-05/06/07, SIGN-05
 *   Sv57: MAP-08/09/10, SIGN-07
 */

/* ===================================================================
 * Group 1: Basic Page Table Mapping Verification
 * =================================================================== */

SV_REGISTER(SFX_MAP_1G);
bool SVFN(SFX_MAP_1G)(void) {
    TEST_BEGIN(ID_MAP_1G ": " SUITE_MODE_NAME " 1GB gigapage identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SUITE_SATP_MODE);

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

SV_REGISTER(SFX_MAP_2M);
bool SVFN(SFX_MAP_2M)(void) {
    TEST_BEGIN(ID_MAP_2M ": " SUITE_MODE_NAME " 2MB megapage identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SUITE_SATP_MODE);

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

SV_REGISTER(SFX_MAP_4K);
bool SVFN(SFX_MAP_4K)(void) {
    TEST_BEGIN(ID_MAP_4K ": " SUITE_MODE_NAME " 4KB page identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SUITE_SATP_MODE);

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
 * Group 2: Virtual Address Sign Extension
 *
 * A VA whose high bits are not a valid sign extension of the mode's
 * VA width is non-canonical and must raise a page fault.
 * =================================================================== */

SV_REGISTER(SFX_SIGN_NONCANON);
bool SVFN(SFX_SIGN_NONCANON)(void) {
    TEST_BEGIN(ID_SIGN_NONCANON ": " SUITE_MODE_NAME
               " non-canonical VA triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SUITE_SATP_MODE);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Non-canonical address for this mode (see SUITE_NONCANON_VA). */
    uintptr_t bad_va = SUITE_NONCANON_VA;
    uintptr_t result = vm_run_in_smode(&ctx, probe_load, bad_va);
    TEST_ASSERT("non-canonical VA triggers page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
