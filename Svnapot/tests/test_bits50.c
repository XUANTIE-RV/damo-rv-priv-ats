/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_bits50.c - Group 11: PTE bits 5-0 Consistency and G bit
 *
 * Tests BITS50-03
 *
 * Verifies NAPOT PTE G=1 global translation semantics.
 * BITS50-01/02 are OS responsibility tests (unpredictable but
 * non-crashing behavior), omitted for now.
 */

/* ===================================================================
 * BITS50-03: NAPOT G=1 global translation
 * =================================================================== */
TEST_REGISTER(test_napot_global_bit);
bool test_napot_global_bit(void) {
    TEST_BEGIN("BITS50-03: NAPOT PTE G=1 global translation");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* 64 KiB NAPOT mapping with G=1 */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_G |
                                          PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Access with default ASID=0 */
    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("G=1 NAPOT read/write succeeds", result == 0);

    /* Flush TLB by ASID (non-zero ASID) - G=1 entries should survive */
    vm_sfence_vma(0, 1);  /* flush ASID=1 only */

    /* Access again - G=1 should still be cached */
    result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("G=1 NAPOT survives ASID-specific sfence", result == 0);

    pt_pool_reset();
    TEST_END();
}
