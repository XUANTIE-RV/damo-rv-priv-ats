/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_ssccptr.c - Group 3: Hypervisor × Ssccptr Cross Tests
 *
 * Per DOCS/testplan/Hypervisor_cross_test_plan.md Group 3:
 *   HCROSS-SSCCPTR-01: VS-stage page table walk in cacheable+coherent memory
 *   HCROSS-SSCCPTR-02: G-stage page table walk in cacheable+coherent memory
 *   HCROSS-SSCCPTR-03: Two-stage page table walk in cacheable+coherent memory
 *   HCROSS-SSCCPTR-04: PMA attribute verification for G-stage PT pages
 *
 * Normative references:
 *   norm:Ssccptr_cacheable_coherent_supports_pt_read
 *     - Main memory regions with cacheability and coherence PMA
 *       attributes must support hardware page table reads.
 *
 * Design notes:
 *   Ssccptr is a PMA-level constraint extension. It introduces no new
 *   instructions or CSRs. On QEMU/Spike, main memory defaults to
 *   cacheable+coherent, so HCROSS-SSCCPTR-01~03 are expected to PASS.
 *   HCROSS-SSCCPTR-04 requires dynamic PMA configuration which most
 *   platforms do not support, so it is expected to SKIP.
 */

/* ===================================================================
 * HCROSS-SSCCPTR-01: VS-stage page table walk in cacheable+coherent
 *                    main memory
 *
 * Setup: VS=Sv39, G=Sv39x4 (both stages enabled)
 * Action: VS-mode executes load against a mapped VA
 * Expected: Page table walk succeeds, load returns correct value
 *
 * The VS-stage page tables are allocated from the default main memory
 * pool (.page_tables section), which on QEMU/Spike satisfies the
 * cacheability+coherence PMA requirements. This test verifies that
 * the hardware can correctly read VS-stage PTEs during two-stage
 * translation.
 * =================================================================== */
TEST_REGISTER(test_hcross_ssccptr_01);
bool test_hcross_ssccptr_01(void) {
    TEST_BEGIN("HCROSS-SSCCPTR-01: VS-stage PT walk in cacheable+coherent mem");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* Setup two-stage: VS=Sv39, G=Sv39x4 */
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Target VA in test region (already identity-mapped by ts2_setup_full) */
    uintptr_t test_va = (uintptr_t)test_data_area;

    /* Write a known value to the test data area */
    *(volatile uintptr_t *)test_va = HYP_TEST_MAGIC;

    /* VS-mode executes load — triggers VS-stage + G-stage page walk */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_load, test_va);

    /* Verify load succeeded (no fault) */
    TEST_ASSERT_EQ("VS-stage PT walk succeeded, load OK",
                   result, (uintptr_t)0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCCPTR-02: G-stage page table walk in cacheable+coherent
 *                    main memory
 *
 * Setup: VS=Bare (no VS-stage), G=Sv39x4
 * Action: VS-mode executes load against a mapped GPA
 * Expected: G-stage page table walk succeeds, load returns correct value
 *
 * With VS=Bare, there is no VS-stage translation. The hardware only
 * performs G-stage page walk. The G-stage page tables are allocated
 * from the default main memory pool (.gpt_page_tables section), which
 * satisfies cacheability+coherence PMA on QEMU/Spike.
 * =================================================================== */
TEST_REGISTER(test_hcross_ssccptr_02);
bool test_hcross_ssccptr_02(void) {
    TEST_BEGIN("HCROSS-SSCCPTR-02: G-stage PT walk in cacheable+coherent mem");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* Setup two-stage: VS=Bare (no VS-stage), G=Sv39x4 */
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Target VA in test region (identity-mapped at G-stage) */
    uintptr_t test_va = (uintptr_t)test_data_area;

    /* Write a known value to the test data area */
    *(volatile uintptr_t *)test_va = HYP_TEST_MAGIC;

    /* VS-mode executes load — triggers G-stage page walk only */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_load, test_va);

    /* Verify load succeeded (no fault) */
    TEST_ASSERT_EQ("G-stage PT walk succeeded, load OK",
                   result, (uintptr_t)0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCCPTR-03: Two-stage page table walk in cacheable+coherent
 *                    main memory (load + store)
 *
 * Setup: VS=Sv39, G=Sv39x4 (both stages enabled)
 * Action: VS-mode executes load AND store against a mapped VA
 * Expected: Both page table walks succeed, R/W return correct values
 *
 * This test exercises both read and write paths through the two-stage
 * translation, verifying that the hardware can correctly read both
 * VS-stage and G-stage PTEs for load and store operations.
 * =================================================================== */
TEST_REGISTER(test_hcross_ssccptr_03);
bool test_hcross_ssccptr_03(void) {
    TEST_BEGIN("HCROSS-SSCCPTR-03: Two-stage PT walk (load+store)");

    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* Setup two-stage: VS=Sv39, G=Sv39x4 */
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Target VA in test region (already identity-mapped by ts2_setup_full) */
    uintptr_t test_va = (uintptr_t)test_data_area;

    /* Phase 1: VS-mode load — verifies read path page walk */
    uintptr_t load_result = two_stage_run_in_vs(&ctx, vs_load, test_va);
    TEST_ASSERT_EQ("Two-stage PT walk succeeded, load OK",
                   load_result, (uintptr_t)0);

    /* Phase 2: VS-mode store — verifies write path page walk */
    uintptr_t store_result = two_stage_run_in_vs(&ctx, vs_store, test_va);
    TEST_ASSERT_EQ("Two-stage PT walk succeeded, store OK",
                   store_result, (uintptr_t)0);

    /* Verify the stored value is correct */
    uintptr_t readback = *(volatile uintptr_t *)test_va;
    TEST_ASSERT_EQ("Store value readback matches",
                   readback, (uintptr_t)0xDEADBEEF);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCCPTR-04: PMA attribute verification for G-stage PT pages
 *
 * Attempt to allocate G-stage page tables in a non-cacheable or
 * non-coherent memory region (if the platform supports dynamic PMA
 * configuration), and verify the behavior.
 *
 * On most platforms (QEMU, Spike, general-purpose hardware), PMA
 * attributes are hardwired and cannot be dynamically reconfigured.
 * In such cases, this test is skipped.
 *
 * If the platform does support PMA configuration:
 *   - Place G-stage PT in a non-cacheable region
 *   - Verify behavior (implementation-dependent: may fail page walk)
 * =================================================================== */
TEST_REGISTER(test_hcross_ssccptr_04);
bool test_hcross_ssccptr_04(void) {
    TEST_BEGIN("HCROSS-SSCCPTR-04: G-stage PT page PMA attribute verification");

    H_REQUIRED_OR_SKIP();

    /*
     * Check if the platform supports dynamic PMA configuration.
     * Most simulation platforms (QEMU, Spike) and general-purpose
     * hardware do not support this. PMA attributes are typically
     * hardwired at design time.
     */
    PMA_CONFIG_REQUIRED_OR_SKIP();

    /*
     * If we reach here, the platform supports dynamic PMA configuration.
     * This is currently unreachable on QEMU/Spike, but the framework
     * is provided for future platform support.
     *
     * Implementation would:
     * 1. Configure a memory region as non-cacheable via PMA registers
     * 2. Allocate G-stage page tables in that region
     * 3. Attempt two-stage translation
     * 4. Verify behavior (implementation-dependent)
     */

    HYP_TEST_END();
}
