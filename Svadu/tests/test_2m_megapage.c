/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_2m_megapage.c - Group 4: ADUE=1 HW updates A/D on 2 MiB megapages
 *
 * Per SPEC/svadu.adoc + translation algorithm (SPEC/supervisor.adoc:1758):
 *   Hardware A/D update applies to leaf PTEs regardless of page size,
 *   including 2 MiB megapages (level 1 in Sv39).
 *
 * Tests:
 *   SVADU-2M-01: 2M A=0 load                 -> success, PTE.A=1
 *   SVADU-2M-02: 2M A=1,D=0 store            -> success, PTE.D=1
 *   SVADU-2M-03: 2M A=0 X fetch              -> success, PTE.A=1
 *   SVADU-2M-04: 2M A=0,D=0 store            -> success, PTE.A=1 AND PTE.D=1
 *   SVADU-2M-05: 2M A=1,D=0 amoadd.w         -> success, PTE.D=1
 */

TEST_REGISTER(test_svadu_2m01);
bool test_svadu_2m01(void) {
    TEST_BEGIN("SVADU-2M-01: ADUE=1 2 MiB megapage A=0 load sets A bit");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_region_2m_pa,
                PTE_V | PTE_R,    /* A=0, D=0 */
                PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("2M load succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("megapage PTE.A set by HW", (pte & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_2m02);
bool test_svadu_2m02(void) {
    TEST_BEGIN("SVADU-2M-02: ADUE=1 2 MiB megapage A=1,D=0 store sets D bit");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_region_2m_pa,
                PTE_V | PTE_R | PTE_W | PTE_A,    /* D=0 */
                PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("2M store succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("megapage PTE.D set by HW", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_2m03);
bool test_svadu_2m03(void) {
    TEST_BEGIN("SVADU-2M-03: ADUE=1 2 MiB megapage A=0 X fetch sets A bit");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Fill the first 4K of the 2 MiB region with nop;ret. Since the
     * megapage PA starts at test_region_2m_pa and setup_code_mapping
     * skipped this region, we write directly via M-mode virtual address
     * (identity mapped in M-mode). */
    uint32_t *p = (uint32_t *)test_region_2m_pa;
    for (int i = 0; i < 1024 - 1; i++)
        p[i] = 0x00000013;  /* nop */
    p[1023] = 0x00008067;   /* ret */

    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_region_2m_pa,
                PTE_V | PTE_X,    /* A=0 */
                PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT_EQ("2M fetch succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("megapage PTE.A set by HW on fetch", (pte & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_2m04);
bool test_svadu_2m04(void) {
    TEST_BEGIN("SVADU-2M-04: ADUE=1 2 MiB megapage A=0,D=0 store sets both");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_region_2m_pa,
                PTE_V | PTE_R | PTE_W,    /* A=0, D=0 */
                PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("2M store succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("megapage PTE.A set by HW", (pte & PTE_A) != 0);
    TEST_ASSERT("megapage PTE.D set by HW", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_svadu_2m05);
bool test_svadu_2m05(void) {
    TEST_BEGIN("SVADU-2M-05: ADUE=1 2 MiB megapage A=1,D=0 amoadd sets D");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_region_2m_pa,
                PTE_V | PTE_R | PTE_W | PTE_A,    /* D=0 */
                PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_amoadd, test_va);
    TEST_ASSERT_EQ("2M amoadd succeeds under ADUE=1", result, 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("megapage PTE.D set by HW after AMO", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}
