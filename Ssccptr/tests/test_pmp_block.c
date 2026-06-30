/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pmp_block.c - Group 8: PMP blocks page walk (control group)
 *
 * Uses PMP to block read access to page table pages, verifying that
 * page walks are correctly blocked (access fault). This serves as
 * a control group for Groups 1-3, proving that normal test success
 * genuinely depends on successful page walk completion.
 *
 * Tests: SSCCPTR-PMP-01 ~ SSCCPTR-PMP-06
 */

/* --- SSCCPTR-PMP-01: PMP blocks L0 PT -> load access fault --- */
TEST_REGISTER(test_ssccptr_pmp_block_l0_load);
bool test_ssccptr_pmp_block_l0_load(void) {
    TEST_BEGIN("SSCCPTR-PMP-01: PMP blocks L0 PT read -> load access fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    uintptr_t pt_page_pa = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("L0 PT page found", pt_page_pa != 0);

    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(pt_page_pa, PAGE_SIZE_4K, PMP_X);
    pmp_set_entry(0, &e0);
    pmp_entry_t e15 = PMP_ENTRY_NAPOT(0,
        (uintptr_t)1 << (__riscv_xlen - 1), PMP_RWX);
    pmp_set_entry(15, &e15);

    vm_enable(&ctx, 0);
    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_load8(test_va));
    goto_priv(PRIV_M);
    CHECK_TRAP("load: PTE read blocked", CAUSE_LAF);

    vm_disable();
    pmp_clear_all();
    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-PMP-02: PMP blocks L0 PT -> store access fault --- */
TEST_REGISTER(test_ssccptr_pmp_block_l0_store);
bool test_ssccptr_pmp_block_l0_store(void) {
    TEST_BEGIN("SSCCPTR-PMP-02: PMP blocks L0 PT read -> store access fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D, PT_LEVEL_4K);

    uintptr_t pt_page_pa = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("L0 PT page found", pt_page_pa != 0);

    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(pt_page_pa, PAGE_SIZE_4K, PMP_X);
    pmp_set_entry(0, &e0);
    pmp_entry_t e15 = PMP_ENTRY_NAPOT(0,
        (uintptr_t)1 << (__riscv_xlen - 1), PMP_RWX);
    pmp_set_entry(15, &e15);

    vm_enable(&ctx, 0);
    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_store8(test_va, 0x42));
    goto_priv(PRIV_M);
    CHECK_TRAP("store: PTE read blocked", CAUSE_SAF);

    vm_disable();
    pmp_clear_all();
    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-PMP-03: PMP blocks L0 PT -> fetch access fault --- */
TEST_REGISTER(test_ssccptr_pmp_block_l0_fetch);
bool test_ssccptr_pmp_block_l0_fetch(void) {
    TEST_BEGIN("SSCCPTR-PMP-03: PMP blocks L0 PT read -> fetch access fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);
    init_exec_page();

    uintptr_t test_va = (uintptr_t)test_exec_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_X | PTE_A | PTE_D, PT_LEVEL_4K);

    uintptr_t pt_page_pa = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("L0 PT page found", pt_page_pa != 0);

    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(pt_page_pa, PAGE_SIZE_4K, PMP_X);
    pmp_set_entry(0, &e0);
    pmp_entry_t e15 = PMP_ENTRY_NAPOT(0,
        (uintptr_t)1 << (__riscv_xlen - 1), PMP_RWX);
    pmp_set_entry(15, &e15);

    vm_enable(&ctx, 0);
    goto_priv(PRIV_S);
    PRIV_DO_TRAP(exec_at(test_va));
    goto_priv(PRIV_M);
    CHECK_TRAP("fetch: PTE read blocked", CAUSE_IAF);

    vm_disable();
    pmp_clear_all();
    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-PMP-04: PMP blocks L1 PT -> load access fault --- */
TEST_REGISTER(test_ssccptr_pmp_block_l1_load);
bool test_ssccptr_pmp_block_l1_load(void) {
    TEST_BEGIN("SSCCPTR-PMP-04: PMP blocks L1 PT read -> load access fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

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
    CHECK_TRAP("load: L1 PTE read blocked", CAUSE_LAF);

    vm_disable();
    pmp_clear_all();
    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-PMP-05: PMP blocks root PT -> load access fault --- */
TEST_REGISTER(test_ssccptr_pmp_block_root_load);
bool test_ssccptr_pmp_block_root_load(void) {
    TEST_BEGIN("SSCCPTR-PMP-05: PMP blocks root PT read -> load access fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    uintptr_t pt_page_pa = (uintptr_t)ctx.root_pt;

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
    CHECK_TRAP("load: root PTE read blocked", CAUSE_LAF);

    vm_disable();
    pmp_clear_all();
    pt_pool_reset();
    TEST_END();
}

/* --- SSCCPTR-PMP-06: PMP remove restores page walk --- */
TEST_REGISTER(test_ssccptr_pmp_restore);
bool test_ssccptr_pmp_restore(void) {
    TEST_BEGIN("SSCCPTR-PMP-06: PMP remove restores page walk");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D, PT_LEVEL_4K);

    uintptr_t pt_page_pa = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("L0 PT page found", pt_page_pa != 0);

    /* Phase 1: PMP blocks L0 PT read -> expect access fault */
    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(pt_page_pa, PAGE_SIZE_4K, PMP_X);
    pmp_set_entry(0, &e0);
    pmp_entry_t e15 = PMP_ENTRY_NAPOT(0,
        (uintptr_t)1 << (__riscv_xlen - 1), PMP_RWX);
    pmp_set_entry(15, &e15);

    vm_enable(&ctx, 0);
    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_load8(test_va));
    goto_priv(PRIV_M);
    CHECK_TRAP("blocked: PTE read fails", CAUSE_LAF);
    vm_disable();

    /* Phase 2: Remove PMP restriction -> expect walk succeeds */
    pmp_clear_all();

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("restored: page walk succeeds after PMP removal",
                   result, 0);

    pt_pool_reset();
    TEST_END();
}
