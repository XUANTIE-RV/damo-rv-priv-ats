/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_1g_gigapage.c - Group 5: ADUE=1 HW updates A/D on 1 GiB gigapages
 *
 * Strategy mirrors svade/test_1g_gigapage.c:
 *   Build a context with the code-segment 1 GiB identity-mapped RWXAD,
 *   UART identity-mapped, and a SEPARATE test VA at code_base + 1G that
 *   maps back to the same 1 GiB physical region with caller-supplied
 *   flags (A=0 / D=0 to exercise Svadu HW update on gigapage).
 *
 * Tests:
 *   SVADU-1G-01: 1G A=0 load                  -> success, PTE.A=1
 *   SVADU-1G-02: 1G A=1,D=0 store             -> success, PTE.D=1
 *   SVADU-1G-03: 1G A=0,D=0 store             -> success, PTE.A=1 AND PTE.D=1
 *   SVADU-1G-04: 1G A=1,D=0 amoadd.w          -> success, PTE.D=1
 */

/* Set up: code 1G fully accessible (RWXAD) + UART 4K + test VA 1G at
 * code_base + 1G with caller-supplied flags. Returns the test VA. */
static uintptr_t setup_1g_dual_mapping_svadu(pt_context_t *ctx,
                                             uintptr_t test_flags) {
    pt_pool_reset();
    pt_init(ctx, SATP_MODE_SV39);

    /* Round UP to next 1GB boundary so the gigapage region is fully
     * backed by real DRAM. For 1GB-aligned MEM_BASE this is a no-op. */
    uintptr_t code_base  = (PLATFORM_MEM_BASE + PAGE_SIZE_1G - 1) & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_map_page(ctx, code_base, code_base, code_flags, PT_LEVEL_1G);

    /* If MEM_BASE is below code_base, the running code/stack/page-table
     * pool lives below the gigapage. Cover it with 2MB identity pages. */
    if (PLATFORM_MEM_BASE < code_base) {
        uintptr_t lo = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
        for (uintptr_t a = lo; a < code_base; a += PAGE_SIZE_2M)
            pt_map_page(ctx, a, a, code_flags, PT_LEVEL_2M);
    }

    uintptr_t uart_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;
    pt_map_page(ctx, PLATFORM_UART0_BASE, PLATFORM_UART0_BASE,
                uart_flags, PT_LEVEL_4K);

    uintptr_t test_va = code_base + PAGE_SIZE_1G;
    pt_map_page(ctx, test_va, code_base, test_flags, PT_LEVEL_1G);

    return test_va;
}

TEST_REGISTER(test_svadu_1g01);
bool test_svadu_1g01(void) {
    TEST_BEGIN("SVADU-1G-01: ADUE=1 1 GiB gigapage A=0 load sets A bit");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    uintptr_t test_va = setup_1g_dual_mapping_svadu(&ctx,
                            PTE_V | PTE_R);   /* A=0, D=0 */

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("1G load succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_1G);
    TEST_ASSERT("gigapage PTE.A set by HW", (pte & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_1g02);
bool test_svadu_1g02(void) {
    TEST_BEGIN("SVADU-1G-02: ADUE=1 1 GiB gigapage A=1,D=0 store sets D bit");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    uintptr_t test_va = setup_1g_dual_mapping_svadu(&ctx,
                            PTE_V | PTE_R | PTE_W | PTE_A);  /* D=0 */

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("1G store succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_1G);
    TEST_ASSERT("gigapage PTE.D set by HW", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_1g03);
bool test_svadu_1g03(void) {
    TEST_BEGIN("SVADU-1G-03: ADUE=1 1 GiB gigapage A=0,D=0 store sets both");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    uintptr_t test_va = setup_1g_dual_mapping_svadu(&ctx,
                            PTE_V | PTE_R | PTE_W);  /* A=0, D=0 */

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("1G store succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_1G);
    TEST_ASSERT("gigapage PTE.A set by HW", (pte & PTE_A) != 0);
    TEST_ASSERT("gigapage PTE.D set by HW", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_1g04);
bool test_svadu_1g04(void) {
    TEST_BEGIN("SVADU-1G-04: ADUE=1 1 GiB gigapage A=1,D=0 amoadd sets D");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    uintptr_t test_va = setup_1g_dual_mapping_svadu(&ctx,
                            PTE_V | PTE_R | PTE_W | PTE_A);  /* D=0 */

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_amoadd, test_va);
    TEST_ASSERT_EQ("1G amoadd succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_1G);
    TEST_ASSERT("gigapage PTE.D set by HW after AMO", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}
