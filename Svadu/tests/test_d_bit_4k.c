/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_d_bit_4k.c - Group 3: ADUE=1 HW updates PTE.D on 4 KiB pages
 *
 * Per SPEC/svadu.adoc + SPEC/machine.adoc:2236-2240 (norm:menvcfg_adue_op):
 *   When menvcfg.ADUE=1, hardware atomically sets PTE.D on store/AMO.
 *   D=0 does NOT trigger a page-fault under this configuration.
 *
 * Tests:
 *   SVADU-D4K-01: A=1, D=0 RW page store    -> success, PTE.D=1
 *   SVADU-D4K-02: A=0, D=0 RW page store    -> success, PTE.A=1 AND PTE.D=1
 *   SVADU-D4K-03: A=1, D=0 RW page amoadd   -> success, PTE.D=1
 *   SVADU-D4K-04: A=1, D=1 RW page store    -> success, A/D unchanged
 */

TEST_REGISTER(test_svadu_d4k01);
bool test_svadu_d4k01(void) {
    TEST_BEGIN("SVADU-D4K-01: ADUE=1 store on A=1,D=0 PTE sets D bit");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A,    /* A=1, D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("store succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A retained", (pte & PTE_A) != 0);
    TEST_ASSERT("PTE.D set by HW", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_d4k02);
bool test_svadu_d4k02(void) {
    TEST_BEGIN("SVADU-D4K-02: ADUE=1 store on A=0,D=0 PTE sets both A and D");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W,    /* A=0, D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("store succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A set by HW", (pte & PTE_A) != 0);
    TEST_ASSERT("PTE.D set by HW", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_d4k03);
bool test_svadu_d4k03(void) {
    TEST_BEGIN("SVADU-D4K-03: ADUE=1 amoadd on A=1,D=0 PTE sets D bit");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A,    /* A=1, D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_amoadd, test_va);
    TEST_ASSERT_EQ("amoadd succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.D set by HW after AMO", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_d4k04);
bool test_svadu_d4k04(void) {
    TEST_BEGIN("SVADU-D4K-04: ADUE=1 store on A=1,D=1 PTE succeeds, A/D unchanged");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,    /* A=1, D=1 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("store succeeds", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A remains 1", (pte & PTE_A) != 0);
    TEST_ASSERT("PTE.D remains 1", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}
