/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_rsw.c - Group 12: RSW Bits Verification
 *
 * Tests RSW-01 through RSW-02
 *
 * Verifies NAPOT PTE RSW bits (9:8) can be written without
 * affecting address translation, and values are preserved.
 */

/* ===================================================================
 * RSW-01: NAPOT PTE RSW bits do not affect translation
 * =================================================================== */
TEST_REGISTER(test_napot_rsw_writable);
bool test_napot_rsw_writable(void) {
    TEST_BEGIN("RSW-01: NAPOT PTE RSW bits do not affect translation");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Test with RSW = 01, 10, 11 */
    uintptr_t rsw_values[] = {1, 2, 3};
    int n = sizeof(rsw_values) / sizeof(rsw_values[0]);

    for (int i = 0; i < n; i++) {
        pt_pool_reset();
        pt_init(&ctx, SATP_MODE_SV39);
        ret = setup_code_mapping(&ctx);

        uintptr_t test_va = NAPOT_TEST_REGION_0;
        uintptr_t napot_pte = napot_make_pte(test_va,
                                              PTE_V | PTE_R | PTE_W |
                                              PTE_A | PTE_D);
        /* Set RSW bits manually */
        napot_pte |= (rsw_values[i] << PTE_RSW_SHIFT);
        napot_install_pte(&ctx, test_va, napot_pte);

        uintptr_t result = vm_run_in_smode(&ctx, smode_read_write,
                                            test_va);
        TEST_ASSERT("RSW does not affect NAPOT translation", result == 0);
    }

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * RSW-02: NAPOT PTE RSW bits are preserved
 * =================================================================== */
TEST_REGISTER(test_napot_rsw_readback);
bool test_napot_rsw_readback(void) {
    TEST_BEGIN("RSW-02: NAPOT PTE RSW bits are preserved");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t rsw_val = 2;  /* RSW = 10 */
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_pte |= (rsw_val << PTE_RSW_SHIFT);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Read back PTE from page table and verify RSW */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PT page found", pt_page != 0);

    uintptr_t *pte_ptr = (uintptr_t *)pt_page + VA_VPN0(test_va);
    uintptr_t readback_rsw = (*pte_ptr >> PTE_RSW_SHIFT) & 0x3;
    TEST_ASSERT_EQ("RSW readback is 10", readback_rsw, rsw_val);

    pt_pool_reset();
    TEST_END();
}
