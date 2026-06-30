/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_svadu.c - Group 1: Hypervisor × Svadu Cross Tests
 *
 * Per DOCS/testplan/Hypervisor_cross_test_plan.md Group 1:
 *   HCROSS-SVADU-01: henvcfg.ADUE writability
 *   HCROSS-SVADU-02: HLV with Svadu (ADUE=1, A-bit HW update)
 *   HCROSS-SVADU-03: HSV with Svadu (ADUE=1, D-bit HW update)
 *   HCROSS-SVADU-04: HLV with Svade (ADUE=0, guest-page-fault)
 *   HCROSS-SVADU-05: menvcfg.ADUE change + HFENCE.GVMA(x0,x0) sync
 *   HCROSS-SVADU-06: menvcfg.ADUE change + HFENCE.GVMA(vmid,x0) sync
 *
 * Normative references:
 *   norm:henvcfg_adue_op         - ADUE controls VS-stage A/D HW update
 *   norm:svadu_henvcfg_adue_writable - henvcfg.ADUE must be writable
 *   norm:svadu_hfence_gvma_sync  - HFENCE.GVMA required after menvcfg.ADUE change
 */

/* ===================================================================
 * HCROSS-SVADU-01: henvcfg.ADUE writability
 *
 * Verify that henvcfg.ADUE is writable when Svadu is implemented,
 * or read-only zero when Svadu is not implemented.
 * =================================================================== */
