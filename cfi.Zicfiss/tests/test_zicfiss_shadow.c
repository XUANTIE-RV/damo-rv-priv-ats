/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 5: Zicfiss Shadow Stack Functional Tests
 *
 * Tests the behavior of shadow stack instructions (SSPUSH, SSPOPCHK,
 * SSAMOSWAP) and the SSE enable/disable semantics.
 *
 * Shadow stack instructions require VM with pte.xwr=010 (shadow stack)
 * pages to function correctly. Without VM (satp=Bare), shadow stack
 * instructions raise a store/AMO access-fault exception per
 * norm:satp_mode_bare (SPEC/priv/cfi.adoc).
 */

/* ===================================================================
 * S-mode test functions for VM-based shadow stack tests.
 *
 * These functions execute in S-mode with VM enabled (satp=Sv39).
 * They use trap_expect_begin/end to detect faults.
 * =================================================================== */

/* S-mode function: SSPUSH/SSPOPCHK matching pair */
static uintptr_t smode_sspush_sspopchk_match(uintptr_t arg)
{
    (void)arg;

    /* Read SSP before operations */
    uintptr_t ssp_before;
    asm volatile("csrr %0, 0x011" : "=r"(ssp_before));

    /* Set ra to a known value */
    asm volatile("li ra, 0x12345678ABCDEF00" ::: "memory");

    /* SSPUSH x1: pushes ra onto shadow stack (SSP -= 8, store ra) */
    asm volatile(
        ".word 0xCE104073\n"    /* SSPUSH x1 */
        ::: "ra", "memory"
    );

    /* Read SSP after push */
    uintptr_t ssp_after_push;
    asm volatile("csrr %0, 0x011" : "=r"(ssp_after_push));

    /* SSPOPCHK x1: loads from SSP, compares with ra, SSP += 8 */
    asm volatile(
        ".word 0xCDC0C073\n"    /* SSPOPCHK x1 */
        ::: "ra", "memory"
    );

    /* Read SSP after pop */
    uintptr_t ssp_after_pop;
    asm volatile("csrr %0, 0x011" : "=r"(ssp_after_pop));

    printf("    SSP: before=0x%lx after_push=0x%lx after_pop=0x%lx\n",
           (unsigned long)ssp_before,
           (unsigned long)ssp_after_push,
           (unsigned long)ssp_after_pop);

    /* Verify SSP was decremented by push */
    if (ssp_after_push >= ssp_before)
        return 1;

    /* Verify SSP was restored after pop (back to original or close) */
    if (ssp_after_pop != ssp_before)
        return 2;

    return 0;  /* success */
}

/* S-mode function: normal store to shadow stack page (should fault) */
static uintptr_t smode_store_to_ss_page(uintptr_t arg)
{
    volatile uintptr_t *ss_ptr = (volatile uintptr_t *)arg;

    trap_expect_begin();

    /* Attempt a normal store to the shadow stack page.
     * Per norm:ssmp_ss_page_access_fault: "Memory mapped as an SS page
     * cannot be written to by instructions other than SSAMOSWAP.W/D,
     * SSPUSH, and C.SSPUSH. Attempts will raise a store/AMO access-fault
     * exception." */
    *ss_ptr = 0xDEADBEEF;

    trap_expect_end();

    if (trap_was_triggered())
        return trap_get_cause();  /* return the cause */
    return 0;  /* no fault (unexpected) */
}

/* CFI-SS-FN-01: SSPUSH/SSPOPCHK basic flow */
TEST_REGISTER(test_zicfiss_shadow_push_pop_match);
bool test_zicfiss_shadow_push_pop_match(void)
{
    TEST_BEGIN("CFI-SS-FN-01: SSPUSH/SSPOPCHK matching pair succeeds");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /*
     * With SSE=1 and VM enabled with shadow stack pages (pte.xwr=010),
     * SSPUSH pushes the return address onto the shadow stack, and
     * SSPOPCHK verifies the top-of-stack matches the expected value.
     * A matching pair should not trigger any exception.
     */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map shadow stack test page as shadow stack page (xwr=010) */
    uintptr_t ss_flags = PTE_SS | PTE_A | PTE_D;
    pt_map_page(&ctx, ss_vm_test_page, ss_vm_test_page,
                ss_flags, PT_LEVEL_4K);

    /* Enable SSE */
    menvcfg_set(MENVCFG_SSE);

    /* Set SSP to point to middle of shadow stack test page */
    uintptr_t ssp_val = ss_vm_test_page + 0x800;
    asm volatile("csrw 0x011, %0" :: "r"(ssp_val) : "memory");

    /* Run SSPUSH/SSPOPCHK test in S-mode with VM */
    uintptr_t result = vm_run_in_smode(&ctx, smode_sspush_sspopchk_match, 0);

    /* Clean up */
    asm volatile("csrw 0x011, zero" ::: "memory");
    menvcfg_clear(MENVCFG_SSE);
    pt_pool_reset();

    TEST_ASSERT("SSPUSH/SSPOPCHK completed without exception", result == 0);

    if (result != 0)
        printf("    [FAIL] smode_sspush_sspopchk_match returned %lu\n",
               (unsigned long)result);

    TEST_END();
}

