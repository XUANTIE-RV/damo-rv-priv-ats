/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 6: Zicfiss Exception Behavior Tests
 *
 * Tests that Shadow Stack Fault (software-check exception with mtval=3)
 * is correctly triggered on SSPOPCHK mismatch, and verifies exception
 * delegation and pte.xwr=010 encoding behavior.
 *
 * Shadow stack instructions require VM with pte.xwr=010 (shadow stack)
 * pages. Without VM (satp=Bare), they raise store/AMO access-fault
 * per norm:satp_mode_bare.
 */

/* ===================================================================
 * S-mode test functions for VM-based shadow stack exception tests.
 * =================================================================== */

/* S-mode function: SSPOPCHK mismatch triggers software-check exception */
static uintptr_t smode_sspopchk_mismatch(uintptr_t arg)
{
    (void)arg;

    /* Set ra to a known value and SSPUSH it onto the shadow stack */
    asm volatile("li ra, 0x1111111111111111" ::: "memory");

    /* SSPUSH x1: pushes ra (0x1111...) onto shadow stack */
    asm volatile(
        ".word 0xCE104073\n"    /* SSPUSH x1 */
        ::: "ra", "memory"
    );

    /* Now modify ra to a DIFFERENT value */
    asm volatile("li ra, 0x2222222222222222" ::: "memory");

    /* Arm trap: SSPOPCHK will compare mem[SSP] (0x1111...) with ra (0x2222...).
     * Mismatch must trigger software-check exception (cause=18, tval=3). */
    trap_expect_begin();

    asm volatile(
        ".word 0xCDC0C073\n"    /* SSPOPCHK x1 */
        ::: "ra", "memory"
    );

    trap_expect_end();

    if (trap_was_triggered())
    {
        uintptr_t cause = trap_get_cause();
        uintptr_t tval = trap_get_tval();
        printf("    SSPOPCHK mismatch: cause=%lu, tval=%lu\n",
               (unsigned long)cause, (unsigned long)tval);
        /* Encode result: cause in low bits, tval in high bits */
        return (tval << 16) | (cause & 0xFFFF);
    }
    return 0;  /* no trap (unexpected) */
}

/* S-mode function: access shadow stack page (load + SSPUSH) */
static uintptr_t smode_access_ss_page_ok(uintptr_t arg)
{
    volatile uintptr_t *ss_ptr = (volatile uintptr_t *)arg;

    /* Per SPEC: "shadow stack is readable by all instructions that
     * only load from memory." So a load should succeed. */
    trap_expect_begin();
    volatile uintptr_t val = *ss_ptr;
    (void)val;
    trap_expect_end();

    if (trap_was_triggered())
        return trap_get_cause();  /* unexpected fault on load */

    /* SSPUSH should also succeed on a shadow stack page with SSE=1 */
    /* Set SSP to point to this page */
    asm volatile("csrw 0x011, %0" :: "r"(arg + 0x100) : "memory");
    asm volatile("li ra, 0xCAFEBABE" ::: "ra");
    trap_expect_begin();
    asm volatile(".word 0xCE104073\n" ::: "ra", "memory");  /* SSPUSH x1 */
    trap_expect_end();

    if (trap_was_triggered())
        return trap_get_cause();

    return 0;  /* success */
}

/* S-mode function: access shadow stack page when SSE=0 (should fault) */
static uintptr_t smode_access_ss_page_sse0(uintptr_t arg)
{
    volatile uintptr_t *ss_ptr = (volatile uintptr_t *)arg;

    /* With SSE=0, pte.xwr=010 is a reserved encoding.
     * Any access (load or store) should trigger a page fault. */
    trap_expect_begin();
    volatile uintptr_t val = *ss_ptr;
    (void)val;
    trap_expect_end();

    if (trap_was_triggered())
        return trap_get_cause();

    return 0;  /* no fault (unexpected) */
}

