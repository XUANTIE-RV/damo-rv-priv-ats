/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_ad_bits.c - Group 10: A/D Bit Management (AD-01 ~ AD-05)
 *
 * Tests (assuming Svade semantics: A/D=0 triggers page-fault):
 *   AD-01: A=0 load triggers page fault
 *   AD-02: A=0 store triggers page fault
 *   AD-03: A=1,D=0 store triggers page fault
 *   AD-04: A=1,D=0 load succeeds (D not checked for loads)
 *   AD-05: A=1,D=1 normal read/write succeeds
 *
 * Note: These tests assume Svade semantics where the hardware raises
 * a page fault when A=0 or D=0. If the hardware instead sets A/D bits
 * automatically (non-Svade), AD-01/02/03 will detect this and SKIP.
 */

/**
 * detect_svade - Check if the platform supports Svade semantics
 *
 * Maps a page with A=0 and attempts a load. If a page fault occurs,
 * Svade is supported. If the load succeeds (hardware set A automatically),
 * Svade is not supported.
 *
 * Returns true if Svade is supported, false otherwise.
 */
static bool detect_svade(void) {
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    if (setup_code_mapping(&ctx) != 0)
        return false;

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_D,  /* A=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    pt_pool_reset();

    /* If load triggered a page fault, Svade is supported */
    return (result == CAUSE_LPF);
}

TEST_REGISTER(test_sv48_ad01);
bool test_sv48_ad01(void) {
    TEST_BEGIN("AD-01: A=0 load triggers page fault (Svade)");

    if (!detect_svade()) {
        printf("  [SKIP] Platform does not support Svade (hardware sets A/D automatically)\n");
        TEST_ASSERT("skipped (no Svade support)", true);
        TEST_END();
    }

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_D,  /* A=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("A=0 triggers load page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_ad02);
bool test_sv48_ad02(void) {
    TEST_BEGIN("AD-02: A=0 store triggers page fault (Svade)");

    if (!detect_svade()) {
        printf("  [SKIP] Platform does not support Svade (hardware sets A/D automatically)\n");
        TEST_ASSERT("skipped (no Svade support)", true);
        TEST_END();
    }

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_D,  /* A=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("A=0 triggers store page fault", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_ad03);
bool test_sv48_ad03(void) {
    TEST_BEGIN("AD-03: A=1,D=0 store triggers page fault (Svade)");

    if (!detect_svade()) {
        printf("  [SKIP] Platform does not support Svade (hardware sets A/D automatically)\n");
        TEST_ASSERT("skipped (no Svade support)", true);
        TEST_END();
    }

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A,  /* D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("A=1,D=0 triggers store page fault", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_ad04);
bool test_sv48_ad04(void) {
    TEST_BEGIN("AD-04: A=1,D=0 load succeeds (D not checked for loads)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A,  /* D=0, but load doesn't check D */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("A=1,D=0: load succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_ad05);
bool test_sv48_ad05(void) {
    TEST_BEGIN("AD-05: A=1,D=1 normal read/write succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_and_store,
                                        test_va);
    TEST_ASSERT("A=1,D=1: read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
