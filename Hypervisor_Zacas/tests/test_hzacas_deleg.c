/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 3.2 - amocas exceptions all land in the store/AMO class and the
 *             unconditional write-permission requirement
 *             (HZACAS-05 ~ HZACAS-10)
 *
 * Spec basis:
 *   norm:mcause_exccode_st_sc_amo - amocas (AMO family) generates
 *     store/AMO exceptions (cause 6/7/15/23), NEVER load class.
 *   norm:Zacas_amocas_w_permission - "An amocas.w/d/q instruction always
 *     requires write permissions." This is UNCONDITIONAL: even a CAS whose
 *     comparison FAILS (and therefore may not logically write memory) must
 *     still pass the write permission check. This is the Zacas-specific
 *     analogue of Group 1's failed-SC permission check (HZLRSC-22~26).
 *   norm:store_page_fault_no_w (+ NOTE) - amocas on an unreadable page
 *     always raises a store page-fault (15), never a load page-fault (13).
 *   norm:hedeleg_acc / norm:hedeleg_op - hedeleg[6/7/15] Writable,
 *     hedeleg[23] read-only-0.
 * =================================================================== */

/* Run an amocas.w against a VS-stage victim and report the trap. @match
 * selects the compare value: 1 -> cmp equals the page's preset (success
 * path), 0 -> cmp mismatches (guaranteed-failed CAS). */
static int hzacas_cas_vs_fault(uintptr_t victim_va, uintptr_t vs_flags,
                               int match, int delegate_to_vs,
                               uintptr_t exp_cause, const char *tag)
{
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, victim_va, vs_flags);
    /* Preset the page through the identity mapping (M-mode). */
    *(volatile uint32_t *)victim_va = 0x00001000u;

    hz_cas_cmp = match ? 0x00001000u : 0x00009999u;
    hz_cas_swap = 0x00002000u;

    int fired, vs_trap = 0;
    uintptr_t cause = 0;
    if (delegate_to_vs) {
        hyp_delegate_to_vs((1UL << CAUSE_STORE_PAGE_FAULT), 0);
        hz_vs_handler_install();
        (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, victim_va);
        fired = g_hz_vs_triggered;
        cause = g_hz_vs_cause;
        vs_trap = 1;
    } else {
        hedeleg_write(hedeleg_read() & ~(1UL << CAUSE_STORE_PAGE_FAULT));
        trap_expect_begin();
        (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, victim_va);
        fired = trap_was_triggered();
        cause = fired ? trap_get_cause() : 0;
        trap_expect_end();
    }
    ts2_finish(&ctx);

    printf("  [INFO] %s: fired=%d cause=%lu (expected %lu)%s\n",
           tag, fired, (unsigned long)cause, (unsigned long)exp_cause,
           vs_trap ? " [deleg->VS]" : "");
    return fired && cause == exp_cause;
}

/* ------------------------------------------------------------------
 * HZACAS-05: hedeleg bit-property probe for the amocas causes.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_05_hedeleg_bits);
bool test_hzacas_05_hedeleg_bits(void)
{
    TEST_BEGIN("HZACAS-05: hedeleg 6/7/15 writable, 23 RO-0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");

    uintptr_t vs_bits = (1UL << 6) | (1UL << 7) | (1UL << 15);
    uintptr_t g_bits  = (1UL << 23);

    uintptr_t saved = hedeleg_read();
    hedeleg_write(vs_bits | g_bits);
    uintptr_t rb = hedeleg_read();

    TEST_ASSERT_BITS("hedeleg VS-stage bits (6/7/15) writable",
                     rb, vs_bits, vs_bits);
    TEST_ASSERT_BITS("hedeleg G-stage bit (23) read-only-0",
                     rb, g_bits, (uintptr_t)0);

    hedeleg_write(saved);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-06: SUCCESS-path amocas to R=1/W=0 VS-stage page -> store
 *            page-fault (15), delegated to VS-mode.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_06_cas_success_no_write);
bool test_hzacas_06_cas_success_no_write(void)
{
    TEST_BEGIN("HZACAS-06: success amocas to W=0 -> store pf (15) to VS");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    int ok = hzacas_cas_vs_fault((uintptr_t)test_fault_page, HZ_VS_R,
                                 /*match*/1, /*deleg*/1,
                                 CAUSE_STORE_PAGE_FAULT,
                                 "success amocas to W=0");
    TEST_ASSERT("success-path amocas to W=0 raises store pf (15)", ok);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-07: FAILED CAS (compare mismatch) to R=1/W=0 page -> STILL store
 *            page-fault (15). norm:Zacas_amocas_w_permission is
 *            unconditional: the write permission check applies even when
 *            the comparison fails and no logical write occurs.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_07_cas_failed_no_write);
