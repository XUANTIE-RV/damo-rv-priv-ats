/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_runtime_switch.c - Group 7: Runtime switching of menvcfg.ADUE
 *
 * Per SPEC/hypervisor.adoc:2109-2111 (norm:Svadu_runtime_switch_requires_sfence):
 *   After modifying menvcfg.ADUE on non-Hyp platforms, SFENCE.VMA(x0,x0)
 *   must be issued before the new setting is guaranteed to take effect.
 *   set_menvcfg_adue() helper already performs this fence.
 *
 * Tests:
 *   SVADU-SW-01: 0->1 switch: first fault, retry succeeds after enabling
 *   SVADU-SW-02: 1->0 switch: first HW-set succeeds, then new A=0 page faults
 *   SVADU-SW-03: final fence makes switch visible (ADUE=0 then ADUE=1+fence)
 *   SVADU-SW-04: toggling ADUE multiple times stays consistent with current value
 */

TEST_REGISTER(test_svadu_sw01);
bool test_svadu_sw01(void) {
    TEST_BEGIN("SVADU-SW-01: ADUE 0->1 switch then retry succeeds");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(0);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R,    /* A=0, D=0 */
                PT_LEVEL_4K);

    /* First access under ADUE=0 -> load page-fault */
    uintptr_t r1 = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("first load faults under ADUE=0", r1, CAUSE_LPF);

    uintptr_t pte0 = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A still 0 after fault", (pte0 & PTE_A) == 0);

    /* Switch ADUE=1 (set_menvcfg_adue internally issues SFENCE.VMA) */
    set_menvcfg_adue(1);

    /* Second access under ADUE=1 -> success, HW sets PTE.A */
    uintptr_t r2 = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("retry load succeeds under ADUE=1", r2, 0);

    uintptr_t pte1 = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A set by HW after switch", (pte1 & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_sw02);
bool test_svadu_sw02(void) {
    TEST_BEGIN("SVADU-SW-02: ADUE 1->0 switch then new A=0 page faults");
    SVADU_REQUIRED_OR_SKIP();

    /* Phase 1: ADUE=1, first page succeeds and gets A set */
    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t p1_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, p1_va, p1_va,
                PTE_V | PTE_R,    /* A=0 */
                PT_LEVEL_4K);

    uintptr_t r_p1 = vm_run_in_smode(&ctx, test_smode_load, p1_va);
    TEST_ASSERT_EQ("P1 access succeeds under ADUE=1", r_p1, 0);

    uintptr_t pte_p1 = pte_read(&ctx, p1_va, PT_LEVEL_4K);
    TEST_ASSERT("P1.A set by HW", (pte_p1 & PTE_A) != 0);

    /* Phase 2: switch to ADUE=0 and map a second A=0 page (different VA).
     * The 2 MiB megapage region is also skipped by setup_code_mapping and
     * can serve as an alternate A=0 4KiB target. */
    set_menvcfg_adue(0);

    uintptr_t p2_va = test_region_2m_va;   /* distinct from p1 */
    pt_map_page(&ctx, p2_va, p2_va,
                PTE_V | PTE_R,    /* A=0 */
                PT_LEVEL_4K);

    uintptr_t r_p2 = vm_run_in_smode(&ctx, test_smode_load, p2_va);
    TEST_ASSERT_EQ("P2 access faults under ADUE=0", r_p2, CAUSE_LPF);

    uintptr_t pte_p2 = pte_read(&ctx, p2_va, PT_LEVEL_4K);
    TEST_ASSERT("P2.A still 0 under ADUE=0", (pte_p2 & PTE_A) == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_sw03);
bool test_svadu_sw03(void) {
    TEST_BEGIN("SVADU-SW-03: final fence after ADUE changes takes effect");
    SVADU_REQUIRED_OR_SKIP();

    /* Conservative strategy: we do not test "stale-before-fence" behavior
     * (implementation may or may not observe change without fence). We only
     * verify that AFTER the final fence, the new ADUE value governs access. */
    set_menvcfg_adue(0);   /* baseline: disabled */
    set_menvcfg_adue(1);   /* enable (includes fence) */

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R,    /* A=0 */
                PT_LEVEL_4K);

    uintptr_t r = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("load succeeds after final ADUE=1+fence", r, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A set by HW", (pte & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_sw04);
bool test_svadu_sw04(void) {
    TEST_BEGIN("SVADU-SW-04: multiple ADUE toggles remain consistent");
    SVADU_REQUIRED_OR_SKIP();

    /* Four iterations alternating ADUE=1 (must succeed+set A) and
     * ADUE=0 (must fault, A unchanged). Each iteration uses a fresh
     * page-table context so PTE.A state is always 0 at start. */
    for (int i = 0; i < 4; i++) {
        int adue = i & 1;         /* 0, 1, 0, 1 */
        set_menvcfg_adue(adue);

        pt_context_t ctx;
        pt_pool_reset();
        pt_init(&ctx, SATP_MODE_SV39);
        TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

        uintptr_t test_va = (uintptr_t)test_fault_page;
        pt_map_page(&ctx, test_va, test_va,
                    PTE_V | PTE_R,    /* A=0 */
                    PT_LEVEL_4K);

        uintptr_t r = vm_run_in_smode(&ctx, test_smode_load, test_va);
        uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);

        if (adue) {
            TEST_ASSERT_EQ("iter ADUE=1 load succeeds", r, 0);
            TEST_ASSERT("iter ADUE=1 PTE.A set by HW",
                        (pte & PTE_A) != 0);
        } else {
            TEST_ASSERT_EQ("iter ADUE=0 load -> CAUSE_LPF", r, CAUSE_LPF);
            TEST_ASSERT("iter ADUE=0 PTE.A remains 0",
                        (pte & PTE_A) == 0);
        }

        pt_pool_reset();
    }

    TEST_END();
}
