/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4.2 - byte/half AMO exceptions all land in the store/AMO class
 *             and the VS-stage delegation path (HZABHA-05 ~ HZABHA-09)
 *
 * Spec basis:
 *   norm:mcause_exccode_st_sc_amo - byte/half AMOs (AMO family) generate
 *     store/AMO exceptions (cause 6/7/15/23), NEVER load class, regardless
 *     of access width.
 *   norm:store_page_fault_no_w (+ NOTE) - an AMO needs write permission;
 *     on an unreadable page it always raises a store page-fault (15).
 *   norm:hedeleg_acc / norm:hedeleg_op - hedeleg[6/7/15] Writable,
 *     hedeleg[23] read-only-0.
 *   Permission is width-independent: .b/.h/.w all require R+W (HZABHA-09).
 * =================================================================== */

/* Word AMO probe for the width-independence contrast (HZABHA-09). */
static uintptr_t hzabha_vs_amo_add_w(uintptr_t addr)
{
    return mem_amo_add_w(addr, 1u);
}

/* Run a byte/half/word AMO against a VS-stage victim, report the trap. */
static int hzabha_amo_vs_fault(uintptr_t victim_va, uintptr_t vs_flags,
                               uintptr_t (*probe)(uintptr_t),
                               int delegate_to_vs, uintptr_t exp_cause,
                               const char *tag)
{
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, victim_va, vs_flags);
    *(volatile uint64_t *)victim_va = 0x0011223344556677ULL;

    int fired;
    uintptr_t cause = 0;
    if (delegate_to_vs) {
        hyp_delegate_to_vs((1UL << CAUSE_STORE_PAGE_FAULT), 0);
        hz_vs_handler_install();
        (void)two_stage_run_in_vs(&ctx, probe, victim_va);
        fired = g_hz_vs_triggered;
        cause = g_hz_vs_cause;
    } else {
        hedeleg_write(hedeleg_read() & ~(1UL << CAUSE_STORE_PAGE_FAULT));
        trap_expect_begin();
        (void)two_stage_run_in_vs(&ctx, probe, victim_va);
        fired = trap_was_triggered();
        cause = fired ? trap_get_cause() : 0;
        trap_expect_end();
    }
    ts2_finish(&ctx);

    printf("  [INFO] %s: fired=%d cause=%lu (expected %lu)%s\n",
           tag, fired, (unsigned long)cause, (unsigned long)exp_cause,
           delegate_to_vs ? " [deleg->VS]" : "");
    return fired && cause == exp_cause;
}

/* ------------------------------------------------------------------
 * HZABHA-05: hedeleg bit-property probe for the byte/half AMO causes.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_05_hedeleg_bits);
bool test_hzabha_05_hedeleg_bits(void)
{
    TEST_BEGIN("HZABHA-05: hedeleg 6/7/15 writable, 23 RO-0");
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
 * HZABHA-06: byte/half AMO to R=1/W=0 VS-stage page -> store page-fault
 *            (15), delegated to VS-mode.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_06_bh_amo_vs_store_fault);
bool test_hzabha_06_bh_amo_vs_store_fault(void)
{
    TEST_BEGIN("HZABHA-06: byte/half AMO to W=0 -> store pf (15) to VS");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    int ok_b = hzabha_amo_vs_fault(va, HZ_VS_R, hz_vs_amo_add_b,
                                   /*deleg*/1, CAUSE_STORE_PAGE_FAULT,
                                   "amoadd.b to W=0");
    int ok_h = hzabha_amo_vs_fault(va, HZ_VS_R, hz_vs_amo_add_h,
                                   /*deleg*/1, CAUSE_STORE_PAGE_FAULT,
                                   "amoadd.h to W=0");
    TEST_ASSERT("amoadd.b to W=0 raises store pf (15)", ok_b);
    TEST_ASSERT("amoadd.h to W=0 raises store pf (15)", ok_h);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-07: byte/half AMO to an unreadable (R=0) page -> store class
 *            (15), NEVER load class (13).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_07_bh_amo_unreadable_store_class);
bool test_hzabha_07_bh_amo_unreadable_store_class(void)
{
    TEST_BEGIN("HZABHA-07: byte/half AMO to R=0 -> store pf (15), not load");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    int ok_b = hzabha_amo_vs_fault(va, HZ_VS_XONLY, hz_vs_amo_add_b,
                                   /*deleg*/0, CAUSE_STORE_PAGE_FAULT,
                                   "amoadd.b to R=0");
    int ok_h = hzabha_amo_vs_fault(va, HZ_VS_XONLY, hz_vs_amo_add_h,
                                   /*deleg*/0, CAUSE_STORE_PAGE_FAULT,
                                   "amoadd.h to R=0");
    TEST_ASSERT("amoadd.b to R=0 raises store pf (15), not load (13)", ok_b);
    TEST_ASSERT("amoadd.h to R=0 raises store pf (15), not load (13)", ok_h);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-08: hedeleg[15]=0 -> byte/half AMO VS-stage fault stays at HS/M
 *            level, cause still 15.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_08_hedeleg0_to_hs);
bool test_hzabha_08_hedeleg0_to_hs(void)
{
    TEST_BEGIN("HZABHA-08: hedeleg[15]=0 -> byte/half AMO fault at HS/M (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    int ok = hzabha_amo_vs_fault(va, HZ_VS_R, hz_vs_amo_add_b,
                                 /*deleg*/0, CAUSE_STORE_PAGE_FAULT,
                                 "amoadd.b to W=0, hedeleg=0");
    TEST_ASSERT("byte AMO fault captured at HS/M (cause 15)", ok);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-09: permission is width-independent: .b/.h/.w all raise store
 *            page-fault (15) on the same R=1/W=0 page.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_09_width_independent_permission);
bool test_hzabha_09_width_independent_permission(void)
{
    TEST_BEGIN("HZABHA-09: .b/.h/.w all -> store pf (15) on R=1/W=0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    int ok_b = hzabha_amo_vs_fault(va, HZ_VS_R, hz_vs_amo_add_b,
                                   /*deleg*/0, CAUSE_STORE_PAGE_FAULT,
                                   "amoadd.b");
    int ok_h = hzabha_amo_vs_fault(va, HZ_VS_R, hz_vs_amo_add_h,
                                   /*deleg*/0, CAUSE_STORE_PAGE_FAULT,
                                   "amoadd.h");
    int ok_w = hzabha_amo_vs_fault(va, HZ_VS_R, hzabha_vs_amo_add_w,
                                   /*deleg*/0, CAUSE_STORE_PAGE_FAULT,
                                   "amoadd.w");
    TEST_ASSERT("amoadd.b -> store pf (15)", ok_b);
    TEST_ASSERT("amoadd.h -> store pf (15)", ok_h);
    TEST_ASSERT("amoadd.w -> store pf (15) (width-independent)", ok_w);

    HYP_TEST_END();
}
