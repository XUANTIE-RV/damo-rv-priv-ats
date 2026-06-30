/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_n_zero.c - Group 5: N=0 Behavior Verification
 *
 * Tests NZERO-01 through NZERO-03
 *
 * Verifies that N=0 PTEs behave as standard non-NAPOT PTEs.
 */

/* ===================================================================
 * NZERO-01: N=0 standard 4 KiB page
 * =================================================================== */
TEST_REGISTER(test_napot_n0_standard);
bool test_napot_n0_standard(void) {
    TEST_BEGIN("NZERO-01: N=0 PTE behaves as standard 4 KiB page");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Standard 4 KiB mapping, N=0 (bit 63 clear) */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("N=0 standard 4KB page works", result == 0);

    /* Next 4KB page should not be mapped (unlike NAPOT) */
    uintptr_t next_page = test_va + PAGE_SIZE_4K;
    result = vm_run_in_smode(&ctx, smode_load_expect_fault, next_page);
    TEST_ASSERT("N=0 next page is unmapped (page fault)",
                result == CAUSE_LOAD_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NZERO-02: N=0 ppn[0]=1000 has no NAPOT effect
 * =================================================================== */
TEST_REGISTER(test_napot_n0_ppn_1000);
bool test_napot_n0_ppn_1000(void) {
    TEST_BEGIN("NZERO-02: N=0 ppn[0]=1000 no NAPOT semantics");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map a single 4KB page at base of NAPOT test region.
     * The PA happens to have ppn[0] low bits matching 1000,
     * but N=0, so no NAPOT behavior. */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* First page should work normally */
    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("N=0 first page works", result == 0);

    /* Offset 0x1000 should NOT be accessible (no NAPOT contiguity) */
    uintptr_t next_page = test_va + PAGE_SIZE_4K;
    result = vm_run_in_smode(&ctx, smode_load_expect_fault, next_page);
    TEST_ASSERT("N=0 ppn[0]=1000: no NAPOT contiguity",
                result == CAUSE_LOAD_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * NZERO-03: N=0 ppn[0] low bits not checked for NAPOT encoding
 * =================================================================== */
TEST_REGISTER(test_napot_n0_no_encoding_check);
bool test_napot_n0_no_encoding_check(void) {
    TEST_BEGIN("NZERO-03: N=0 ppn[0] low bits no NAPOT encoding check");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map a page with N=0. The ppn[0] low bits are just a normal
     * physical page frame number - no NAPOT encoding check applies. */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("N=0 normal access works (no NAPOT encoding check)",
                result == 0);

    pt_pool_reset();
    TEST_END();
}
