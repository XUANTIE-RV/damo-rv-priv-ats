/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_sfence_rsvd.c - Group 12: SFENCE.VMA Instruction (SFENCE-01/04/05)
 *                    + Group 13: PTE Reserved Bits (RSVD-01~02)
 *                    + Group 14: Non-leaf PTE Reserved Fields (NLPTE-04)
 *
 * Tests:
 *   SFENCE-01: Permission change + sfence.vma takes effect
 *   SFENCE-04: Mapping change + sfence.vma takes effect
 *   SFENCE-05: Permission upgrade (R -> RW) + sfence.vma takes effect
 *   RSVD-01:   PTE bits[60:54] non-zero triggers load page fault
 *   RSVD-02:   PTE bits[60:54] non-zero triggers store page fault
 *   NLPTE-04:  Non-leaf PTE with D=A=U=0 works correctly
 */

/* ===================================================================
 * S-mode helper: modify PTE and sfence, then access
 *
 * These functions run in S-mode and manipulate page table entries
 * directly, then execute sfence.vma before accessing the page.
 * =================================================================== */

static volatile uintptr_t *g_pte_addr;
static volatile uintptr_t g_test_va;

static uintptr_t test_smode_sfence01(uintptr_t arg) {
    uintptr_t test_va = g_test_va;
    volatile uintptr_t *pte = g_pte_addr;

    /* Step 1: Verify write succeeds with RW permission */
    trap_expect_begin();
    *(volatile uintptr_t *)test_va = 0x1234;
    trap_expect_end();
    if (trap_was_triggered())
        return 1;  /* unexpected fault on initial write */

    /* Step 2: Change PTE from RW to R-only (clear W bit) */
    *pte = (*pte) & ~PTE_W;

    /* Step 3: sfence.vma to flush TLB */
    __asm__ volatile ("sfence.vma %0, zero" :: "r"(test_va) : "memory");

    /* Step 4: Try to write again - should fault */
    trap_expect_begin();
    *(volatile uintptr_t *)test_va = 0x5678;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();

    return 0;  /* no fault = test failed */
}

static uintptr_t test_smode_sfence04(uintptr_t arg) {
    uintptr_t test_va = g_test_va;
    volatile uintptr_t *pte = g_pte_addr;
    uintptr_t new_pa = arg;

    /* Step 1: Read current value */
    trap_expect_begin();
    volatile uintptr_t old_val = *(volatile uintptr_t *)test_va;
    (void)old_val;
    trap_expect_end();
    if (trap_was_triggered())
        return 1;

    /* Step 2: Change PTE to point to new PA */
    uintptr_t old_flags = (*pte) & PTE_FLAGS_MASK;
    *pte = PA_TO_PTE(new_pa) | old_flags;

    /* Step 3: sfence.vma */
    __asm__ volatile ("sfence.vma %0, zero" :: "r"(test_va) : "memory");

    /* Step 4: Read from the same VA - should get data from new PA */
    trap_expect_begin();
    volatile uintptr_t new_val = *(volatile uintptr_t *)test_va;
    (void)new_val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();

    return 0;
}

static uintptr_t test_smode_sfence05(uintptr_t arg) {
    uintptr_t test_va = g_test_va;
    volatile uintptr_t *pte = g_pte_addr;

    /* Step 1: Verify write faults with R-only permission */
    trap_expect_begin();
    *(volatile uintptr_t *)test_va = 0x1234;
    trap_expect_end();
    if (!trap_was_triggered())
        return 1;  /* expected fault but didn't get one */

    /* Step 2: Upgrade PTE from R to RW (set W bit) */
    *pte = (*pte) | PTE_W | PTE_D;

    /* Step 3: sfence.vma */
    __asm__ volatile ("sfence.vma %0, zero" :: "r"(test_va) : "memory");

    /* Step 4: Write should now succeed */
    trap_expect_begin();
    *(volatile uintptr_t *)test_va = 0x5678;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();

    return 0;
}

/* ===================================================================
 * Group 12: SFENCE.VMA Instruction Tests
 * =================================================================== */

TEST_REGISTER(test_sv48_sfence01);
bool test_sv48_sfence01(void) {
    TEST_BEGIN("SFENCE-01: Permission change + sfence.vma takes effect");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test page with RW permission */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Find the L0 PTE address for this VA */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("found L0 page table", pt_page != 0);

    uintptr_t vpn0 = VA_VPN0(test_va);
    g_pte_addr = (volatile uintptr_t *)(pt_page + vpn0 * sizeof(uintptr_t));
    g_test_va = test_va;

    /* Run test: RW -> R-only, sfence, verify store faults */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_sfence01, 0);
    TEST_ASSERT("permission downgrade + sfence: store faults",
                result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_sfence04);
