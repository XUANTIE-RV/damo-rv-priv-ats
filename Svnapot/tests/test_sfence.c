/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_sfence.c - Group 6: SFENCE.VMA and NAPOT Page Interaction
 *
 * Tests SFENCE-01 through SFENCE-05
 *
 * Verifies SFENCE.VMA correctly flushes NAPOT TLB entries.
 */

/* ===================================================================
 * Helper: S-mode function that modifies PTE and flushes TLB
 *
 * We pass PTE modification info via global variables since
 * vm_run_in_smode only passes one argument.
 * =================================================================== */
static volatile uintptr_t *g_napot_pte_slot = NULL;
static volatile uintptr_t g_new_pte_val = 0;
static volatile uintptr_t g_sfence_va = 0;
static volatile int g_sfence_mode = 0;  /* 0=global, 1=per-page, 2=single */

/**
 * smode_modify_pte_and_sfence - Modify PTE in-place and sfence
 * @test_va: base VA of the NAPOT region
 *
 * This runs in S-mode with VM enabled. Since we have identity mapping,
 * we can directly modify the PTE in the page table.
 *
 * Since napot_install_pte() fills all 16 level-0 PTE slots for the
 * 64 KiB region, this function also updates all 16 slots with the
 * new PTE value.
 */
static uintptr_t smode_modify_pte_and_sfence(uintptr_t test_va) {
    /* Modify all 16 PTE slots covering the NAPOT region.
     * g_napot_pte_slot points to the base VA's L0 slot (VPN0 low 4 bits = 0
     * since NAPOT base is 64 KiB aligned). The next 15 slots are contiguous. */
    if (g_napot_pte_slot) {
        for (int i = 0; i < 16; i++) {
            g_napot_pte_slot[i] = g_new_pte_val;
        }
    }

    /* SFENCE.VMA */
    if (g_sfence_mode == 0) {
        /* Global flush */
        asm volatile("sfence.vma" ::: "memory");
    } else if (g_sfence_mode == 1) {
        /* Per-page flush: flush each 4KB page in 64 KiB region */
        for (int i = 0; i < 16; i++) {
            uintptr_t addr = test_va + (uintptr_t)i * PAGE_SIZE_4K;
            asm volatile("sfence.vma %0, zero" :: "r"(addr) : "memory");
        }
    } else if (g_sfence_mode == 2) {
        /* Single address flush */
        asm volatile("sfence.vma %0, zero" :: "r"(g_sfence_va) : "memory");
    }

    return 0;
}

/* ===================================================================
 * SFENCE-01: Global SFENCE.VMA flushes NAPOT cache
 * =================================================================== */
TEST_REGISTER(test_napot_sfence_global);
bool test_napot_sfence_global(void) {
    TEST_BEGIN("SFENCE-01: Global sfence.vma flushes NAPOT TLB entries");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Initial mapping: 64 KiB NAPOT R-only */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* First access - establish TLB */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load, test_va);
    TEST_ASSERT("initial read succeeds", result == 0);

    /* Verify write fails (R-only) */
    result = vm_run_in_smode(&ctx, smode_store_expect_fault, test_va);
    TEST_ASSERT("R-only: write faults", result == CAUSE_STORE_PAGE_FAULT);

    /* Modify PTE to RW directly in page table */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, PT_LEVEL_4K);
    uintptr_t *pte_ptr = (uintptr_t *)pt_page + VA_VPN0(test_va);
    uintptr_t new_pte = napot_make_pte(test_va,
                                        PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);

    /* Set up globals for S-mode PTE modification */
    g_napot_pte_slot = (volatile uintptr_t *)pte_ptr;
    g_new_pte_val = new_pte;
    g_sfence_mode = 0; /* global flush */

    /* Modify PTE and sfence in S-mode */
    vm_run_in_smode(&ctx, smode_modify_pte_and_sfence, test_va);

    /* Write should now succeed */
    result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("after sfence.vma: write succeeds", result == 0);

    g_napot_pte_slot = NULL;
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SFENCE-02a: Single global SFENCE.VMA flushes NAPOT region
 * =================================================================== */
