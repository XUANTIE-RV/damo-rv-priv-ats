/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.2 - Exception-class split and VS-stage delegation path
 *             (HZLRSC-05 ~ HZLRSC-08)
 *
 * Spec basis:
 *   norm:hedeleg_acc  - hedeleg bits 4/5/6/7/13/15 Writable, 21/23 RO-0.
 *   norm:hedeleg_op   - V=1 + hedeleg bit set -> trap further delegated
 *                       to VS-mode; otherwise it stays at HS/M level.
 *   norm:mcause_exccode_ld_ldrsv  - LR (load-reserved) -> load class.
 *   norm:mcause_exccode_st_sc_amo - SC (store-conditional) -> store/AMO.
 *   norm:load_page_fault_no_r  - LR without read perm -> load page-fault.
 *   norm:store_page_fault_no_w - SC without write perm -> store page-fault.
 * =================================================================== */

/* ------------------------------------------------------------------
 * HZLRSC-05: hedeleg bit-property probe for the LR/SC causes.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_05_hedeleg_bits);
bool test_hzlrsc_05_hedeleg_bits(void)
{
    TEST_BEGIN("HZLRSC-05: hedeleg 4/5/6/7/13/15 writable, 21/23 RO-0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");

    uintptr_t vs_bits = (1UL << 4) | (1UL << 5) | (1UL << 6) | (1UL << 7) |
                        (1UL << 13) | (1UL << 15);
    uintptr_t g_bits  = (1UL << 21) | (1UL << 23);

    uintptr_t saved = hedeleg_read();
    hedeleg_write(vs_bits | g_bits);
    uintptr_t rb = hedeleg_read();

    TEST_ASSERT_BITS("hedeleg VS-stage bits (4/5/6/7/13/15) writable",
                     rb, vs_bits, vs_bits);
    TEST_ASSERT_BITS("hedeleg G-stage bits (21/23) read-only-0",
                     rb, g_bits, (uintptr_t)0);

    hedeleg_write(saved);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-06: LR VS-stage read-permission fault -> load page-fault (13)
 *            delegated to VS-mode; vsepc = lr.w PC; never cause=15.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_06_lr_vs_load_fault);
bool test_hzlrsc_06_lr_vs_load_fault(void)
{
    TEST_BEGIN("HZLRSC-06: LR VS-stage R=0 -> load page-fault (13) to VS");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_XONLY);

    hyp_delegate_to_vs((1UL << CAUSE_LOAD_PAGE_FAULT), 0);
    hz_vs_handler_install();

    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);

    TEST_ASSERT("VS-mode trap was triggered", g_hz_vs_triggered);
    TEST_ASSERT_EQ("vscause == load page-fault (13)",
                   g_hz_vs_cause, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT_NEQ("vscause != store page-fault (15)",
                    g_hz_vs_cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    /* vsepc points at the faulting lr.w (opcode 0x2f, funct5=00010,
     * funct3=010). */
    uint32_t inst = hyp_fetch_inst32(g_hz_vs_epc);
    TEST_ASSERT_EQ("vsepc inst opcode == AMO (0x2f)",
                   inst & 0x7FUL, (uintptr_t)0x2FUL);
    TEST_ASSERT_EQ("vsepc inst funct5 == LR (00010)",
                   (inst >> 27) & 0x1FUL, (uintptr_t)0x02UL);
    TEST_ASSERT_EQ("vsepc inst funct3 == .w (010)",
                   (inst >> 12) & 0x7UL, (uintptr_t)0x02UL);

#ifdef SHVSTVALA_SUPPORTED
    TEST_ASSERT_EQ("vstval == faulting GVA (Shvstvala)", g_hz_vs_tval, va);
#else
    printf("  [INFO] vstval=0x%lx (Shvstvala not declared; base H allows 0)\n",
           (unsigned long)g_hz_vs_tval);
#endif

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-07: SC VS-stage write-permission fault -> store/AMO page-fault
 *            (15) delegated to VS-mode; never cause=13.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_07_sc_vs_store_fault);
bool test_hzlrsc_07_sc_vs_store_fault(void)
{
    TEST_BEGIN("HZLRSC-07: SC VS-stage R=1/W=0 -> store page-fault (15) to VS");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    /* R=1/W=0 so the LR succeeds and only the SC faults. */
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);

    hyp_delegate_to_vs((1UL << CAUSE_STORE_PAGE_FAULT), 0);
    hz_vs_handler_install();

    (void)two_stage_run_in_vs(&ctx, hz_vs_lrsc_w, va);

    TEST_ASSERT("VS-mode trap was triggered", g_hz_vs_triggered);
    TEST_ASSERT_EQ("vscause == store/AMO page-fault (15)",
                   g_hz_vs_cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT_NEQ("vscause != load page-fault (13)",
                    g_hz_vs_cause, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-08: hedeleg=0 -> LR/SC VS-stage faults are NOT delegated to
 *            VS-mode; they are captured at the hypervisor (HS/M) level
 *            with the cause unchanged (13 for LR, 15 for SC).
 *
 * Framework note: with medeleg=0 the hypervisor-level trap is captured
 * by the M-mode record (trap_get_cause). The architecturally relevant
 * outcome - "not delivered to the guest VS-mode, cause preserved" - is
 * asserted by verifying the VS handler did NOT fire while the cause is
 * still 13/15 at the privileged capture point (norm:hedeleg_op).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_08_hedeleg0_not_to_vs);
bool test_hzlrsc_08_hedeleg0_not_to_vs(void)
{
    TEST_BEGIN("HZLRSC-08: hedeleg=0 -> LR/SC faults stay at HS/M (13/15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    /* --- LR read fault (cause 13), hedeleg[13]=0 --- */
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_XONLY);
    hedeleg_write(hedeleg_read() & ~(1UL << CAUSE_LOAD_PAGE_FAULT));
    hz_vs_handler_install();

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired_lr = trap_was_triggered();
    uintptr_t cause_lr = fired_lr ? trap_get_cause() : 0;
    trap_expect_end();
    bool vs_lr = g_hz_vs_triggered;
    ts2_finish(&ctx);

    TEST_ASSERT("LR fault captured at HS/M level", fired_lr);
    TEST_ASSERT_EQ("LR cause == load page-fault (13)",
                   cause_lr, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT("LR fault NOT delegated to VS-mode", !vs_lr);

    /* --- SC write fault (cause 15), hedeleg[15]=0 --- */
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);
    hedeleg_write(hedeleg_read() & ~(1UL << CAUSE_STORE_PAGE_FAULT));
    hz_vs_handler_install();

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lrsc_w, va);
    bool fired_sc = trap_was_triggered();
    uintptr_t cause_sc = fired_sc ? trap_get_cause() : 0;
    trap_expect_end();
    bool vs_sc = g_hz_vs_triggered;
    ts2_finish(&ctx);

    TEST_ASSERT("SC fault captured at HS/M level", fired_sc);
    TEST_ASSERT_EQ("SC cause == store/AMO page-fault (15)",
                   cause_sc, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT("SC fault NOT delegated to VS-mode", !vs_sc);

    HYP_TEST_END();
}