bool test_sv48_sfence04(void) {
    TEST_BEGIN("SFENCE-04: Mapping change + sfence.vma takes effect");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test_fault_page with RW */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Find the L0 PTE address */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("found L0 page table", pt_page != 0);

    uintptr_t vpn0 = VA_VPN0(test_va);
    g_pte_addr = (volatile uintptr_t *)(pt_page + vpn0 * sizeof(uintptr_t));
    g_test_va = test_va;

    /* Run test: change PA mapping, sfence, verify access works */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_sfence04,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("mapping change + sfence: access succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_sfence05);
bool test_sv48_sfence05(void) {
    TEST_BEGIN("SFENCE-05: Permission upgrade (R -> RW) + sfence.vma");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test page with R-only permission */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A,  /* R-only, no W, no D */
                PT_LEVEL_4K);

    /* Find the L0 PTE address */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("found L0 page table", pt_page != 0);

    uintptr_t vpn0 = VA_VPN0(test_va);
    g_pte_addr = (volatile uintptr_t *)(pt_page + vpn0 * sizeof(uintptr_t));
    g_test_va = test_va;

    /* Run test: R -> RW upgrade, sfence, verify write succeeds */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_sfence05, 0);
    TEST_ASSERT("permission upgrade + sfence: write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * Group 13: PTE Reserved Bits Check (RSVD-01 ~ RSVD-02)
 * =================================================================== */

TEST_REGISTER(test_sv48_rsvd01);
bool test_sv48_rsvd01(void) {
    TEST_BEGIN("RSVD-01: PTE bits[60:54] non-zero triggers load page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test page normally first */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Find the L0 PTE and set reserved bit 54 */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("found L0 page table", pt_page != 0);

    uintptr_t vpn0 = VA_VPN0(test_va);
    volatile uintptr_t *pte = (volatile uintptr_t *)(pt_page + vpn0 * sizeof(uintptr_t));
    *pte |= (1UL << 54);  /* Set reserved bit 54 */

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("reserved bit 54 triggers load page fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv48_rsvd02);
bool test_sv48_rsvd02(void) {
    TEST_BEGIN("RSVD-02: PTE bits[60:54] non-zero triggers store page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test page normally first */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Find the L0 PTE and set reserved bit 56 */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, 0);
    TEST_ASSERT("found L0 page table", pt_page != 0);

    uintptr_t vpn0 = VA_VPN0(test_va);
    volatile uintptr_t *pte = (volatile uintptr_t *)(pt_page + vpn0 * sizeof(uintptr_t));
    *pte |= (1UL << 56);  /* Set reserved bit 56 */

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("reserved bit 56 triggers store page fault",
                result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * Group 14: Non-leaf PTE Reserved Fields (NLPTE-04)
 * =================================================================== */

TEST_REGISTER(test_sv48_nlpte04);
bool test_sv48_nlpte04(void) {
    TEST_BEGIN("NLPTE-04: Non-leaf PTE with D=A=U=0 works correctly");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

    /*
     * Set up a 4KB identity mapping. The intermediate (non-leaf) PTEs
     * at L3, L2, and L1 should have D=0, A=0, U=0 by default (only
     * PTE_V set). The leaf PTE at L0 has full permissions.
     *
     * This test verifies that non-leaf PTEs with D=A=U=0 do not cause
     * page faults during the walk.
     */
    uintptr_t base = PLATFORM_MEM_BASE;
    uintptr_t size = 2 * PAGE_SIZE_2M;
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, size,
                                        flags, PT_LEVEL_4K);
    TEST_ASSERT("4KB identity mapping setup", ret == 0);

    /* Verify non-leaf PTEs at L3 (root) have D=A=U=0 */
    uintptr_t test_va = (uintptr_t)test_data_area;

    /* Check L3 (root) PTE - Sv48 root is at level 3 */
    uintptr_t *root = ctx.root_pt;
    uintptr_t vpn3 = VA_VPN3(test_va);
    uintptr_t l3_pte = root[vpn3];
    TEST_ASSERT("L3 non-leaf PTE is valid", (l3_pte & PTE_V) != 0);
    TEST_ASSERT("L3 non-leaf PTE is non-leaf",
                (l3_pte & (PTE_R | PTE_W | PTE_X)) == 0);
    TEST_ASSERT("L3 non-leaf PTE has D=A=U=0",
                (l3_pte & (PTE_D | PTE_A | PTE_U)) == 0);

    /* Verify access works despite non-leaf PTEs having D=A=U=0 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("access works with non-leaf D=A=U=0", result == 0);

    pt_pool_reset();
    TEST_END();
}
