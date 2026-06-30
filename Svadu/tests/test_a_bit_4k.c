/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_a_bit_4k.c - Group 2: ADUE=1 HW updates PTE.A on 4 KiB pages
 *
 * Per SPEC/svadu.adoc + SPEC/machine.adoc:2236-2240 (norm:menvcfg_adue_op):
 *   When menvcfg.ADUE=1, hardware atomically sets PTE.A on first access.
 *   A=0 does NOT trigger a page-fault under this configuration.
 *
 * Tests:
 *   SVADU-A4K-01: A=0 R-only page load          -> success, PTE.A=1
 *   SVADU-A4K-02: A=0 X-only page fetch         -> success, PTE.A=1
 *   SVADU-A4K-03: A=0, D=1 RW page load         -> success, PTE.A=1, PTE.D unchanged
 *   SVADU-A4K-04: Multiple loads on A=0 page    -> all succeed, PTE.A=1
 */

TEST_REGISTER(test_svadu_a4k01);
bool test_svadu_a4k01(void) {
    TEST_BEGIN("SVADU-A4K-01: ADUE=1 load on A=0 PTE sets A bit");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R,    /* A=0, D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("load succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A set by HW", (pte & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_a4k02);
bool test_svadu_a4k02(void) {
    TEST_BEGIN("SVADU-A4K-02: ADUE=1 fetch on A=0 X page sets A bit");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);
    init_exec_page();

    uintptr_t test_va = (uintptr_t)test_exec_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_X,    /* A=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT_EQ("fetch succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A set by HW on fetch", (pte & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_a4k03);
bool test_svadu_a4k03(void) {
    TEST_BEGIN("SVADU-A4K-03: ADUE=1 load on A=0,D=1 RW page sets A, keeps D");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_D,    /* A=0, D=1 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("load succeeds", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A set by HW", (pte & PTE_A) != 0);
    TEST_ASSERT("PTE.D unchanged (load does not touch D)",
                (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_a4k04);
bool test_svadu_a4k04(void) {
    TEST_BEGIN("SVADU-A4K-04: Repeated loads on A=0 page all succeed");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R,    /* A=0, D=0 */
                PT_LEVEL_4K);

    /* First access: HW sets A. Subsequent accesses find A=1, no further
     * side effects expected. All must succeed. */
    for (int i = 0; i < 3; i++) {
        uintptr_t r = vm_run_in_smode(&ctx, test_smode_load, test_va);
        TEST_ASSERT_EQ("Each load succeeds", r, 0);
        uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
        TEST_ASSERT("PTE.A remains 1", (pte & PTE_A) != 0);
    }

    pt_pool_reset();
    TEST_END();
}