TEST_REGISTER(test_napot_sfence_global_single);
bool test_napot_sfence_global_single(void) {
    TEST_BEGIN("SFENCE-02a: Single global sfence.vma flushes NAPOT region");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* R-only NAPOT mapping */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Establish TLB for multiple pages */
    vm_run_in_smode(&ctx, smode_load, test_va);
    vm_run_in_smode(&ctx, smode_load, test_va + 0x4000);

    /* Modify to RW */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, PT_LEVEL_4K);
    uintptr_t *pte_ptr = (uintptr_t *)pt_page + VA_VPN0(test_va);
    uintptr_t new_pte = napot_make_pte(test_va,
                                        PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    g_napot_pte_slot = (volatile uintptr_t *)pte_ptr;
    g_new_pte_val = new_pte;
    g_sfence_mode = 0;

    vm_run_in_smode(&ctx, smode_modify_pte_and_sfence, test_va);

    /* Both offsets should now be writable */
    uintptr_t r0 = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("offset 0x0: write succeeds after sfence", r0 == 0);

    uintptr_t r1 = vm_run_in_smode(&ctx, smode_store, test_va + 0x4000);
    TEST_ASSERT("offset 0x4000: write succeeds after sfence", r1 == 0);

    g_napot_pte_slot = NULL;
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SFENCE-02b: Per-page SFENCE.VMA (16 invocations) flushes NAPOT region
 * =================================================================== */
TEST_REGISTER(test_napot_sfence_per_page);
bool test_napot_sfence_per_page(void) {
    TEST_BEGIN("SFENCE-02b: Per-page sfence.vma flushes NAPOT region");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Establish TLB */
    vm_run_in_smode(&ctx, smode_load, test_va);

    /* Modify to RW */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, PT_LEVEL_4K);
    uintptr_t *pte_ptr = (uintptr_t *)pt_page + VA_VPN0(test_va);
    uintptr_t new_pte = napot_make_pte(test_va,
                                        PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    g_napot_pte_slot = (volatile uintptr_t *)pte_ptr;
    g_new_pte_val = new_pte;
    g_sfence_mode = 1; /* per-page flush */

    vm_run_in_smode(&ctx, smode_modify_pte_and_sfence, test_va);

    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("after per-page sfence: write succeeds", result == 0);

    g_napot_pte_slot = NULL;
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SFENCE-03: NAPOT mapping removal after sfence
 * =================================================================== */
TEST_REGISTER(test_napot_sfence_removal);
bool test_napot_sfence_removal(void) {
    TEST_BEGIN("SFENCE-03: NAPOT mapping removal after sfence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Establish TLB */
    vm_run_in_smode(&ctx, smode_load, test_va);

    /* Invalidate PTE (V=0) */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, PT_LEVEL_4K);
    uintptr_t *pte_ptr = (uintptr_t *)pt_page + VA_VPN0(test_va);
    g_napot_pte_slot = (volatile uintptr_t *)pte_ptr;
    g_new_pte_val = 0;  /* V=0 */
    g_sfence_mode = 0;

    vm_run_in_smode(&ctx, smode_modify_pte_and_sfence, test_va);

    /* Access should now fault */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("removed mapping: page fault",
                result == CAUSE_LOAD_PAGE_FAULT);

    g_napot_pte_slot = NULL;
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SFENCE-04: NAPOT permission upgrade (R -> RW) after sfence
 * =================================================================== */
TEST_REGISTER(test_napot_sfence_upgrade);
bool test_napot_sfence_upgrade(void) {
    TEST_BEGIN("SFENCE-04: NAPOT permission upgrade R->RW after sfence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Verify write fails before upgrade */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("before upgrade: write faults",
                result == CAUSE_STORE_PAGE_FAULT);

    /* Upgrade to RW */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, PT_LEVEL_4K);
    uintptr_t *pte_ptr = (uintptr_t *)pt_page + VA_VPN0(test_va);
    uintptr_t new_pte = napot_make_pte(test_va,
                                        PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    g_napot_pte_slot = (volatile uintptr_t *)pte_ptr;
    g_new_pte_val = new_pte;
    g_sfence_mode = 0;

    vm_run_in_smode(&ctx, smode_modify_pte_and_sfence, test_va);

    /* Write should succeed after upgrade */
    result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("after upgrade: write succeeds", result == 0);

    g_napot_pte_slot = NULL;
    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * SFENCE-05: Single-address SFENCE.VMA on NAPOT region
 * =================================================================== */
TEST_REGISTER(test_napot_sfence_single_addr);
bool test_napot_sfence_single_addr(void) {
    TEST_BEGIN("SFENCE-05: Single-address sfence.vma on NAPOT region");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Establish TLB */
    vm_run_in_smode(&ctx, smode_load, test_va);

    /* Upgrade to RW */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, PT_LEVEL_4K);
    uintptr_t *pte_ptr = (uintptr_t *)pt_page + VA_VPN0(test_va);
    uintptr_t new_pte = napot_make_pte(test_va,
                                        PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    g_napot_pte_slot = (volatile uintptr_t *)pte_ptr;
    g_new_pte_val = new_pte;
    g_sfence_mode = 2; /* single address flush */
    g_sfence_va = test_va;

    vm_run_in_smode(&ctx, smode_modify_pte_and_sfence, test_va);

    /* The flushed address should have new permissions */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("sfence single addr: write succeeds at flushed addr",
                result == 0);

    g_napot_pte_slot = NULL;
    pt_pool_reset();
    TEST_END();
}
