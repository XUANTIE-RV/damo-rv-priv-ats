/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_superpage.c - Group 2: Superpage page walk verification (Sv39)
 *
 * Verifies that hardware page-table reads succeed for 2 MiB megapages
 * (L1 leaf PTE) and 1 GiB gigapages (L2 leaf PTE) in default main
 * memory. Superpages exercise shorter page walk paths.
 *
 * Tests: SSCCPTR-SUPER-01 ~ SSCCPTR-SUPER-06
 */

/* --- SSCCPTR-SUPER-01: Sv39 2M megapage S-mode load --- */
TEST_REGISTER(test_ssccptr_super_sv39_2m_load);
bool test_ssccptr_super_sv39_2m_load(void) {
    TEST_BEGIN("SSCCPTR-SUPER-01: Sv39 2M megapage S-mode load succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* Use 2 MiB megapage identity mapping for code+data area */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, 16 * PAGE_SIZE_2M,
                              flags, PT_LEVEL_2M);

    /* Map the dedicated 2M test region */
    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_va, flags, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("2M megapage load via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-SUPER-02: Sv39 2M megapage S-mode store --- */
TEST_REGISTER(test_ssccptr_super_sv39_2m_store);
bool test_ssccptr_super_sv39_2m_store(void) {
    TEST_BEGIN("SSCCPTR-SUPER-02: Sv39 2M megapage S-mode store succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, 16 * PAGE_SIZE_2M,
                              flags, PT_LEVEL_2M);

    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_va, flags, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("2M megapage store via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-SUPER-03: Sv39 2M megapage S-mode fetch --- */
TEST_REGISTER(test_ssccptr_super_sv39_2m_fetch);
bool test_ssccptr_super_sv39_2m_fetch(void) {
    TEST_BEGIN("SSCCPTR-SUPER-03: Sv39 2M megapage S-mode fetch succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* Fill exec page within the 2M region with nop+ret */
    init_exec_page();

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, 16 * PAGE_SIZE_2M,
                              flags, PT_LEVEL_2M);

    /* Map exec page's 2M-aligned region */
    uintptr_t exec_2m = (uintptr_t)test_exec_page & ~(PAGE_SIZE_2M - 1);
    pt_map_page(&ctx, exec_2m, exec_2m, flags, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec,
                                        (uintptr_t)test_exec_page);
    TEST_ASSERT_EQ("2M megapage fetch via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-SUPER-04: Sv39 1G gigapage S-mode load --- */
TEST_REGISTER(test_ssccptr_super_sv39_1g_load);
bool test_ssccptr_super_sv39_1g_load(void) {
    TEST_BEGIN("SSCCPTR-SUPER-04: Sv39 1G gigapage S-mode load succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("1G identity mapping", ret == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("1G gigapage load via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-SUPER-05: Sv39 1G gigapage S-mode store --- */
TEST_REGISTER(test_ssccptr_super_sv39_1g_store);
bool test_ssccptr_super_sv39_1g_store(void) {
    TEST_BEGIN("SSCCPTR-SUPER-05: Sv39 1G gigapage S-mode store succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("1G identity mapping", ret == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("1G gigapage store via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-SUPER-06: Sv39 1G gigapage S-mode fetch --- */
TEST_REGISTER(test_ssccptr_super_sv39_1g_fetch);
bool test_ssccptr_super_sv39_1g_fetch(void) {
    TEST_BEGIN("SSCCPTR-SUPER-06: Sv39 1G gigapage S-mode fetch succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    init_exec_page();

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("1G identity mapping", ret == 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec,
                                        (uintptr_t)test_exec_page);
    TEST_ASSERT_EQ("1G gigapage fetch via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}