/* CFI-SS-FN-02: Shadow stack page write protection (pte.xwr=010) */
TEST_REGISTER(test_zicfiss_shadow_write_protect);
bool test_zicfiss_shadow_write_protect(void)
{
    TEST_BEGIN("CFI-SS-FN-02: Shadow stack page write protection");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /*
     * Per SPEC norm:ssmp_ss_page_access_fault:
     * "Memory mapped as an SS page cannot be written to by instructions
     *  other than SSAMOSWAP.W/D, SSPUSH, and C.SSPUSH. Attempts will
     *  raise a store/AMO access-fault exception."
     *
     * This test sets up a shadow stack page (pte.xwr=010) with SSE=1,
     * then attempts a normal store to it from S-mode. The store must
     * trigger a store/AMO access-fault (cause=7).
     */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map shadow stack test page as shadow stack page */
    uintptr_t ss_flags = PTE_SS | PTE_A | PTE_D;
    pt_map_page(&ctx, ss_vm_test_page, ss_vm_test_page,
                ss_flags, PT_LEVEL_4K);

    /* Enable SSE */
    menvcfg_set(MENVCFG_SSE);

    /* Run store-to-SS-page test in S-mode with VM */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store_to_ss_page,
                                        ss_vm_test_page);

    /* Clean up */
    menvcfg_clear(MENVCFG_SSE);
    pt_pool_reset();

    /* Per SPEC: normal store to SS page must trigger store/AMO access-fault */
    TEST_ASSERT("normal store to SS page triggered exception",
                result != 0);
    TEST_ASSERT("cause is CAUSE_STORE_ACCESS_FAULT (7) per SPEC",
                result == CAUSE_STORE_ACCESS_FAULT);

    TEST_END();
}

/* CFI-SS-FN-03: SSE=0 makes 32-bit Zicfiss instructions revert to Zimop */
TEST_REGISTER(test_zicfiss_shadow_sse0_zimop_revert);
bool test_zicfiss_shadow_sse0_zimop_revert(void)
{
    TEST_BEGIN("CFI-SS-FN-03: 32-bit Zicfiss insns revert to Zimop when SSE=0");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /*
     * Per norm:menvcfg_sse_op_list:
     * "32-bit Zicfiss instructions will revert to their behavior as
     *  defined by Zimop."
     *
     * Zimop instructions are defined as no-ops (they do nothing).
     * So with SSE=0, executing a 32-bit Zicfiss instruction should
     * behave as a no-op (no exception, no side effects).
     */
    menvcfg_clear(MENVCFG_SSE);

    /* Execute a 32-bit Zicfiss instruction encoding with SSE=0.
     * It should act as Zimop (no-op). */
    trap_expect_begin();
    /* SSPUSH x1 encoding: MOP.RR.7 with rs2=x1
     * funct7=1100111 | rs2=00001 | rs1=00000 | funct3=100 | rd=00000 | opcode=1110011 */
    asm volatile(
        ".word 0xCE104073\n"  /* SSPUSH x1 (MOP.RR.7) */
        ::: "memory"
    );
    trap_expect_end();

    /* SPEC mandates no-op (Zimop) behavior; exception is non-compliant */
    TEST_ASSERT("32-bit Zicfiss reverts to Zimop no-op (no exception)",
                !trap_was_triggered());

    TEST_END();
}

/* CFI-SS-FN-04: SSE=0 makes 16-bit Zicfiss instructions revert to Zcmop */
TEST_REGISTER(test_zicfiss_shadow_sse0_zcmop_revert);
bool test_zicfiss_shadow_sse0_zcmop_revert(void)
{
    TEST_BEGIN("CFI-SS-FN-04: 16-bit Zicfiss insns revert to Zcmop when SSE=0");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /*
     * Per norm:menvcfg_sse_op_list:
     * "16-bit Zicfiss instructions will revert to their behavior as
     *  defined by Zcmop."
     *
     * Zcmop instructions are compressed no-ops.
     */
    menvcfg_clear(MENVCFG_SSE);

    trap_expect_begin();
    /* C.SSPUSH x1 encoding: C.MOP.1 (Zcmop-based)
     * [15:13]=011, [12]=0, [11]=0, [10:8]=000, [7]=1, [6:2]=00000, [1:0]=01 */
    asm volatile(
        ".hword 0x6081\n"  /* C.SSPUSH x1 (C.MOP.1) */
        ::: "memory"
    );
    trap_expect_end();

    if (trap_was_triggered())
    {
        printf("    16-bit instruction raised exception (cause=%lu) - "
               "Zcmop may not be implemented\n",
               (unsigned long)trap_get_cause());
        TEST_ASSERT("exception is illegal-instruction (Zcmop not impl)",
                    trap_get_cause() == CAUSE_ILLEGAL_INST);
    }
    else
    {
        printf("    16-bit instruction executed as no-op (Zcmop behavior)\n");
        TEST_ASSERT("no exception - Zcmop no-op behavior confirmed", true);
    }

    TEST_END();
}
