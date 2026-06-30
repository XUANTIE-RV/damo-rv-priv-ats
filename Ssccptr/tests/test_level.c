/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_level.c - Group 3: Per-level PTE read verification (Sv39)
 *
 * Verifies that each level of page table is correctly read during
 * a page walk. Uses SFENCE.VMA (via vm_run_in_smode) to force
 * complete page walks. PMP control group proves test effectiveness.
 *
 * Tests: SSCCPTR-LEVEL-01 ~ SSCCPTR-LEVEL-06
 */

/* --- SSCCPTR-LEVEL-01: L0 PTE read verification (4K page) --- */
TEST_REGISTER(test_ssccptr_level_l0);
bool test_ssccptr_level_l0(void) {
    TEST_BEGIN("SSCCPTR-LEVEL-01: L0 PTE read via full L2->L1->L0 walk");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("L0 PTE read succeeds (full L2->L1->L0 walk)",
                   result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-LEVEL-02: L1 PTE read verification (2M page) --- */
TEST_REGISTER(test_ssccptr_level_l1);
bool test_ssccptr_level_l1(void) {
    TEST_BEGIN("SSCCPTR-LEVEL-02: L1 PTE read via L2->L1 walk");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, 16 * PAGE_SIZE_2M,
                              flags, PT_LEVEL_2M);

    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_va, flags, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("L1 PTE read succeeds (L2->L1 walk)", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-LEVEL-03: L2 PTE read verification (1G page) --- */
TEST_REGISTER(test_ssccptr_level_l2);
bool test_ssccptr_level_l2(void) {
    TEST_BEGIN("SSCCPTR-LEVEL-03: L2 PTE read via single-level walk");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("1G identity mapping", ret == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("L2 PTE read succeeds (single-level walk)", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-LEVEL-04: PMP blocks L0 PT read (control group) --- */
TEST_REGISTER(test_ssccptr_level_l0_pmp_block);
bool test_ssccptr_level_l0_pmp_block(void) {
    TEST_BEGIN("SSCCPTR-LEVEL-04: PMP blocks L0 PT read -> access fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Get L0 page table page physical address */
    uintptr_t pt_page_pa = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("L0 PT page found", pt_page_pa != 0);

    /* PMP: set L0 PT page to X-only (block reads) */
    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(pt_page_pa, PAGE_SIZE_4K, PMP_X);
    pmp_set_entry(0, &e0);

    pmp_entry_t e15 = PMP_ENTRY_NAPOT(0,
        (uintptr_t)1 << (__riscv_xlen - 1), PMP_RWX);
    pmp_set_entry(15, &e15);

    /* With PMP blocking L0 PT reads, page walk should fail */
    vm_enable(&ctx, 0);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_load8(test_va));
    goto_priv(PRIV_M);
    CHECK_TRAP("PMP blocks L0 PT read", CAUSE_LAF);

    vm_disable();
    pmp_clear_all();
    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-LEVEL-05: PMP blocks L1 PT read (control group) --- */
TEST_REGISTER(test_ssccptr_level_l1_pmp_block);
bool test_ssccptr_level_l1_pmp_block(void) {
    TEST_BEGIN("SSCCPTR-LEVEL-05: PMP blocks L1 PT read -> access fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Get L1 page table page physical address */
    uintptr_t pt_page_pa = get_pt_page_addr(&ctx, test_va, 1);
    TEST_ASSERT("L1 PT page found", pt_page_pa != 0);

    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(pt_page_pa, PAGE_SIZE_4K, PMP_X);
    pmp_set_entry(0, &e0);

    pmp_entry_t e15 = PMP_ENTRY_NAPOT(0,
        (uintptr_t)1 << (__riscv_xlen - 1), PMP_RWX);
    pmp_set_entry(15, &e15);

    vm_enable(&ctx, 0);

    /* Use MPRV to test S-mode load from M-mode. Cannot use goto_priv(PRIV_S)
     * because the L1 PT page is shared between code and test regions (same
     * VPN[2]), so S-mode code fetch page walk would also fail. MPRV=1 with
     * MPP=S makes load/store use S-mode translation while instruction fetch
     * remains in M-mode (unaffected by VM/PMP). */
    {
        uintptr_t mstatus_val = CSRR(mstatus);
        uintptr_t new_mstatus = mstatus_val;
        new_mstatus |= MSTATUS_MPRV_BIT;
        new_mstatus &= ~MSTATUS_MPP_MASK;
        new_mstatus |= (1UL << MSTATUS_MPP_OFF);  /* MPP=S */

        trap_expect_begin();
        asm volatile (
            "csrw mstatus, %[new_ms]\n\t"
            "lb   t0, 0(%[addr])\n\t"
            "csrw mstatus, %[old_ms]\n\t"
            :
            : [new_ms] "r" (new_mstatus),
              [old_ms] "r" (mstatus_val),
              [addr]   "r" (test_va)
            : "t0", "memory"
        );
        trap_expect_end();
    }
    CHECK_TRAP("PMP blocks L1 PT read", CAUSE_LAF);

    vm_disable();
    pmp_clear_all();
    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-LEVEL-06: PMP blocks root PT read (control group) --- */
TEST_REGISTER(test_ssccptr_level_root_pmp_block);
bool test_ssccptr_level_root_pmp_block(void) {
    TEST_BEGIN("SSCCPTR-LEVEL-06: PMP blocks root PT read -> access fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Root PT page = ctx.root_pt */
    uintptr_t pt_page_pa = (uintptr_t)ctx.root_pt;
    TEST_ASSERT("root PT page found", pt_page_pa != 0);

    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(pt_page_pa, PAGE_SIZE_4K, PMP_X);
    pmp_set_entry(0, &e0);

    pmp_entry_t e15 = PMP_ENTRY_NAPOT(0,
        (uintptr_t)1 << (__riscv_xlen - 1), PMP_RWX);
    pmp_set_entry(15, &e15);

    vm_enable(&ctx, 0);

    /* Use MPRV to test S-mode load from M-mode. Cannot use goto_priv(PRIV_S)
     * because the root PT is shared by all addresses, so S-mode code fetch
     * page walk would also fail. MPRV=1 with MPP=S makes load/store use
     * S-mode translation while instruction fetch remains in M-mode. */
    {
        uintptr_t mstatus_val = CSRR(mstatus);
        uintptr_t new_mstatus = mstatus_val;
        new_mstatus |= MSTATUS_MPRV_BIT;
        new_mstatus &= ~MSTATUS_MPP_MASK;
        new_mstatus |= (1UL << MSTATUS_MPP_OFF);  /* MPP=S */

        trap_expect_begin();
        asm volatile (
            "csrw mstatus, %[new_ms]\n\t"
            "lb   t0, 0(%[addr])\n\t"
            "csrw mstatus, %[old_ms]\n\t"
            :
            : [new_ms] "r" (new_mstatus),
              [old_ms] "r" (mstatus_val),
              [addr]   "r" (test_va)
            : "t0", "memory"
        );
        trap_expect_end();
    }
    CHECK_TRAP("PMP blocks root PT read", CAUSE_LAF);

    vm_disable();
    pmp_clear_all();
    pt_pool_reset();
    TEST_END();
}
