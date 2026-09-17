/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2.2 - AMO exceptions all land in the store/AMO class and the
 *             VS-stage delegation path (HZAMO-05 ~ HZAMO-09)
 *
 * Spec basis:
 *   norm:mcause_exccode_st_sc_amo - AMO instructions generate store/AMO
 *     exceptions (cause 6/7/15/23), NEVER load class (4/5/13/21).
 *   norm:store_page_fault_no_w (+ NOTE) - an AMO needs write permission;
 *     an AMO on an unreadable page always raises a store page-fault (15),
 *     never a load page-fault (13).
 *   norm:hedeleg_acc / norm:hedeleg_op - hedeleg[6/7/15] Writable (VS-stage
 *     faults delegable to VS-mode), hedeleg[23] read-only-0.
 *   norm:load_page_fault_no_r - LR needs only read permission (contrast:
 *     LR to R=1/W=0 executes, AMO to R=1/W=0 faults) - HZAMO-09.
 * =================================================================== */

/* LR probe for the LR-vs-AMO permission contrast (HZAMO-09). */
static uintptr_t hzamo_vs_lr_w(uintptr_t addr)
{
    uintptr_t rd;
    asm volatile(".option push\n\t.option norvc\n\t"
                 "lr.w %0, (%1)\n\t.option pop\n\t"
                 : "=r"(rd) : "r"(addr) : "memory");
    return rd;
}

/* ------------------------------------------------------------------
 * HZAMO-05: hedeleg bit-property probe for the AMO causes.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_05_hedeleg_bits);
bool test_hzamo_05_hedeleg_bits(void)
{
    TEST_BEGIN("HZAMO-05: hedeleg 6/7/15 writable, 23 RO-0");
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
 * HZAMO-06: AMO to R=1/W=0 VS-stage page -> store page-fault (15),
 *           delegated to VS-mode.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_06_amo_vs_store_fault);
bool test_hzamo_06_amo_vs_store_fault(void)
{
    TEST_BEGIN("HZAMO-06: AMO VS-stage R=1/W=0 -> store pf (15) to VS");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);

    hyp_delegate_to_vs((1UL << CAUSE_STORE_PAGE_FAULT), 0);
    hz_vs_handler_install();

    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, va);

    TEST_ASSERT("VS-mode trap was triggered", g_hz_vs_triggered);
    TEST_ASSERT_EQ("vscause == store/AMO page-fault (15)",
                   g_hz_vs_cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    /* vsepc points at the faulting amoadd.w (opcode 0x2f, funct3=010). */
    uint32_t inst = hyp_fetch_inst32(g_hz_vs_epc);
    TEST_ASSERT_EQ("vsepc inst opcode == AMO (0x2f)",
                   inst & 0x7FUL, (uintptr_t)0x2FUL);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-07: AMO to an unreadable (R=0) page -> store class (15), NEVER
 *           load class (13) (norm:store_page_fault_no_w NOTE).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_07_amo_unreadable_store_class);
bool test_hzamo_07_amo_unreadable_store_class(void)
{
    TEST_BEGIN("HZAMO-07: AMO to R=0 page -> store pf (15), not load (13)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_XONLY);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("AMO to unreadable page faulted", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT_NEQ("cause != load page-fault (13)",
                    cause, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-08: hedeleg[15]=0 -> AMO VS-stage fault stays at HS/M level,
 *           cause still 15 (delegation changes target, not cause).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_08_hedeleg0_to_hs);
bool test_hzamo_08_hedeleg0_to_hs(void)
{
    TEST_BEGIN("HZAMO-08: hedeleg[15]=0 -> AMO fault stays at HS/M (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);
    hedeleg_write(hedeleg_read() & ~(1UL << CAUSE_STORE_PAGE_FAULT));
    hz_vs_handler_install();

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    bool vs = g_hz_vs_triggered;
    ts2_finish(&ctx);

    TEST_ASSERT("AMO fault captured at HS/M level", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT("AMO fault NOT delegated to VS-mode", !vs);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-09: LR (needs only R) vs AMO (needs R+W) on the same R=1/W=0
 *           page: LR executes, AMO faults with 15.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_09_lr_vs_amo_permission);
bool test_hzamo_09_lr_vs_amo_permission(void)
{
    TEST_BEGIN("HZAMO-09: LR executes but AMO faults on R=1/W=0 page");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    /* LR: read-only suffices -> no trap. */
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hzamo_vs_lr_w, va);
    bool lr_fired = trap_was_triggered();
    uintptr_t lr_cause = lr_fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);
    if (lr_fired)
        printf("  UNEXPECTED LR TRAP: cause=%lu\n", (unsigned long)lr_cause);
    TEST_ASSERT("LR to R=1/W=0 executes (needs only read)", !lr_fired);

    /* AMO: needs write -> store page-fault (15). */
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, va);
    bool amo_fired = trap_was_triggered();
    uintptr_t amo_cause = amo_fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);
    TEST_ASSERT("AMO to R=1/W=0 faulted", amo_fired);
    TEST_ASSERT_EQ("AMO cause == store/AMO page-fault (15)",
                   amo_cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    HYP_TEST_END();
}
