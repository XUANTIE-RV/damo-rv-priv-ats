/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_ordering.c - Group 2: Ordering Semantics Verification
 *
 * Tests:
 *   ORDER-01: Complete sequence ordering guarantee
 *   ORDER-02: Missing SFENCE.W.INVAL (undefined behavior, probe only)
 *   ORDER-03: Missing SFENCE.INVAL.IR (undefined behavior, probe only)
 *   ORDER-04: Complete sequence equivalence with SFENCE.VMA
 *   ORDER-05: SFENCE.VMA as ordering fence for SINVAL.VMA
 *   ORDER-06: Multiple SINVAL.VMA followed by SFENCE.VMA
 *
 * Verifies the ordering guarantees of the three-instruction sequence.
 */

/* ===================================================================
 * ORDER-01: Complete sequence ordering guarantee
 *
 * store(PTE) -> SFENCE.W.INVAL -> SINVAL.VMA -> SFENCE.INVAL.IR -> access
 * =================================================================== */
TEST_REGISTER(test_sinval_ordering_complete);
bool test_sinval_ordering_complete(void) {
    TEST_BEGIN("ORDER-01: Complete sequence ordering guarantee");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Initial mapping: RW */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* First access to build TLB entry */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("Initial write succeeds", result == 0);

    /* Modify PTE: downgrade to R-only */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Complete three-instruction sequence */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    /* Store should use new translation (R-only), trigger fault */
    result = vm_run_in_smode(&ctx, smode_store_expect_fault, test_va);
    TEST_ASSERT("After complete sequence: write faults (R-only)",
                result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ORDER-02: Missing SFENCE.W.INVAL (undefined behavior)
 *
 * NOTE: The spec does not guarantee correctness without SFENCE.W.INVAL.
 * Simple implementations may implement SINVAL.VMA identically to
 * SFENCE.VMA and SFENCE.W.INVAL as NOP, so this may still work.
 * This test only probes implementation behavior.
 * =================================================================== */
TEST_REGISTER(test_sinval_ordering_missing_w_inval);
bool test_sinval_ordering_missing_w_inval(void) {
    TEST_BEGIN("ORDER-02: Missing SFENCE.W.INVAL (probe only)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Initial mapping: RW */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Build TLB entry */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("Initial write succeeds", result == 0);

    /* Modify PTE: downgrade to R-only */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Missing SFENCE.W.INVAL: only SINVAL.VMA + SFENCE.INVAL.IR */
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    /* Behavior is undefined - just record what happens */
    result = vm_run_in_smode(&ctx, smode_store_expect_fault, test_va);
    if (result == CAUSE_SPF) {
        printf("  INFO: Missing W.INVAL: new translation took effect (impl may treat SINVAL.VMA as SFENCE.VMA)\n");
    } else {
        printf("  INFO: Missing W.INVAL: old translation still in use (result=0x%lx)\n",
               (unsigned long)result);
    }
    /* No assertion - this is a probe test */
    TEST_ASSERT("probe completed", true);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ORDER-03: Missing SFENCE.INVAL.IR (undefined behavior)
 * =================================================================== */
TEST_REGISTER(test_sinval_ordering_missing_inval_ir);
bool test_sinval_ordering_missing_inval_ir(void) {
    TEST_BEGIN("ORDER-03: Missing SFENCE.INVAL.IR (probe only)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Initial mapping: RW */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Build TLB entry */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("Initial write succeeds", result == 0);

    /* Modify PTE: downgrade to R-only */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Missing SFENCE.INVAL.IR: only SFENCE.W.INVAL + SINVAL.VMA */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);

    /* Behavior is undefined - just record what happens */
    result = vm_run_in_smode(&ctx, smode_store_expect_fault, test_va);
    if (result == CAUSE_SPF) {
        printf("  INFO: Missing INVAL.IR: new translation took effect\n");
    } else {
        printf("  INFO: Missing INVAL.IR: old translation still in use (result=0x%lx)\n",
               (unsigned long)result);
    }
    TEST_ASSERT("probe completed", true);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ORDER-04: Complete sequence equivalence with SFENCE.VMA
 *
 * Performs the same PTE modification twice: once with SFENCE.VMA,
 * once with the three-instruction sequence, and verifies identical
 * results.
 * =================================================================== */
TEST_REGISTER(test_sinval_ordering_equivalence);
bool test_sinval_ordering_equivalence(void) {
    TEST_BEGIN("ORDER-04: Complete sequence equivalence with SFENCE.VMA");

    /* --- Test with SFENCE.VMA --- */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping (sfence)", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Build TLB */
    vm_run_in_smode(&ctx, smode_load, test_va);

    /* Upgrade to RW */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Use SFENCE.VMA */
    vm_sfence_vma(test_va, 0);

    uintptr_t result_sfence = vm_run_in_smode(&ctx, smode_store, test_va);

    /* --- Test with three-instruction sequence --- */
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping (sinval)", setup_code_mapping(&ctx) == 0);

    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Build TLB */
    vm_run_in_smode(&ctx, smode_load, test_va);

    /* Upgrade to RW */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Use three-instruction sequence */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    uintptr_t result_sinval = vm_run_in_smode(&ctx, smode_store, test_va);

    /* Both should produce the same result */
    TEST_ASSERT("SFENCE.VMA result: write succeeds", result_sfence == 0);
    TEST_ASSERT("SINVAL.VMA sequence result: write succeeds", result_sinval == 0);
    TEST_ASSERT("Results are equivalent", result_sfence == result_sinval);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ORDER-05: SFENCE.VMA as ordering fence for SINVAL.VMA
 *
 * The spec states that SINVAL.VMA is ordered with respect to
 * SFENCE.VMA. So SFENCE.VMA can replace SFENCE.INVAL.IR.
 * =================================================================== */
TEST_REGISTER(test_sinval_ordering_sfence_as_fence);
bool test_sinval_ordering_sfence_as_fence(void) {
    TEST_BEGIN("ORDER-05: SFENCE.VMA as ordering fence for SINVAL.VMA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Initial mapping: R-only */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Build TLB */
    vm_run_in_smode(&ctx, smode_load, test_va);

    /* Upgrade to RW */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Use SINVAL.VMA followed by SFENCE.VMA (instead of SFENCE.INVAL.IR) */
    SINVAL_VMA(test_va, 0);
    vm_sfence_vma(0, 0);

    /* Verify new translation takes effect */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("SFENCE.VMA orders SINVAL.VMA: write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ORDER-06: Multiple SINVAL.VMA followed by SFENCE.VMA
 *
 * Modify multiple pages, issue multiple SINVAL.VMA, then use
 * SFENCE.VMA as the ordering fence.
 * =================================================================== */
TEST_REGISTER(test_sinval_ordering_multi_sinval_sfence);
bool test_sinval_ordering_multi_sinval_sfence(void) {
    TEST_BEGIN("ORDER-06: Multiple SINVAL.VMA followed by SFENCE.VMA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map two test pages: R-only */
    uintptr_t va0 = (uintptr_t)test_fault_page;
    uintptr_t va1 = (uintptr_t)test_data_area;
    pt_map_page(&ctx, va0, va0, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va1, va1, PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Build TLB entries */
    vm_run_in_smode(&ctx, smode_load, va0);
    vm_run_in_smode(&ctx, smode_load, va1);

    /* Upgrade both to RW */
    pt_map_page(&ctx, va0, va0, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va1, va1, PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);

    /* Multiple SINVAL.VMA followed by SFENCE.VMA */
    SINVAL_VMA(va0, 0);
    SINVAL_VMA(va1, 0);
    vm_sfence_vma(0, 0);

    /* Both pages should be writable */
    uintptr_t r0 = vm_run_in_smode(&ctx, smode_store, va0);
    uintptr_t r1 = vm_run_in_smode(&ctx, smode_store, va1);
    TEST_ASSERT("Page 0 write succeeds", r0 == 0);
    TEST_ASSERT("Page 1 write succeeds", r1 == 0);

    pt_pool_reset();
    TEST_END();
}
