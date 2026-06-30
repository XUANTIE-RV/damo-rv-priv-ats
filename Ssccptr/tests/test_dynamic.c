/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_dynamic.c - Group 7: Dynamic PTE modification + page walk
 *
 * Verifies that after modifying PTE contents at runtime and executing
 * SFENCE.VMA, the hardware correctly re-reads the updated PTEs during
 * subsequent page walks.
 *
 * Tests: SSCCPTR-DYN-01 ~ SSCCPTR-DYN-04
 */

/* --- SSCCPTR-DYN-01: PTE permission change (R-only -> RW) --- */
TEST_REGISTER(test_ssccptr_dyn_perm_change);
bool test_ssccptr_dyn_perm_change(void) {
    TEST_BEGIN("SSCCPTR-DYN-01: PTE permission change R->RW after SFENCE");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;

    /* Initial mapping: R-only (no W) */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Store should fail with store page fault */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("R-only page store triggers page-fault",
                   result, CAUSE_SPF);

    /* Update PTE to add W bit */
    pte_set_bits(&ctx, test_va, PT_LEVEL_4K, PTE_W);

    /* Store should now succeed */
    result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("RW page store succeeds after PTE update", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-DYN-02: PTE target PA change --- */
TEST_REGISTER(test_ssccptr_dyn_pa_change);
bool test_ssccptr_dyn_pa_change(void) {
    TEST_BEGIN("SSCCPTR-DYN-02: PTE remapped to different PA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t va = (uintptr_t)test_fault_page;
    uintptr_t pa1 = (uintptr_t)test_data_area;
    uintptr_t pa2 = (uintptr_t)test_fault_page;

    /* Write known values to both PAs */
    *(volatile uintptr_t *)pa1 = MAGIC_WRITE_A;
    *(volatile uintptr_t *)pa2 = MAGIC_WRITE_B;

    /* Map VA -> PA1 */
    pt_map_page(&ctx, va, pa1,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    uintptr_t val = vm_run_in_smode(&ctx, test_smode_load_value, va);
    TEST_ASSERT_EQ("reads PA1 value", val, MAGIC_WRITE_A);

    /* Remap VA -> PA2 by rewriting the PTE */
    uintptr_t *pte = pt_get_pte(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("PTE found", pte != NULL);
    if (pte) {
        *pte = PA_TO_PTE(pa2) | PTE_V | PTE_R | PTE_A | PTE_D;
        vm_sfence_vma(0, 0);
    }

    val = vm_run_in_smode(&ctx, test_smode_load_value, va);
    TEST_ASSERT_EQ("reads PA2 value after remap", val, MAGIC_WRITE_B);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-DYN-03: Add mapping where none existed --- */
TEST_REGISTER(test_ssccptr_dyn_add_mapping);
bool test_ssccptr_dyn_add_mapping(void) {
    TEST_BEGIN("SSCCPTR-DYN-03: Add new mapping then load succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;

    /* No mapping for test_va yet -> should page fault */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("unmapped VA triggers page fault",
                   result, CAUSE_LPF);

    /* Add the mapping */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Now load should succeed */
    result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("mapped VA load succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-DYN-04: Remove mapping (clear PTE.V) --- */
TEST_REGISTER(test_ssccptr_dyn_remove_mapping);
bool test_ssccptr_dyn_remove_mapping(void) {
    TEST_BEGIN("SSCCPTR-DYN-04: Remove mapping then load page-faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;

    /* Create valid mapping */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Load should succeed */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("mapped VA load succeeds", result, 0);

    /* Clear PTE.V to invalidate the mapping */
    pte_clear_bits(&ctx, test_va, PT_LEVEL_4K, PTE_V);

    /* Load should now page fault */
    result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("invalidated VA triggers page fault",
                   result, CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