/* CFI-SS-EXC-01: SSPOPCHK mismatch triggers Shadow Stack Fault (S-mode) */
TEST_REGISTER(test_zicfiss_trap_popchk_mismatch);
bool test_zicfiss_trap_popchk_mismatch(void)
{
    TEST_BEGIN("CFI-SS-EXC-01: SSPOPCHK mismatch triggers SS Fault (S-mode)");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /*
     * Per SPEC (norm:sspopchk_shadow_return_pop):
     * "The SSPOPCHK instruction can be used to pop the shadow return
     *  address value from the shadow stack and check that the value
     *  matches the contents of the link register, and if not cause a
     *  software-check exception with xtval set to shadow stack fault
     *  (code=3)."
     *
     * Zicfiss has no M-mode SSE control. menvcfg.SSE only activates
     * Zicfiss in S-mode. In M-mode, SSPOPCHK reverts to Zimop (no-op).
     * Therefore we MUST test SSPOPCHK mismatch in S-mode with VM
     * and shadow stack pages (pte.xwr=010).
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

    /* Set SSP to point to shadow stack test page */
    uintptr_t ssp_val = ss_vm_test_page + 0x800;
    asm volatile("csrw 0x011, %0" :: "r"(ssp_val) : "memory");

    /* Run SSPOPCHK mismatch test in S-mode with VM */
    uintptr_t result = vm_run_in_smode(&ctx, smode_sspopchk_mismatch, 0);

    /* Clean up */
    asm volatile("csrw 0x011, zero" ::: "memory");
    menvcfg_clear(MENVCFG_SSE);
    pt_pool_reset();

    /* Parse result */
    uintptr_t cause = result & 0xFFFF;
    uintptr_t tval = (result >> 16) & 0xFFFF;

    TEST_ASSERT("SSPOPCHK mismatch triggered an exception",
                result != 0);

    /* Per SPEC: must be software-check exception with tval=3.
     * Do NOT accept cause=7 as a "limitation" — that would be
     * lowering the standard to match a non-compliant implementation. */
    TEST_ASSERT("cause is CAUSE_SOFTWARE_CHECK (18) per SPEC",
                cause == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("tval is SWCHECK_SHADOW_STACK_FAULT (3) per SPEC",
                tval == SWCHECK_SHADOW_STACK_FAULT);

    TEST_END();
}

/* CFI-SS-EXC-02: Shadow Stack Fault exception delegation */
TEST_REGISTER(test_zicfiss_trap_delegation);
bool test_zicfiss_trap_delegation(void)
{
    TEST_BEGIN("CFI-SS-EXC-02: software-check exception delegatable via medeleg[18]");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /*
     * Verify that medeleg bit 18 (software-check exception) is writable,
     * meaning the exception can be delegated to S-mode.
     *
     * Shadow Stack Fault (cause=18, tval=3) uses the same exception
     * code as Landing Pad Fault, so delegation is shared.
     */
    uintptr_t orig_medeleg;
    asm volatile("csrr %0, 0x302" : "=r"(orig_medeleg));

    uintptr_t bit18 = (1UL << 18);
    asm volatile("csrs 0x302, %0" :: "r"(bit18) : "memory");

    uintptr_t new_medeleg;
    asm volatile("csrr %0, 0x302" : "=r"(new_medeleg));

    bool delegatable = (new_medeleg & bit18) != 0;

    /* Restore */
    asm volatile("csrw 0x302, %0" :: "r"(orig_medeleg) : "memory");

    TEST_ASSERT("medeleg[18] writable (software-check delegatable)",
                delegatable);

    TEST_END();
}

/* CFI-SS-EXC-03: pte.xwr=010 valid as shadow stack page when SSE=1 */
TEST_REGISTER(test_zicfiss_trap_pte_xwr010_sse1);
bool test_zicfiss_trap_pte_xwr010_sse1(void)
{
    TEST_BEGIN("CFI-SS-EXC-03: pte.xwr=010 is shadow stack page (SSE=1)");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /*
     * Per SPEC norm:ss_page_enc:
     * "The encoding R=0, W=1, and X=0, is defined to represent an SS page."
     * "When menvcfg.SSE=0, this encoding remains reserved."
     *
     * With SSE=1, pte.xwr=010 is a valid shadow stack page encoding.
     * Loads from the page should succeed (shadow stack is readable).
     * SSPUSH should also succeed on a shadow stack page.
     */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test page as shadow stack page (xwr=010) */
    uintptr_t ss_flags = PTE_SS | PTE_A | PTE_D;
    pt_map_page(&ctx, ss_vm_test_page, ss_vm_test_page,
                ss_flags, PT_LEVEL_4K);

    /* Enable SSE */
    menvcfg_set(MENVCFG_SSE);

    /* Run access test in S-mode with VM */
    uintptr_t result = vm_run_in_smode(&ctx, smode_access_ss_page_ok,
                                        ss_vm_test_page);

    /* Clean up */
    asm volatile("csrw 0x011, zero" ::: "memory");
    menvcfg_clear(MENVCFG_SSE);
    pt_pool_reset();

    TEST_ASSERT("shadow stack page accessible with SSE=1 (no fault)",
                result == 0);

    if (result != 0)
        printf("    [FAIL] Unexpected fault cause=%lu when accessing SS page\n",
               (unsigned long)result);

    TEST_END();
}

/* CFI-SS-EXC-04: pte.xwr=010 reserved when SSE=0 (triggers page fault) */
TEST_REGISTER(test_zicfiss_trap_pte_xwr010_sse0);
bool test_zicfiss_trap_pte_xwr010_sse0(void)
{
    TEST_BEGIN("CFI-SS-EXC-04: pte.xwr=010 reserved when SSE=0");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /*
     * Per norm:menvcfg_sse_op_list (SPEC/machine.adoc:2298):
     * "The pte.xwr=0b010 encoding in VS/S-stage page tables becomes
     *  reserved."
     *
     * When SSE=0, accessing a page with pte.xwr=010 should trigger
     * a page fault (since the encoding is reserved/invalid).
     */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test page with xwr=010 encoding (would be SS page if SSE=1) */
    uintptr_t ss_flags = PTE_SS | PTE_A | PTE_D;
    pt_map_page(&ctx, ss_vm_test_page, ss_vm_test_page,
                ss_flags, PT_LEVEL_4K);

    /* Ensure SSE=0 */
    menvcfg_clear(MENVCFG_SSE);

    /* Run access test in S-mode with VM */
    uintptr_t result = vm_run_in_smode(&ctx, smode_access_ss_page_sse0,
                                        ss_vm_test_page);

    /* Clean up */
    pt_pool_reset();

    TEST_ASSERT("access to xwr=010 page triggered exception with SSE=0",
                result != 0);

    /* The fault should be a page fault (cause 13 for load, 15 for store) */
    if (result != 0)
    {
        printf("    Fault cause = %lu (expected page fault: 13 or 15)\n",
               (unsigned long)result);
        TEST_ASSERT("cause is a page fault (13=load, 15=store)",
                    result == CAUSE_LOAD_PAGE_FAULT ||
                    result == CAUSE_STORE_PAGE_FAULT);
    }

    TEST_END();
}
