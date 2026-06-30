/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_nonleaf_pbmt.c - Group 3: Non-leaf PTE PBMT Bit Check
 *
 * Tests NLPTE-01 through NLPTE-05
 *
 * Verifies that non-leaf PTEs (intermediate page table pointers)
 * with non-zero PBMT bits (62-61) trigger page-fault.
 */

/* ===================================================================
 * NLPTE-01: Non-leaf PTE PBMT=NC (01) triggers page-fault
 * =================================================================== */
TEST_REGISTER(test_nonleaf_pbmt_nc_fault);
bool test_nonleaf_pbmt_nc_fault(void) {
    TEST_BEGIN("NLPTE-01: Non-leaf PTE PBMT=NC triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map test page using 4KB page (creates intermediate non-leaf PTEs) */
    uintptr_t test_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Find the non-leaf PTE at level 1 (2MB level) and set PBMT=NC */
    uintptr_t *nonleaf_pte = pt_get_pte(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("found non-leaf PTE", nonleaf_pte != NULL);
    if (nonleaf_pte)
        *nonleaf_pte |= PBMT_NC;

    /* Flush TLB */
    vm_sfence_vma(0, 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("Non-leaf PTE PBMT=NC triggers page-fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NLPTE-02: Non-leaf PTE PBMT=IO (10) triggers page-fault
 * =================================================================== */
TEST_REGISTER(test_nonleaf_pbmt_io_fault);
bool test_nonleaf_pbmt_io_fault(void) {
    TEST_BEGIN("NLPTE-02: Non-leaf PTE PBMT=IO triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Set PBMT=IO on the non-leaf PTE at level 1 */
    uintptr_t *nonleaf_pte = pt_get_pte(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("found non-leaf PTE", nonleaf_pte != NULL);
    if (nonleaf_pte)
        *nonleaf_pte |= PBMT_IO;

    vm_sfence_vma(0, 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("Non-leaf PTE PBMT=IO triggers page-fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NLPTE-03: Non-leaf PTE PBMT=3 (11) triggers page-fault
 * =================================================================== */
TEST_REGISTER(test_nonleaf_pbmt_rsvd_fault);
bool test_nonleaf_pbmt_rsvd_fault(void) {
    TEST_BEGIN("NLPTE-03: Non-leaf PTE PBMT=3 triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Set PBMT=3 (reserved) on the non-leaf PTE at level 1 */
    uintptr_t *nonleaf_pte = pt_get_pte(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("found non-leaf PTE", nonleaf_pte != NULL);
    if (nonleaf_pte)
        *nonleaf_pte |= PBMT_RSVD;

    vm_sfence_vma(0, 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("Non-leaf PTE PBMT=3 triggers page-fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NLPTE-04: Non-leaf PTE PBMT=0 (compliant) - normal access
 * =================================================================== */
TEST_REGISTER(test_nonleaf_pbmt_zero_ok);
bool test_nonleaf_pbmt_zero_ok(void) {
    TEST_BEGIN("NLPTE-04: Non-leaf PTE PBMT=0 allows normal access");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map test page at 4KB level - non-leaf PTEs have PBMT=0 by default */
    uintptr_t test_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Verify the non-leaf PTE at level 1 has PBMT=0 */
    uintptr_t *nonleaf_pte = pt_get_pte(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("found non-leaf PTE", nonleaf_pte != NULL);
    if (nonleaf_pte)
        TEST_ASSERT("non-leaf PTE PBMT=0",
                     PTE_PBMT(*nonleaf_pte) == 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("Non-leaf PTE PBMT=0: normal access succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NLPTE-05: Multi-level non-leaf PTE PBMT check
 *
 * In Sv39, level 2 is root. For 4KB mapping, level 2 PTE (root) is
 * a non-leaf PTE pointing to L1 page table. Set PBMT!=0 on it.
 *
 * IMPORTANT: We must use a virtual address in a DIFFERENT 1GB region
 * from the code. If test_va and code share the same root PTE,
 * modifying the root PTE's PBMT will also break code fetch, causing
 * an infinite trap loop. We map test_data_area's PA to a VA in a
 * separate 1GB region (0xC0000000).
 * =================================================================== */
TEST_REGISTER(test_nonleaf_pbmt_root_fault);
bool test_nonleaf_pbmt_root_fault(void) {
    TEST_BEGIN("NLPTE-05: Root-level non-leaf PTE PBMT!=0 triggers fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Use a virtual address in a different 1GB region (0xC0000000)
     * so that modifying its root PTE won't affect code execution
     * (code is in the 0x80000000 1GB region). */
    uintptr_t test_pa = (uintptr_t)test_data_area;
    uintptr_t test_va = 0xC0000000UL;

    /* Map test page at 4KB level in the different 1GB region */
    pt_map_page(&ctx, test_va, test_pa,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Set PBMT=NC on the root-level (level 2) non-leaf PTE for test_va.
     * This root PTE is in a different 1GB slot from the code. */
    uintptr_t *root_pte = pt_get_pte(&ctx, test_va, PT_LEVEL_1G);
    TEST_ASSERT("found root non-leaf PTE", root_pte != NULL);

    if (root_pte) {
        /* Verify it's a non-leaf PTE (no R/W/X) */
        TEST_ASSERT("root PTE is non-leaf",
                     !PTE_IS_LEAF(*root_pte));
        *root_pte |= PBMT_NC;
    }

    vm_sfence_vma(0, 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("Root non-leaf PTE PBMT!=0 triggers page-fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
