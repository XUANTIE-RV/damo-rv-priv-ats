/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_basic.c - Group 1: Basic page walk verification (Sv39, 4 KiB)
 *
 * Verifies that hardware page-table reads succeed for 4 KiB pages
 * in default main memory (cacheable + coherent PMA) under different
 * access types (load/store/fetch) and privilege modes (S/U).
 *
 * Tests: SSCCPTR-BASIC-01 ~ SSCCPTR-BASIC-06
 */

/* --- SSCCPTR-BASIC-01: Sv39 4K page S-mode load --- */
TEST_REGISTER(test_ssccptr_basic_sv39_4k_smode_load);
bool test_ssccptr_basic_sv39_4k_smode_load(void) {
    TEST_BEGIN("SSCCPTR-BASIC-01: Sv39 4K page S-mode load succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("S-mode load via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-BASIC-02: Sv39 4K page S-mode store --- */
TEST_REGISTER(test_ssccptr_basic_sv39_4k_smode_store);
bool test_ssccptr_basic_sv39_4k_smode_store(void) {
    TEST_BEGIN("SSCCPTR-BASIC-02: Sv39 4K page S-mode store succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("S-mode store via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-BASIC-03: Sv39 4K page S-mode fetch --- */
TEST_REGISTER(test_ssccptr_basic_sv39_4k_smode_fetch);
bool test_ssccptr_basic_sv39_4k_smode_fetch(void) {
    TEST_BEGIN("SSCCPTR-BASIC-03: Sv39 4K page S-mode fetch succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);
    init_exec_page();

    uintptr_t test_va = (uintptr_t)test_exec_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_X | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT_EQ("S-mode fetch via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-BASIC-04: Sv39 4K page U-mode load --- */
TEST_REGISTER(test_ssccptr_basic_sv39_4k_umode_load);
bool test_ssccptr_basic_sv39_4k_umode_load(void) {
    TEST_BEGIN("SSCCPTR-BASIC-04: Sv39 4K page U-mode load succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping_umode(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_umode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("U-mode load via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-BASIC-05: Sv39 4K page U-mode store --- */
TEST_REGISTER(test_ssccptr_basic_sv39_4k_umode_store);
bool test_ssccptr_basic_sv39_4k_umode_store(void) {
    TEST_BEGIN("SSCCPTR-BASIC-05: Sv39 4K page U-mode store succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping_umode(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_umode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("U-mode store via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-BASIC-06: Sv39 4K page U-mode fetch --- */
TEST_REGISTER(test_ssccptr_basic_sv39_4k_umode_fetch);
bool test_ssccptr_basic_sv39_4k_umode_fetch(void) {
    TEST_BEGIN("SSCCPTR-BASIC-06: Sv39 4K page U-mode fetch succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping_umode(&ctx) == 0);
    init_exec_page();

    uintptr_t test_va = (uintptr_t)test_exec_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_X | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_umode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT_EQ("U-mode fetch via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}
