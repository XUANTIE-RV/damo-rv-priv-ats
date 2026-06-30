/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_ad_bits.c - Group 8: A/D Bits and NAPOT Pages
 *
 * Tests AD-01 through AD-04
 *
 * Verifies A/D bit behavior with NAPOT pages under Svade semantics.
 *
 * QEMU's "max" CPU enables Svadu by default, which causes hardware
 * to automatically update A/D bits without triggering page faults.
 * We must disable Svadu (clear menvcfg.ADUE) to test Svade behavior.
 */

/**
 * disable_svadu - Disable hardware A/D bit update (Svadu)
 *
 * Clears menvcfg.ADUE so that A=0 / D=0 pages trigger page faults
 * (Svade semantics) instead of being silently updated by hardware.
 *
 * menvcfg is CSR 0x30A. ADUE is bit 61.
 */
static void disable_svadu(void) {
    uintptr_t menvcfg;
    asm volatile("csrr %0, 0x30A" : "=r"(menvcfg));
    menvcfg &= ~MENVCFG_ADUE;
    asm volatile("csrw 0x30A, %0" :: "r"(menvcfg) : "memory");
}

/**
 * restore_svadu - Re-enable hardware A/D bit update (Svadu)
 */
static void restore_svadu(void) {
    uintptr_t menvcfg;
    asm volatile("csrr %0, 0x30A" : "=r"(menvcfg));
    menvcfg |= MENVCFG_ADUE;
    asm volatile("csrw 0x30A, %0" :: "r"(menvcfg) : "memory");
}

/* ===================================================================
 * AD-01: NAPOT A=0 triggers page-fault on load (Svade)
 * =================================================================== */
TEST_REGISTER(test_napot_ad_a0_fault);
bool test_napot_ad_a0_fault(void) {
    TEST_BEGIN("AD-01: NAPOT A=0 load triggers page fault (Svade)");

    /* Disable Svadu to enforce Svade behavior */
    disable_svadu();

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* 64 KiB NAPOT mapping: A=0, D=1 (no PTE_A) */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("NAPOT A=0 triggers load page fault",
                result == CAUSE_LOAD_PAGE_FAULT);

    restore_svadu();
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * AD-02: NAPOT D=0 write triggers page-fault (Svade)
 * =================================================================== */
TEST_REGISTER(test_napot_ad_d0_fault);
bool test_napot_ad_d0_fault(void) {
    TEST_BEGIN("AD-02: NAPOT D=0 write triggers page fault (Svade)");

    /* Disable Svadu to enforce Svade behavior */
    disable_svadu();

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* 64 KiB NAPOT mapping: A=1, D=0 (no PTE_D) */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Read should succeed (A=1) */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load, test_va);
    TEST_ASSERT("NAPOT D=0: read succeeds (A=1)", result == 0);

    /* Write should fault (D=0 under Svade) */
    result = vm_run_in_smode(&ctx, smode_store_expect_fault, test_va);
    TEST_ASSERT("NAPOT D=0 triggers store page fault",
                result == CAUSE_STORE_PAGE_FAULT);

    restore_svadu();
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * AD-03: NAPOT A=1, D=1 normal access
 * =================================================================== */
TEST_REGISTER(test_napot_ad_normal);
bool test_napot_ad_normal(void) {
    TEST_BEGIN("AD-03: NAPOT A=1, D=1 normal access");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* 64 KiB NAPOT mapping: A=1, D=1 */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("NAPOT A=1 D=1: read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * AD-04: NAPOT region A/D consistency across offsets
 * =================================================================== */
TEST_REGISTER(test_napot_ad_consistency);
bool test_napot_ad_consistency(void) {
    TEST_BEGIN("AD-04: NAPOT A/D consistency across 64 KiB region");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* 64 KiB NAPOT mapping: A=1, D=1 */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Access all 16 pages - all should succeed with A=1, D=1 */
    uintptr_t result = vm_run_in_smode(&ctx, smode_access_all_16_pages,
                                        test_va);
    TEST_ASSERT("all 16 pages accessible with A=1 D=1", result == 0);

    pt_pool_reset();
    TEST_END();
}
