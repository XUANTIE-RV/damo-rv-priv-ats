/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_tlb.c - Group 9: NAPOT PTE and TLB Cache Behavior
 *
 * Tests TLB-01 through TLB-03
 *
 * Verifies TLB caching behavior for NAPOT PTEs.
 */

/* ===================================================================
 * TLB-01: Single PTE walk covers entire 64 KiB region
 * =================================================================== */
TEST_REGISTER(test_napot_tlb_full_region);
bool test_napot_tlb_full_region(void) {
    TEST_BEGIN("TLB-01: Single NAPOT PTE walk covers entire 64 KiB");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Access all 16 x 4 KiB pages - TLB should cache entire region */
    uintptr_t result = vm_run_in_smode(&ctx, smode_access_all_16_pages,
                                        test_va);
    TEST_ASSERT("all 16 x 4 KiB pages accessible", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * TLB-02: NAPOT replaced by standard PTE after TLB flush
 * =================================================================== */
TEST_REGISTER(test_napot_tlb_replace_standard);
bool test_napot_tlb_replace_standard(void) {
    TEST_BEGIN("TLB-02: NAPOT replaced by standard 4 KiB PTE after flush");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;

    /* First: install NAPOT PTE covering 64 KiB */
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Verify full region accessible */
    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("NAPOT region accessible", result == 0);

    result = vm_run_in_smode(&ctx, smode_load, test_va + 0x4000);
    TEST_ASSERT("NAPOT offset 0x4000 accessible", result == 0);

    /* Replace all 16 NAPOT PTE slots with: base slot gets standard 4 KiB
     * PTE, remaining 15 slots get invalidated (V=0) */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, PT_LEVEL_4K);
    uintptr_t vpn0_base = VA_VPN0(test_va);
    uintptr_t *l0_pt = (uintptr_t *)pt_page;

    /* Base slot: standard PTE (N=0) mapping only the first 4 KiB page */
    l0_pt[vpn0_base] = PA_TO_PTE(test_va) | PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;

    /* Remaining 15 slots: invalidate */
    for (int i = 1; i < 16; i++) {
        l0_pt[vpn0_base + i] = 0;
    }

    /* Flush TLB */
    vm_sfence_vma(0, 0);

    /* First page should still be accessible (now as standard 4 KiB) */
    result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("standard 4 KiB page accessible after replace", result == 0);

    /* Other pages within the old NAPOT region should fault
     * (their PTEs are now invalid) */
    result = vm_run_in_smode(&ctx, smode_load_expect_fault,
                              test_va + 0x4000);
    TEST_ASSERT("offset 0x4000 faults after NAPOT->standard replace",
                result == CAUSE_LOAD_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * TLB-03: NAPOT V=0 triggers page-fault
 * =================================================================== */
TEST_REGISTER(test_napot_tlb_v0);
bool test_napot_tlb_v0(void) {
    TEST_BEGIN("TLB-03: NAPOT PTE V=0 triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Install NAPOT PTE with V=0 (invalid) */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_R | PTE_A | PTE_D);
    /* Note: no PTE_V set, so the PTE is invalid */
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("NAPOT V=0 triggers page fault",
                result == CAUSE_LOAD_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}
