/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_sv48.c - Group 4: Sv48 page table walk verification
 *
 * Verifies hardware page-table reads succeed under Sv48 (4-level)
 * page tables for all supported page sizes.
 *
 * Tests: SSCCPTR-SV48-01 ~ SSCCPTR-SV48-06
 */

#define SV48_SKIP_IF_UNSUPPORTED() do { \
    if (!is_sv48_supported()) { \
        TEST_SKIP("Sv48 not supported on this platform"); \
    } \
} while (0)

/* --- SSCCPTR-SV48-01: Sv48 4K page S-mode load --- */
TEST_REGISTER(test_ssccptr_sv48_4k_load);
bool test_ssccptr_sv48_4k_load(void) {
    TEST_BEGIN("SSCCPTR-SV48-01: Sv48 4K page S-mode load succeeds");
    SV48_SKIP_IF_UNSUPPORTED();

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping_sv48(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("Sv48 4K page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-SV48-02: Sv48 4K page S-mode store --- */
TEST_REGISTER(test_ssccptr_sv48_4k_store);
bool test_ssccptr_sv48_4k_store(void) {
    TEST_BEGIN("SSCCPTR-SV48-02: Sv48 4K page S-mode store succeeds");
    SV48_SKIP_IF_UNSUPPORTED();

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping_sv48(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("Sv48 4K store succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-SV48-03: Sv48 4K page S-mode fetch --- */
TEST_REGISTER(test_ssccptr_sv48_4k_fetch);
bool test_ssccptr_sv48_4k_fetch(void) {
    TEST_BEGIN("SSCCPTR-SV48-03: Sv48 4K page S-mode fetch succeeds");
    SV48_SKIP_IF_UNSUPPORTED();

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping_sv48(&ctx) == 0);
    init_exec_page();

    uintptr_t test_va = (uintptr_t)test_exec_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_X | PTE_A | PTE_D, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT_EQ("Sv48 4K fetch succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-SV48-04: Sv48 2M megapage load --- */
TEST_REGISTER(test_ssccptr_sv48_2m_load);
bool test_ssccptr_sv48_2m_load(void) {
    TEST_BEGIN("SSCCPTR-SV48-04: Sv48 2M megapage S-mode load succeeds");
    SV48_SKIP_IF_UNSUPPORTED();

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, 16 * PAGE_SIZE_2M,
                              flags, PT_LEVEL_2M);

    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_va, flags, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("Sv48 2M megapage load succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-SV48-05: Sv48 1G gigapage load --- */
TEST_REGISTER(test_ssccptr_sv48_1g_load);
bool test_ssccptr_sv48_1g_load(void) {
    TEST_BEGIN("SSCCPTR-SV48-05: Sv48 1G gigapage S-mode load succeeds");
    SV48_SKIP_IF_UNSUPPORTED();

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("1G identity mapping", ret == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("Sv48 1G gigapage load succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-SV48-06: Sv48 512G terapage load --- */
TEST_REGISTER(test_ssccptr_sv48_512g_load);
bool test_ssccptr_sv48_512g_load(void) {
    TEST_BEGIN("SSCCPTR-SV48-06: Sv48 512G terapage S-mode load succeeds");
    SV48_SKIP_IF_UNSUPPORTED();

    if (!is_superpage_feasible(PT_LEVEL_512G)) {
        TEST_SKIP("512G terapage requires 512GiB-aligned MEM_BASE");
    }

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

    uintptr_t base = PLATFORM_MEM_BASE & ~(((uintptr_t)1 << 39) - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_map_page(&ctx, base, base, flags, PT_LEVEL_512G);
    TEST_ASSERT("512G mapping", ret == 0);

    /* Map UART for printf */
    uintptr_t uart_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;
    pt_map_page(&ctx, PLATFORM_UART0_BASE, PLATFORM_UART0_BASE,
                uart_flags, PT_LEVEL_4K);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("Sv48 512G terapage load succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}
