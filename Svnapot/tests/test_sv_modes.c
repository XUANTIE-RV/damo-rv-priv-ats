/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_sv_modes.c - Group 7: Sv Mode Compatibility
 *
 * Tests MODE-01 through MODE-05
 *
 * Verifies NAPOT pages work correctly under Sv39, Sv48, Sv57 modes.
 */

/* ===================================================================
 * MODE-01: Sv39 64 KiB NAPOT
 * =================================================================== */
TEST_REGISTER(test_napot_sv39);
bool test_napot_sv39(void) {
    TEST_BEGIN("MODE-01: 64 KiB NAPOT page under Sv39");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("Sv39 64 KiB NAPOT read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * MODE-02: Sv48 64 KiB NAPOT
 * =================================================================== */
TEST_REGISTER(test_napot_sv48);
bool test_napot_sv48(void) {
    TEST_BEGIN("MODE-02: 64 KiB NAPOT page under Sv48");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

    /* Use 1GB mapping for Sv48 - simpler setup */
    int ret = setup_code_mapping_1g(&ctx);
    if (ret != 0) {
        TEST_SKIP("Sv48 not supported on this platform");
    }

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("Sv48 64 KiB NAPOT read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * MODE-03: Sv57 64 KiB NAPOT
 * =================================================================== */
TEST_REGISTER(test_napot_sv57);
bool test_napot_sv57(void) {
    TEST_BEGIN("MODE-03: 64 KiB NAPOT page under Sv57");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);

    int ret = setup_code_mapping_1g(&ctx);
    if (ret != 0) {
        TEST_SKIP("Sv57 not supported on this platform");
    }

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("Sv57 64 KiB NAPOT read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * MODE-04: Sv39 -> Sv48 switch with NAPOT preserved
 * =================================================================== */
TEST_REGISTER(test_napot_mode_switch);
bool test_napot_mode_switch(void) {
    TEST_BEGIN("MODE-04: Sv39->Sv48 switch NAPOT preserved");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping_1g(&ctx);
    TEST_ASSERT("Sv39 code mapping setup", ret == 0);

    /* Set up NAPOT under Sv39 */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("Sv39 NAPOT works before switch", result == 0);

    /* Switch to Sv48 and rebuild */
    vm_switch_mode(&ctx, SATP_MODE_SV48);

    /* Re-install NAPOT PTE (page tables were reset by mode switch) */
    napot_pte = napot_make_pte(test_va,
                                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("Sv48 NAPOT works after switch", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * MODE-05: Sv39 dependency verification
 * =================================================================== */
TEST_REGISTER(test_napot_sv39_dependency);
bool test_napot_sv39_dependency(void) {
    TEST_BEGIN("MODE-05: Svnapot requires Sv39 support");

    /* Simply verify Sv39 mode is available by initializing page tables */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    TEST_ASSERT("Sv39 mode available (root_pt allocated)",
                ctx.root_pt != NULL);
    TEST_ASSERT("Sv39 levels = 3", ctx.levels == SV39_LEVELS);

    int ret = setup_code_mapping_1g(&ctx);
    TEST_ASSERT("Sv39 identity mapping works", ret == 0);

    /* Run a simple S-mode test to verify Sv39 functions */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("Sv39 basic VM works", result == 0);

    pt_pool_reset();
    TEST_END();
}
