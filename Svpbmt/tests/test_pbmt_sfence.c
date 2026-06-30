/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pbmt_sfence.c - Group 8: SFENCE.VMA and PBMT
 *
 * Tests SFENCE-01 through SFENCE-04
 *
 * Verifies that SFENCE.VMA correctly flushes TLB entries when
 * the PBMT field of a PTE is modified.
 */

/* ===================================================================
 * SFENCE-01: PBMT change PMA->NC with global sfence.vma
 * =================================================================== */
TEST_REGISTER(test_pbmt_sfence_pma_to_nc);
bool test_pbmt_sfence_pma_to_nc(void) {
    TEST_BEGIN("SFENCE-01: Change PBMT PMA->NC with sfence.vma");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Initial mapping: PBMT=PMA */
    uintptr_t test_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_PMA,
                PT_LEVEL_4K);

    /* First access to establish TLB */
    vm_run_in_smode(&ctx, test_smode_load, test_va);

    /* Modify PTE: PBMT change to NC */
    uintptr_t *pte = pt_get_pte(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("found leaf PTE", pte != NULL);
    if (pte) {
        g_pte_slot = (volatile uintptr_t *)pte;
        g_new_pte_val = PA_TO_PTE(test_va)
                        | PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC;
    }

    /* Modify PTE and sfence in S-mode */
    vm_run_in_smode(&ctx, smode_modify_pte_and_sfence_global, test_va);

    /* Verify new attribute takes effect (NC should still allow access) */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("After PBMT PMA->NC + sfence: access succeeds",
                result == 0);

    g_pte_slot = NULL;
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SFENCE-02: PBMT change PMA->IO with address-specific sfence.vma
 * =================================================================== */
TEST_REGISTER(test_pbmt_sfence_pma_to_io);
bool test_pbmt_sfence_pma_to_io(void) {
    TEST_BEGIN("SFENCE-02: Change PBMT PMA->IO with sfence.vma addr");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Initial mapping: PBMT=PMA */
    uintptr_t test_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_PMA,
                PT_LEVEL_4K);

    /* First access to establish TLB */
    vm_run_in_smode(&ctx, test_smode_load, test_va);

    /* Modify PTE: PBMT change to IO */
    uintptr_t *pte = pt_get_pte(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("found leaf PTE", pte != NULL);
    if (pte) {
        g_pte_slot = (volatile uintptr_t *)pte;
        g_new_pte_val = PA_TO_PTE(test_va)
                        | PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_IO;
    }

    /* Modify PTE and sfence by address in S-mode */
    vm_run_in_smode(&ctx, smode_modify_pte_and_sfence_addr, test_va);

    /* Verify IO attribute takes effect */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("After PBMT PMA->IO + sfence addr: access succeeds",
                result == 0);

    g_pte_slot = NULL;
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SFENCE-03: PBMT change NC->reserved with sfence.vma
 * =================================================================== */
TEST_REGISTER(test_pbmt_sfence_to_reserved);
bool test_pbmt_sfence_to_reserved(void) {
    TEST_BEGIN("SFENCE-03: Change PBMT NC->reserved with sfence.vma");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Initial mapping: PBMT=NC */
    uintptr_t test_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC,
                PT_LEVEL_4K);

    /* First access to establish TLB */
    vm_run_in_smode(&ctx, test_smode_load, test_va);

    /* Modify PTE: PBMT change to reserved (3) */
    uintptr_t *pte = pt_get_pte(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("found leaf PTE", pte != NULL);
    if (pte) {
        g_pte_slot = (volatile uintptr_t *)pte;
        g_new_pte_val = PA_TO_PTE(test_va)
                        | PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_RSVD;
    }

    /* Modify PTE and global sfence in S-mode */
    vm_run_in_smode(&ctx, smode_modify_pte_and_sfence_global, test_va);

    /* Verify reserved value triggers page-fault */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("After PBMT->reserved + sfence: page-fault",
                result == CAUSE_LPF);

    g_pte_slot = NULL;
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SFENCE-04: PBMT change reserved->PMA with sfence.vma (recovery)
 * =================================================================== */
TEST_REGISTER(test_pbmt_sfence_from_reserved);
bool test_pbmt_sfence_from_reserved(void) {
    TEST_BEGIN("SFENCE-04: Change PBMT reserved->PMA with sfence.vma");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Initial mapping: PBMT=RSVD (reserved) */
    uintptr_t test_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_RSVD,
                PT_LEVEL_4K);

    /* Verify access faults */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=reserved: initial load faults", result == CAUSE_LPF);

    /* Modify PTE: PBMT change to PMA (recover) */
    uintptr_t *pte = pt_get_pte(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("found leaf PTE", pte != NULL);
    if (pte) {
        g_pte_slot = (volatile uintptr_t *)pte;
        g_new_pte_val = PA_TO_PTE(test_va)
                        | PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_PMA;
    }

    /* Modify PTE and sfence in S-mode */
    vm_run_in_smode(&ctx, smode_modify_pte_and_sfence_global, test_va);

    /* Verify access now succeeds */
    result = vm_run_in_smode(&ctx, test_smode_read_write, test_va);
    TEST_ASSERT("After PBMT reserved->PMA + sfence: access succeeds",
                result == 0);

    g_pte_slot = NULL;
    pt_pool_reset();
    TEST_END();
}