TEST_REGISTER(test_hcross_svadu_01);
bool test_hcross_svadu_01(void) {
    TEST_BEGIN("HCROSS-SVADU-01: henvcfg.ADUE writability");

    H_REQUIRED_OR_SKIP();

    uintptr_t old_henvcfg = henvcfg_read();

    /* Try to write ADUE=1 */
    henvcfg_adue_set(1);
    int readback_1 = henvcfg_adue_read();

    /* Try to write ADUE=0 */
    henvcfg_adue_set(0);
    int readback_0 = henvcfg_adue_read();

    /* Restore original value */
    henvcfg_write(old_henvcfg);

    if (check_svadu_via_adue()) {
        /* Svadu implemented: ADUE must be writable */
        TEST_ASSERT("ADUE=1 writable", readback_1 == 1);
        TEST_ASSERT("ADUE=0 writable", readback_0 == 0);
    } else {
        /* Svadu not implemented: ADUE must be read-only zero */
        TEST_ASSERT("ADUE read-only zero without Svadu", readback_1 == 0);
    }

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SVADU-02: HLV with Svadu (ADUE=1, A-bit HW update)
 *
 * Setup: henvcfg.ADUE=1, VS-stage PTE A=0
 * Action: HS-mode executes HLV.D to read the GPA
 * Expected: Access succeeds, VS-stage PTE A-bit is set by HW
 *
 * Note: We use VS=Bare + G=Sv39x4 for simplicity. When VS=Bare,
 * there is no VS-stage translation, so we operate on G-stage PTEs.
 * However, henvcfg.ADUE controls VS-stage A/D updates. To properly
 * test this, we should use VS=Sv39 + G=Sv39x4.
 *
 * For this test, we use VS=Sv39 to properly exercise henvcfg.ADUE.
 * =================================================================== */
TEST_REGISTER(test_hcross_svadu_02);
bool test_hcross_svadu_02(void) {
    TEST_BEGIN("HCROSS-SVADU-02: HLV with Svadu (ADUE=1, A-bit update)");

    H_REQUIRED_OR_SKIP();
    SVADU_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* Setup two-stage: VS=Sv39, G=Sv39x4 */
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Enable ADUE: menvcfg.ADUE=1 + henvcfg.ADUE=1 */
    menvcfg_adue_set(1);
    henvcfg_adue_set(1);

    /* Target VA in test region */
    uintptr_t test_va = (uintptr_t)test_fault_page;

    /* Clear A-bit in VS-stage PTE */
    vs_pte_clear_ad(&ctx, test_va, PT_LEVEL_4K);

    /* Verify A=0 before HLV */
    uintptr_t pte_before = vs_pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("VS-stage PTE A=0 before HLV", (pte_before & PTE_A) == 0);

    /* HS-mode executes HLV.D to read the GPA (identity mapped) */
    trap_expect_begin();
    uint64_t val = hlv_d(test_va);
    (void)val;
    trap_expect_end();

    /* Verify no trap occurred */
    TEST_ASSERT("HLV.D succeeded without trap", !trap_was_triggered());

    /* Verify A-bit was set by hardware */
    uintptr_t pte_after = vs_pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("VS-stage PTE A-bit set by HW after HLV", (pte_after & PTE_A) != 0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SVADU-03: HSV with Svadu (ADUE=1, D-bit HW update)
 *
 * Setup: henvcfg.ADUE=1, VS-stage PTE A=1, D=0
 * Action: HS-mode executes HSV.D to write the GPA
 * Expected: Access succeeds, VS-stage PTE D-bit is set by HW
 * =================================================================== */
TEST_REGISTER(test_hcross_svadu_03);
bool test_hcross_svadu_03(void) {
    TEST_BEGIN("HCROSS-SVADU-03: HSV with Svadu (ADUE=1, D-bit update)");

    H_REQUIRED_OR_SKIP();
    SVADU_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* Setup two-stage: VS=Sv39, G=Sv39x4 */
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Enable ADUE: menvcfg.ADUE=1 + henvcfg.ADUE=1 */
    menvcfg_adue_set(1);
    henvcfg_adue_set(1);

    /* Target VA in test region */
    uintptr_t test_va = (uintptr_t)test_data_area;

    /* Set A=1, D=0 in VS-stage PTE */
    uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, test_va, PT_LEVEL_4K);
    if (pte) {
        *pte = (*pte | PTE_A) & ~PTE_D;
        hfence_vvma_all();
    }

    /* Verify A=1, D=0 before HSV */
    uintptr_t pte_before = vs_pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("VS-stage PTE A=1 before HSV", (pte_before & PTE_A) != 0);
    TEST_ASSERT("VS-stage PTE D=0 before HSV", (pte_before & PTE_D) == 0);

    /* HS-mode executes HSV.D to write the GPA */
    trap_expect_begin();
    hsv_d(test_va, 0xCAFEBABE);
    trap_expect_end();

    /* Verify no trap occurred */
    TEST_ASSERT("HSV.D succeeded without trap", !trap_was_triggered());

    /* Verify D-bit was set by hardware */
    uintptr_t pte_after = vs_pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("VS-stage PTE D-bit set by HW after HSV", (pte_after & PTE_D) != 0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SVADU-04: HLV with Svade (ADUE=0, guest-page-fault)
 *
 * Setup: henvcfg.ADUE=0, VS-stage PTE A=0
 * Action: HS-mode executes HLV.D to read the GPA
 * Expected: guest-page-fault (cause=21), HW does not update A-bit
 * =================================================================== */
TEST_REGISTER(test_hcross_svadu_04);
bool test_hcross_svadu_04(void) {
    TEST_BEGIN("HCROSS-SVADU-04: HLV with Svade (ADUE=0, guest-page-fault)");

    H_REQUIRED_OR_SKIP();
    SVADU_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* Setup two-stage: VS=Sv39, G=Sv39x4 */
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Disable ADUE: menvcfg.ADUE=0 + henvcfg.ADUE=0 */
    menvcfg_adue_set(0);
    henvcfg_adue_set(0);

    /* Target VA in test region */
    uintptr_t test_va = (uintptr_t)test_fault_page;

    /* Clear A-bit in VS-stage PTE */
    vs_pte_clear_ad(&ctx, test_va, PT_LEVEL_4K);

    /* Verify A=0 before HLV */
    uintptr_t pte_before = vs_pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("VS-stage PTE A=0 before HLV", (pte_before & PTE_A) == 0);

    /* HS-mode executes HLV.D to read the GPA */
    trap_expect_begin();
    uint64_t val = hlv_d(test_va);
    (void)val;
    trap_expect_end();

    /* Verify guest-page-fault was triggered */
    TEST_ASSERT("HLV.D triggered trap", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause is load guest-page-fault (21)",
                       trap_get_cause(), (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    }

    /* Verify A-bit was NOT set by hardware (Svade behavior) */
    uintptr_t pte_after = vs_pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("VS-stage PTE A-bit NOT set by HW (Svade)", (pte_after & PTE_A) == 0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SVADU-05: menvcfg.ADUE change + HFENCE.GVMA(x0,x0) sync
 *
 * Setup: menvcfg.ADUE=0, henvcfg.ADUE=1
 * Action: Change menvcfg.ADUE from 0 to 1, execute HFENCE.GVMA(x0,x0)
 * Expected: After HFENCE.GVMA, A-bit is updated by HW
 * =================================================================== */
TEST_REGISTER(test_hcross_svadu_05);
bool test_hcross_svadu_05(void) {
    TEST_BEGIN("HCROSS-SVADU-05: menvcfg.ADUE change + HFENCE.GVMA(x0,x0)");

    H_REQUIRED_OR_SKIP();
    SVADU_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* Setup two-stage: VS=Sv39, G=Sv39x4 */
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Initial: menvcfg.ADUE=0, henvcfg.ADUE=1 */
    menvcfg_adue_set(0);
    henvcfg_adue_set(1);

    /* Target VA in test region */
    uintptr_t test_va = (uintptr_t)test_fault_page;

    /* Clear A-bit in VS-stage PTE */
    vs_pte_clear_ad(&ctx, test_va, PT_LEVEL_4K);

    /* Change menvcfg.ADUE from 0 to 1 */
    menvcfg_adue_set(1);

    /* Execute HFENCE.GVMA(x0,x0) to synchronize across all VMIDs */
    hfence_gvma_all();

    /* VS-mode access to trigger A-bit update */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_load, test_va);

    /* Verify access succeeded */
    TEST_ASSERT_EQ("VS-mode load succeeded after HFENCE.GVMA", result, (uintptr_t)0);

    /* Verify A-bit was set by hardware */
    uintptr_t pte_after = vs_pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("VS-stage PTE A-bit set after HFENCE.GVMA", (pte_after & PTE_A) != 0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SVADU-06: menvcfg.ADUE change + HFENCE.GVMA(vmid,x0) sync
 *
 * Setup: menvcfg.ADUE=1, henvcfg.ADUE=1, VMID=5 and VMID=6
 * Action: Change menvcfg.ADUE from 1 to 0, execute HFENCE.GVMA(5,x0)
 * Expected: VMID=5 behavior follows new ADUE (fault on A=0),
 *           VMID=6 behavior is implementation-dependent
 * =================================================================== */
TEST_REGISTER(test_hcross_svadu_06);
bool test_hcross_svadu_06(void) {
    TEST_BEGIN("HCROSS-SVADU-06: menvcfg.ADUE change + HFENCE.GVMA(vmid,x0)");

    H_REQUIRED_OR_SKIP();
    SVADU_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* Setup two-stage: VS=Sv39, G=Sv39x4 */
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Initial: menvcfg.ADUE=1, henvcfg.ADUE=1 */
    menvcfg_adue_set(1);
    henvcfg_adue_set(1);

    /* Target VA in test region */
    uintptr_t test_va = (uintptr_t)test_fault_page;

    /* Clear A-bit in VS-stage PTE */
    vs_pte_clear_ad(&ctx, test_va, PT_LEVEL_4K);

    /* Change menvcfg.ADUE from 1 to 0 */
    menvcfg_adue_set(0);

    /* Execute HFENCE.GVMA(vmid=5, x0) to synchronize only VMID=5 */
    hfence_gvma(0, 5);  /* gpa_shifted=0 (all GPAs), vmid=5 */

    /* Enable two-stage with VMID=5 */
    two_stage_enable(&ctx, 5);

    /* VS-mode access with VMID=5 should fault (ADUE=0, A=0) */
    trap_expect_begin();
    uintptr_t result_vmid5 = two_stage_run_in_vs(&ctx, vs_load, test_va);
    trap_expect_end();

    /* Verify VMID=5 triggered guest-page-fault */
    TEST_ASSERT("VMID=5 triggered trap (ADUE=0)", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("VMID=5 cause is load guest-page-fault",
                       trap_get_cause(), (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    }

    /* Verify A-bit was NOT set for VMID=5 */
    uintptr_t pte_vmid5 = vs_pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("VMID=5 PTE A-bit NOT set (ADUE=0)", (pte_vmid5 & PTE_A) == 0);

    /* Note: VMID=6 behavior is implementation-dependent, not tested here */

    ts2_finish(&ctx);
    HYP_TEST_END();
}
