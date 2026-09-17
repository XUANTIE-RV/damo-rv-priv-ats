/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.9 - Fetch-class exceptions do not write a transformed htinst
 *             (HZCA-28 ~ HZCA-29)
 *
 * Spec basis:
 *   norm:H_trap_xtinst_val (tinst-values table) - for an instruction
 *     guest-page fault (cause=20), Transformed = No: only zero or a
 *     pseudoinstruction may be written. This is the key contrast with a
 *     load/store guest-page fault (cause=21/23), which MAY write a
 *     compressed transformed value.
 *
 * Distinguishing rule: a transformed value always has bit0 = 1 (see
 * norm:H_trap_xtinst_exception_list); zero and the pseudoinstructions
 * (0x00003000 / 0x00003020) have bit0 = 0. So "never transformed" is
 * asserted robustly as (htinst & 1) == 0.
 *
 * The G-stage target instruction page is mapped readable+writable but
 * NOT executable, so a VS-mode fetch raises an instruction guest-page
 * fault regardless of the (unconsumed) instruction bytes at the target.
 * =================================================================== */

/* ------------------------------------------------------------------
 * HZCA-28: guest compressed fetch instruction guest-page-fault ->
 *          htinst = 0 or a pseudoinstruction, NEVER a transformed value.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_28_fetch_gpf_no_transformed);
bool test_hzca_28_fetch_gpf_no_transformed(void)
{
    TEST_BEGIN("HZCA-28: compressed fetch guest-page-fault htinst not transformed");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t target = (uintptr_t)test_exec_page;
    /* Place a compressed instruction at the target for faithfulness; the
     * fetch faults on the missing X permission before decode, so these
     * bytes are never architecturally consumed. */
    *(volatile uint16_t *)target = 0x8082UL;   /* c.jr ra */
    asm volatile ("fence.i" ::: "memory");

    two_stage_ctx_t ctx;
    /* G-stage: readable+writable, NOT executable -> fetch guest-page
     * fault (cause=20). VS-stage keeps X=1 so the fault is a G-stage
     * (guest-page) fault, not a VS-stage instruction page fault. */
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, target, HZ_G_WNXU);
    hz_route_to_hs(1UL << CAUSE_INST_GUEST_PAGE_FAULT);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_exec_expect_fault, target);
    bool fired = trap_was_triggered();
    uintptr_t cause  = fired ? trap_get_cause()  : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_INST_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    TEST_ASSERT("instruction guest-page-fault fired", fired);
    TEST_ASSERT_EQ("cause == instruction guest-page-fault (20)",
                   cause, (uintptr_t)CAUSE_INST_GUEST_PAGE_FAULT);

    printf("  [INFO] fetch-class htinst=0x%lx\n", (unsigned long)htinst);
    TEST_ASSERT("fetch-class htinst never transformed (bit0 == 0)",
                (htinst & 1UL) == 0UL);
    TEST_ASSERT("fetch-class htinst == 0 or a pseudoinstruction",
                htinst == 0 ||
                htinst == (uintptr_t)HTINST_PSEUDO_READ_RV64 ||
                htinst == (uintptr_t)HTINST_PSEUDO_WRITE_RV64);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-29: contrast/disambiguation of fetch-class (cause=20) vs
 *          memory-access-class (cause=21) htinst writable value space.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_29_fetch_vs_mem_contrast);
bool test_hzca_29_fetch_vs_mem_contrast(void)
{
    TEST_BEGIN("HZCA-29: fetch-class vs memory-access-class htinst contrast");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* (a) compressed FETCH fault (cause=20) on a no-X G-stage page. */
    uintptr_t exec_target = (uintptr_t)test_exec_page;
    *(volatile uint16_t *)exec_target = 0x8082UL;   /* c.jr ra */
    asm volatile ("fence.i" ::: "memory");
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, exec_target, HZ_G_WNXU);
    hz_route_to_hs(1UL << CAUSE_INST_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_exec_expect_fault, exec_target);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a  = fired_a ? trap_get_cause()  : 0;
    uintptr_t htinst_a = fired_a ? trap_get_htinst() : 0;
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_INST_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    /* (b) compressed LOAD fault (cause=21) on an invalid G-stage page. */
    uintptr_t load_va = (uintptr_t)test_fault_page;
    hzca_trap_t lb = hzca_fire_mem_fault(hz_vs_c_lw, load_va, HZ_G_INV,
                                         CAUSE_LOAD_GUEST_PAGE_FAULT, true);

    TEST_ASSERT("(a) fetch fault fired", fired_a);
    TEST_ASSERT_EQ("(a) cause == instruction guest-page-fault (20)",
                   cause_a, (uintptr_t)CAUSE_INST_GUEST_PAGE_FAULT);
    TEST_ASSERT("(b) load fault fired", lb.fired);
    TEST_ASSERT_EQ("(b) cause == load guest-page-fault (21)",
                   lb.cause, (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);

    /* Fetch class: never a transformed value (bit0 == 0). */
    TEST_ASSERT("(a) fetch htinst never transformed (bit0 == 0)",
                (htinst_a & 1UL) == 0UL);
    /* Memory-access class: zero (legal) or a compressed transformed value
     * carrying the bits[1:0] == 01 marker. */
    if (lb.xtinst != 0)
        TEST_ASSERT_EQ("(b) load htinst bits[1:0] == 01 (transformed)",
                       lb.xtinst & 3UL, 1UL);
    printf("  [INFO] (a) fetch htinst=0x%lx | (b) load htinst=0x%lx\n",
           (unsigned long)htinst_a, (unsigned long)lb.xtinst);

    HYP_TEST_END();
}