bool test_hzacas_07_cas_failed_no_write(void)
{
    TEST_BEGIN("HZACAS-07: FAILED amocas to W=0 -> still store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    int ok = hzacas_cas_vs_fault((uintptr_t)test_fault_page, HZ_VS_R,
                                 /*match*/0, /*deleg*/1,
                                 CAUSE_STORE_PAGE_FAULT,
                                 "FAILED amocas to W=0");
    TEST_ASSERT("FAILED amocas to W=0 still raises store pf (15) "
                "[norm:Zacas_amocas_w_permission]", ok);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-08: amocas to an unreadable (R=0) page -> store class (15),
 *            NEVER load class (13) (norm:store_page_fault_no_w NOTE).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_08_cas_unreadable_store_class);
bool test_hzacas_08_cas_unreadable_store_class(void)
{
    TEST_BEGIN("HZACAS-08: amocas to R=0 page -> store pf (15), not load (13)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_XONLY);
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("amocas to unreadable page faulted", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT_NEQ("cause != load page-fault (13)",
                    cause, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-09: hedeleg[15]=0 -> amocas VS-stage fault stays at HS/M level,
 *            cause still 15, for both success and failed CAS.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_09_hedeleg0_to_hs);
bool test_hzacas_09_hedeleg0_to_hs(void)
{
    TEST_BEGIN("HZACAS-09: hedeleg[15]=0 -> amocas fault stays at HS/M (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    int ok_s = hzacas_cas_vs_fault((uintptr_t)test_fault_page, HZ_VS_R,
                                   /*match*/1, /*deleg*/0,
                                   CAUSE_STORE_PAGE_FAULT,
                                   "success amocas, hedeleg=0");
    int ok_f = hzacas_cas_vs_fault((uintptr_t)test_fault_page, HZ_VS_R,
                                   /*match*/0, /*deleg*/0,
                                   CAUSE_STORE_PAGE_FAULT,
                                   "FAILED amocas, hedeleg=0");
    TEST_ASSERT("success amocas fault captured at HS/M (cause 15)", ok_s);
    TEST_ASSERT("FAILED amocas fault captured at HS/M (cause 15)", ok_f);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-10: success vs failed CAS permission consistency - both raise
 *            store page-fault (15) on the same R=1/W=0 page.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_10_success_failed_consistency);
bool test_hzacas_10_success_failed_consistency(void)
{
    TEST_BEGIN("HZACAS-10: success and failed CAS both -> store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    int ok_s = hzacas_cas_vs_fault((uintptr_t)test_fault_page, HZ_VS_R,
                                   /*match*/1, /*deleg*/0,
                                   CAUSE_STORE_PAGE_FAULT,
                                   "success amocas");
    int ok_f = hzacas_cas_vs_fault((uintptr_t)test_fault_page, HZ_VS_R,
                                   /*match*/0, /*deleg*/0,
                                   CAUSE_STORE_PAGE_FAULT,
                                   "FAILED amocas");
    TEST_ASSERT("success CAS -> store pf (15)", ok_s);
    TEST_ASSERT("failed CAS -> store pf (15) (permission independent of "
                "comparison result)", ok_f);

    HYP_TEST_END();
}
