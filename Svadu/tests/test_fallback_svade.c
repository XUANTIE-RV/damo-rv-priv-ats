/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_fallback_svade.c - Group 6: ADUE=0 falls back to Svade behavior
 *
 * Per SPEC/svadu.adoc:12-13 (norm:Svadu_disabled_hw_update_falls_back_to_svade)
 * and SPEC/machine.adoc:2245 (norm:menvcfg_adue_op, ADUE=0 branch):
 *   When menvcfg.ADUE=0, the processor behaves like Svade:
 *     A=0           -> page-fault (access type)
 *     store, D=0    -> store/AMO page-fault
 *     instruction fetch, A=0 -> instruction page-fault
 *   Hardware does NOT update A/D bits on any access.
 *
 * Tests:
 *   SVADU-FB-01: ADUE=0, A=0 R load    -> CAUSE_LPF, PTE.A unchanged
 *   SVADU-FB-02: ADUE=0, A=1,D=0 store -> CAUSE_SPF, PTE.D unchanged
 *   SVADU-FB-03: ADUE=0, A=0 X fetch   -> CAUSE_INST_PAGE_FAULT, PTE.A unchanged
 *   SVADU-FB-04: ADUE=0, A=1,D=0 AMO   -> CAUSE_SPF, PTE.D unchanged
 *   SVADU-FB-05: ADUE=0, A=0,D=0 store -> CAUSE_SPF, both A and D unchanged
 */

TEST_REGISTER(test_svadu_fb01);
bool test_svadu_fb01(void) {
    TEST_BEGIN("SVADU-FB-01: ADUE=0 A=0 load -> CAUSE_LPF, PTE.A unchanged");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(0);  /* fallback to Svade */

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R,    /* A=0, D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("A=0 load -> CAUSE_LPF", result, CAUSE_LPF);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A remains 0 under ADUE=0", (pte & PTE_A) == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_fb02);
bool test_svadu_fb02(void) {
    TEST_BEGIN("SVADU-FB-02: ADUE=0 D=0 store -> CAUSE_SPF, PTE.D unchanged");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(0);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A,    /* D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("D=0 store -> CAUSE_SPF", result, CAUSE_SPF);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.D remains 0 under ADUE=0", (pte & PTE_D) == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_fb03);
bool test_svadu_fb03(void) {
    TEST_BEGIN("SVADU-FB-03: ADUE=0 A=0 fetch -> CAUSE_INST_PAGE_FAULT");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(0);

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
    TEST_ASSERT_EQ("A=0 fetch -> CAUSE_INST_PAGE_FAULT",
                   result, CAUSE_INST_PAGE_FAULT);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A remains 0 under ADUE=0", (pte & PTE_A) == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_fb04);
bool test_svadu_fb04(void) {
    TEST_BEGIN("SVADU-FB-04: ADUE=0 D=0 AMO -> CAUSE_SPF, PTE.D unchanged");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(0);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A,    /* D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_amoadd, test_va);
    TEST_ASSERT_EQ("D=0 AMO -> CAUSE_SPF", result, CAUSE_SPF);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.D remains 0 under ADUE=0", (pte & PTE_D) == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_fb05);
bool test_svadu_fb05(void) {
    TEST_BEGIN("SVADU-FB-05: ADUE=0 A=0,D=0 store -> CAUSE_SPF, A/D unchanged");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(0);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W,    /* A=0, D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("A=0,D=0 store -> CAUSE_SPF", result, CAUSE_SPF);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A remains 0 under ADUE=0", (pte & PTE_A) == 0);
    TEST_ASSERT("PTE.D remains 0 under ADUE=0", (pte & PTE_D) == 0);

    pt_pool_reset();
    TEST_END();
}
