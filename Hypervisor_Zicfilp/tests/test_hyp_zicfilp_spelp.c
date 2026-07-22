/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group A2: vsstatus.SPELP field and VS-mode trap save/restore
 *
 * Tests:
 *   HCFI-LP-11: vsstatus.SPELP basic read/write
 *   HCFI-LP-12: VS-mode trap to VS-mode, SPELP saves ELP
 *   HCFI-LP-13: VS-mode SRET restores ELP (VSLPE=1)
 *   HCFI-LP-14: VS-mode SRET does not restore ELP (VSLPE=0)
 *   HCFI-LP-15: VS-mode trap to HS-mode, mstatus.SPELP saves ELP
 *   HCFI-LP-16: HS-mode SRET returns VS-mode, restores ELP (VSLPE=1)
 *   HCFI-LP-17: HS-mode SRET returns VS-mode, does not restore ELP (VSLPE=0)
 *   HCFI-LP-18: VS-mode trap to M-mode, mstatus.MPELP saves ELP
 *   HCFI-LP-19: M-mode MRET returns VS-mode, restores ELP (VSLPE=1)
 *   HCFI-LP-20: M-mode MRET returns VS-mode, does not restore ELP (VSLPE=0)
 *   HCFI-LP-21: ELP=NO_LP_EXPECTED, trap saves SPELP=0
 *   HCFI-LP-22: VS-mode SRET to VU-mode, ELP restored
 *   HCFI-LP-23: VS-mode SRET to VU-mode, VULPE=0
 *   HCFI-LP-24: V=0, vsstatus.SPELP does not affect behavior
 *
 * Norm references:
 *   norm:vsstatus_spelp_op - SPELP holds previous ELP in vsstatus
 *   norm:Zicfilp_pelp_trap - xpelp set to ELP on trap, ELP set to NO_LP_EXPECTED
 *   norm:Zicfilp_pelp_trap_return - xRET restores ELP from xpelp if yLPE=1
 */

/* ===================================================================
 * HCFI-LP-11: vsstatus.SPELP basic read/write
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_11);
bool test_hcfi_lp_11(void) {
    TEST_BEGIN("HCFI-LP-11: vsstatus.SPELP basic read/write");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    uintptr_t orig = vsstatus_read();

    /* Try to set SPELP */
    vsstatus_write(orig | VSSTATUS_SPELP_BIT);
    uintptr_t val = vsstatus_read();

    TEST_ASSERT("vsstatus.SPELP can be set to 1",
                (val & VSSTATUS_SPELP_BIT) != 0);

    /* Clear SPELP */
    vsstatus_write(orig & ~VSSTATUS_SPELP_BIT);
    val = vsstatus_read();
    TEST_ASSERT("vsstatus.SPELP can be cleared to 0",
                (val & VSSTATUS_SPELP_BIT) == 0);

    /* Restore */
    vsstatus_write(orig);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-12: VS-mode trap to VS-mode, SPELP saves ELP
 *
 * Set henvcfg.LPE=1. In VS-mode, when ELP=LP_EXPECTED and a trap
 * is delivered to VS-mode, vsstatus.SPELP should be set to 1.
 *
 * We trigger a software-check exception (cause=18) which is delegated
 * to VS-mode. At trap entry, SPELP should capture ELP=LP_EXPECTED.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_12);
bool test_hcfi_lp_12(void) {
    TEST_BEGIN("HCFI-LP-12: VS-mode trap to VS-mode, SPELP saves ELP");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Enable LPE for VS-mode */
    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Delegate software-check exception to VS-mode */
    setup_deleg_to_vs(BIT(CAUSE_SOFTWARE_CHECK));

    /* Prepare code: NOP (non-LPAD) + RET - will trigger LP Fault */
    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* Clear vsstatus.SPELP before the test */
    vsstatus_write(vsstatus_read() & ~VSSTATUS_SPELP_BIT);

    /* VS-mode indirect jump to non-LPAD - triggers LP Fault delegated to VS */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    /* The trap should have been delegated to VS-mode. The VS-mode
     * handler recorded vscause and the SPELP value captured at trap
     * entry (per norm:Zicfilp_pelp_trap, xPELP is set to ELP on trap). */
    printf("    VS handler: triggered=%d vscause=%lu SPELP at trap=%lu\n",
           (int)g_vs_exc_triggered, (unsigned long)g_vs_exc_cause,
           (unsigned long)g_vs_exc_spelp);

    TEST_ASSERT("LP fault delegated to VS-mode handler",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("vsstatus.SPELP captured LP_EXPECTED at VS trap entry",
                g_vs_exc_spelp == 1);
    TEST_ASSERT("no trap escaped to M-mode",
                result == 0);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-13: VS-mode SRET restores ELP (VSLPE=1)
 *
 * After HCFI-LP-12, SRET in VS-mode should restore ELP from SPELP
 * when VSLPE=1 (henvcfg.LPE=1).
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_13);
bool test_hcfi_lp_13(void) {
    TEST_BEGIN("HCFI-LP-13: VS-mode SRET restores ELP (VSLPE=1)");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Delegate software-check to VS, using the two-entry handler that
     * keeps SPELP intact on the first entry. */
    setup_deleg_to_vs_ex(BIT(CAUSE_SOFTWARE_CHECK),
                         (uintptr_t)vs_exc_handler_noclear);

    /* Code buffer layout: NOP (faults), NOP (re-faults after the SRET
     * restores ELP), RET (returns cleanly once ELP is cleared).
     * Note: for a landing pad fault, xepc is the address of the target
     * instruction (the non-LPAD instruction), so the VS handler resumes
     * inside this buffer. */
    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_nop(code + 1);
    emit_ret(code + 2);
    flush_cfi_code();

    /* VS-mode JALR to non-LPAD:
     * - LP fault #1: vsstatus.SPELP <- ELP(LP_EXPECTED)=1, ELP<-0.
     *   Handler entry #1 keeps SPELP=1 and resumes at NOP #2.
     * - Per norm:Zicfilp_pelp_trap_return, the SRET restores
     *   ELP <- SPELP = LP_EXPECTED (VSLPE=1), so NOP #2 (non-LPAD)
     *   must raise LP fault #2.
     * - Handler entry #2 clears SPELP and resumes at the RET, which
     *   returns normally. */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);
    (void)result;

    printf("    VS handler: count=%u cause1=%lu spelp1=%lu cause2=%lu\n",
           g_vs_exc_count, (unsigned long)g_vs_exc_cause,
           (unsigned long)g_vs_exc_spelp, (unsigned long)g_vs_exc_cause2);

    TEST_ASSERT("two LP faults (initial + post-SRET restore)",
                g_vs_exc_count == 2);
    TEST_ASSERT_EQ("fault #1 is software-check",
                   g_vs_exc_cause, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("fault #1 saved ELP=LP_EXPECTED into SPELP",
                g_vs_exc_spelp == 1);
    TEST_ASSERT_EQ("fault #2 (after SRET) is software-check",
                   g_vs_exc_cause2, (uintptr_t)CAUSE_SOFTWARE_CHECK);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    /* Clean up vsstatus.SPELP so later tests start from a known state */
    vsstatus_write(vsstatus_read() & ~VSSTATUS_SPELP_BIT);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-14: VS-mode SRET does not restore ELP (VSLPE=0)
 *
 * Set henvcfg.LPE=0. After a trap, SRET should set ELP to
 * NO_LP_EXPECTED (VSLPE=0), regardless of SPELP.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_14);
bool test_hcfi_lp_14(void) {
    TEST_BEGIN("HCFI-LP-14: VS-mode SRET clears SPELP (VSLPE=0)");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set henvcfg.LPE=0 - VSLPE=0 */
    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(false, true);
    mseccfg_clear(MSECCFG_MLPE);

    /* Delegate store page fault to VS, using the handler that forces
     * SPELP=1 before its sret. A natural trap entry cannot create
     * SPELP=1 with VSLPE=0 (ELP is always NO_LP_EXPECTED when
     * henvcfg.LPE=0), so the handler sets it explicitly. */
    setup_deleg_to_vs_ex(BIT(CAUSE_STORE_PAGE_FAULT),
                         (uintptr_t)vs_exc_handler_set_spelp);

    /* Make test_fault_page invalid in VS-stage so the store faults. */
    vs_pte_modify(&ctx, (uintptr_t)test_fault_page, PT_LEVEL_4K, 0);

    /* Trigger a store page fault in VS-mode. Per
     * norm:Zicfilp_pelp_trap_return, the handler's sret sets ELP to
     * NO_LP_EXPECTED (VSLPE=0) and sets SPELP to NO_LP_EXPECTED
     * unconditionally. */
    trap_expect_begin();
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_store,
                                           (uintptr_t)test_fault_page);
    trap_expect_end();
    (void)result;

    TEST_ASSERT("store page fault delivered to VS handler",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_STORE_PAGE_FAULT);

    /* The handler set SPELP=1 before sret; hardware must have cleared
     * it on sret. */
    uintptr_t spelp = vsstatus_read() & VSSTATUS_SPELP_BIT;
    printf("    vsstatus.SPELP after SRET (VSLPE=0) = %lu\n",
           (unsigned long)(spelp ? 1 : 0));
    TEST_ASSERT("vsstatus.SPELP cleared by hardware on SRET",
                spelp == 0);

    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-15: VS-mode trap to HS-mode, mstatus.SPELP saves ELP
 *
 * Set henvcfg.LPE=1, no delegation to VS (hedeleg=0). VS-mode
 * indirect jump to non-LPAD triggers LP Fault trapped to HS-mode.
 * mstatus.SPELP should capture ELP=LP_EXPECTED.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_15);
bool test_hcfi_lp_15(void) {
    TEST_BEGIN("HCFI-LP-15: VS-mode trap to HS-mode, mstatus.SPELP saves ELP");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Delegate to HS only (no hedeleg) */
    setup_deleg_to_hs(BIT(CAUSE_SOFTWARE_CHECK));

    /* Clear mstatus.SPELP and vsstatus.SPELP for a clean start */
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_SPELP_BIT) : "memory");
    vsstatus_write(vsstatus_read() & ~VSSTATUS_SPELP_BIT);

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    /* LPAD safety net: on simulators with a VS->HS trap ELP/SPELP
     * save bug (e.g. QEMU saves ELP to vsstatus.SPELP instead of
     * mstatus.SPELP, see test plan known-gap note), sret may leave
     * ELP=LP_EXPECTED.  Without this LPAD the subsequent RET would
     * raise a second software-check while armed=false, crashing with
     * UNEXPECTED TRAP.  The LPAD absorbs the stale LP_EXPECTED so
     * the test reports the SPELP=0 mismatch via a normal ASSERT
     * failure and later tests continue to run.  On SPEC-compliant
     * simulators (Spike) ELP is already NO_LP_EXPECTED after sret,
     * so the LPAD is a harmless no-op. */
    emit_lpad(code + 1);
    emit_ret(code + 2);
    flush_cfi_code();

    /* VS-mode indirect jump to non-LPAD - LP Fault trapped to HS */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    /* Check mstatus.SPELP was set at trap entry. The HS-mode handler
     * clears SPELP before sret for clean recovery, so verify against
     * the status snapshot taken at trap entry. */
    uintptr_t snap = trap_get_status_snap();
    uintptr_t spelp = (snap & MSTATUS_SPELP_BIT) ? 1 : 0;
    printf("    mstatus.SPELP at HS trap = %lu (expect 1)\n",
           (unsigned long)spelp);

    TEST_ASSERT_EQ("software-check exception trapped to HS-mode",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("mstatus.SPELP captured LP_EXPECTED at HS trap entry",
                spelp == 1);

    /* Clear SPELP */
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_SPELP_BIT) : "memory");

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-16: HS-mode SRET returns VS-mode, restores ELP (VSLPE=1)
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_16);
bool test_hcfi_lp_16(void) {
    TEST_BEGIN("HCFI-LP-16: HS-mode SRET returns VS, restores ELP (VSLPE=1)");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    /* Map test_exec_target with X=0 so the JALR target fetch faults
     * with an instruction page fault (ELP=LP_EXPECTED is preserved
     * across the trap, per norm:Zicfilp_forward_traps). */
    ts2_setup_with_vs_victim(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4,
                             (uintptr_t)test_exec_target,
                             VS_FLAGS_RW_S_AD);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Instruction page fault -> HS (medeleg only); software-check ->
     * VS (medeleg + hedeleg) so the post-SRET re-fault is handled. */
    CSRS(medeleg, BIT(CAUSE_INST_PAGE_FAULT) | BIT(CAUSE_SOFTWARE_CHECK));
    CSRS(CSR_HEDELEG, BIT(CAUSE_SOFTWARE_CHECK));
    CSRC(CSR_HEDELEG, BIT(CAUSE_INST_PAGE_FAULT));
    setup_deleg_to_vs_ex(0, (uintptr_t)vs_exc_handler);

    /* Keep the ELP saved in mstatus.SPELP across the HS handler so the
     * sret restores it (the default handler clearing would hide the
     * hardware behavior under test). */
    g_trap_preserve_pelp = 1;
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_SPELP_BIT) : "memory");

    /* VS-mode JALR to the non-executable page:
     * - JALR sets ELP=LP_EXPECTED; the fetch faults -> instruction
     *   page fault to HS, mstatus.SPELP <- LP_EXPECTED.
     * - HS handler resumes at the trampoline recovery PC with SPELP
     *   preserved; sret restores ELP=LP_EXPECTED (VSLPE=1).
     * - The recovery instruction (non-LPAD) raises a software-check,
     *   which is delegated to the VS handler. */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_unmapped,
                                           (uintptr_t)test_exec_target);

    g_trap_preserve_pelp = 0;

    uintptr_t snap = trap_get_status_snap();
    printf("    first trap cause=%lu SPELP at HS entry=%lu re-fault=%d cause=%lu\n",
           (unsigned long)result,
           (unsigned long)((snap & MSTATUS_SPELP_BIT) ? 1 : 0),
           (int)g_vs_exc_triggered, (unsigned long)g_vs_exc_cause);

    TEST_ASSERT_EQ("instruction page fault trapped to HS",
                   result, (uintptr_t)CAUSE_INST_PAGE_FAULT);
    TEST_ASSERT("mstatus.SPELP captured LP_EXPECTED at HS trap entry",
                (snap & MSTATUS_SPELP_BIT) != 0);
    TEST_ASSERT("software-check after HS SRET proves ELP restored",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("mstatus.SPELP cleared by hardware on SRET",
                (mstatus_read() & MSTATUS_SPELP_BIT) == 0);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-17: HS-mode SRET returns VS, does not restore ELP (VSLPE=0)
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_17);
bool test_hcfi_lp_17(void) {
    TEST_BEGIN("HCFI-LP-17: HS-mode SRET returns VS, clears SPELP (VSLPE=0)");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(false, true);
    mseccfg_clear(MSECCFG_MLPE);
    setup_deleg_to_hs(BIT(CAUSE_STORE_PAGE_FAULT));

    /* Make test_fault_page invalid in VS-stage so the store faults. */
    vs_pte_modify(&ctx, (uintptr_t)test_fault_page, PT_LEVEL_4K, 0);

    /* Force mstatus.SPELP=1 in the HS handler before its sret (a
     * natural trap entry cannot create SPELP=1 with VSLPE=0 because
     * ELP is always NO_LP_EXPECTED when henvcfg.LPE=0). Per
     * norm:Zicfilp_pelp_trap_return, hardware must clear SPELP on
     * sret unconditionally. */
    g_trap_preserve_pelp = 1;
    g_trap_force_pelp = 1;

    /* Trigger store page fault in VS-mode, trapped to HS */
    trap_expect_begin();
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_store,
                                           (uintptr_t)test_fault_page);
    trap_expect_end();

    g_trap_preserve_pelp = 0;
    g_trap_force_pelp = 0;

    TEST_ASSERT_EQ("store page fault trapped to HS",
                   (uintptr_t)trap_get_cause(),
                   (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    /* The handler forced SPELP=1 before sret; hardware must have
     * cleared it on sret (VSLPE=0). */
    uintptr_t ms = mstatus_read();
    uintptr_t spelp = (ms & MSTATUS_SPELP_BIT) ? 1 : 0;
    printf("    mstatus.SPELP after HS SRET (VSLPE=0) = %lu\n",
           (unsigned long)spelp);

    TEST_ASSERT("mstatus.SPELP cleared by hardware on SRET (VSLPE=0)",
                spelp == 0);

    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-18: VS-mode trap to M-mode, mstatus.MPELP saves ELP
 *
 * No delegation (medeleg=0). VS-mode LP Fault trapped to M-mode.
 * mstatus.MPELP should capture ELP=LP_EXPECTED.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_18);
bool test_hcfi_lp_18(void) {
    TEST_BEGIN("HCFI-LP-18: VS-mode trap to M-mode, mstatus.MPELP saves ELP");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* No delegation - trap goes to M-mode */
    clear_all_deleg();

    /* Clear mstatus.MPELP */
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_MPELP_BIT) : "memory");

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* VS-mode indirect jump to non-LPAD - LP Fault trapped to M */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    /* Check mstatus.MPELP was set at trap entry. The M-mode handler
     * clears MPELP before mret for clean recovery, so verify against
     * the status snapshot taken at trap entry. */
    uintptr_t snap = trap_get_status_snap();
    uintptr_t mpelp = (snap & MSTATUS_MPELP_BIT) ? 1 : 0;
    printf("    mstatus.MPELP at M trap = %lu (expect 1)\n",
           (unsigned long)mpelp);

    TEST_ASSERT_EQ("software-check exception trapped to M-mode",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("mstatus.MPELP captured LP_EXPECTED at M trap entry",
                mpelp == 1);

    /* Clear MPELP */
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_MPELP_BIT) : "memory");

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-19: M-mode MRET returns VS-mode, restores ELP (VSLPE=1)
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_19);
bool test_hcfi_lp_19(void) {
    TEST_BEGIN("HCFI-LP-19: M-mode MRET returns VS, restores ELP (VSLPE=1)");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    /* Map test_exec_target with X=0 so the JALR target fetch faults
     * with an instruction page fault (ELP=LP_EXPECTED is preserved). */
    ts2_setup_with_vs_victim(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4,
                             (uintptr_t)test_exec_target,
                             VS_FLAGS_RW_S_AD);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Instruction page fault -> M-mode (no medeleg); software-check ->
     * VS (medeleg + hedeleg) so the post-MRET re-fault is handled. */
    CSRC(medeleg, BIT(CAUSE_INST_PAGE_FAULT));
    CSRS(medeleg, BIT(CAUSE_SOFTWARE_CHECK));
    CSRS(CSR_HEDELEG, BIT(CAUSE_SOFTWARE_CHECK));
    setup_deleg_to_vs_ex(0, (uintptr_t)vs_exc_handler);

    /* Keep the ELP saved in mstatus.MPELP across the M-mode handler so
     * the mret restores it. */
    g_trap_preserve_pelp = 1;
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_MPELP_BIT) : "memory");

    /* VS-mode JALR to the non-executable page:
     * - JALR sets ELP=LP_EXPECTED; the fetch faults -> instruction
     *   page fault to M, mstatus.MPELP <- LP_EXPECTED.
     * - M handler resumes at the trampoline recovery PC with MPELP
     *   preserved; mret restores ELP=LP_EXPECTED (VSLPE=1).
     * - The recovery instruction (non-LPAD) raises a software-check,
     *   which is delegated to the VS handler. */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_unmapped,
                                           (uintptr_t)test_exec_target);

    g_trap_preserve_pelp = 0;

    uintptr_t snap = trap_get_status_snap();
    printf("    first trap cause=%lu MPELP at M entry=%lu re-fault=%d cause=%lu\n",
           (unsigned long)result,
           (unsigned long)((snap & MSTATUS_MPELP_BIT) ? 1 : 0),
           (int)g_vs_exc_triggered, (unsigned long)g_vs_exc_cause);

    TEST_ASSERT_EQ("instruction page fault trapped to M",
                   result, (uintptr_t)CAUSE_INST_PAGE_FAULT);
    TEST_ASSERT("mstatus.MPELP captured LP_EXPECTED at M trap entry",
                (snap & MSTATUS_MPELP_BIT) != 0);
    TEST_ASSERT("software-check after MRET proves ELP restored",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("mstatus.MPELP cleared by hardware on MRET",
                (mstatus_read() & MSTATUS_MPELP_BIT) == 0);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-20: M-mode MRET returns VS-mode, does not restore ELP (VSLPE=0)
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_20);
bool test_hcfi_lp_20(void) {
    TEST_BEGIN("HCFI-LP-20: M-mode MRET returns VS, clears MPELP (VSLPE=0)");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(false, true);
    mseccfg_clear(MSECCFG_MLPE);
    clear_all_deleg();

    /* Make test_fault_page invalid in VS-stage so the store faults. */
    vs_pte_modify(&ctx, (uintptr_t)test_fault_page, PT_LEVEL_4K, 0);

    /* Force mstatus.MPELP=1 in the M-mode handler before its mret (a
     * natural trap entry cannot create MPELP=1 with VSLPE=0). Per
     * norm:Zicfilp_pelp_trap_return, hardware must clear MPELP on
     * mret unconditionally. */
    g_trap_preserve_pelp = 1;
    g_trap_force_pelp = 1;

    /* Trigger a trap in VS-mode that goes to M-mode */
    trap_expect_begin();
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_store,
                                           (uintptr_t)test_fault_page);
    trap_expect_end();

    g_trap_preserve_pelp = 0;
    g_trap_force_pelp = 0;

    TEST_ASSERT_EQ("store page fault trapped to M",
                   (uintptr_t)trap_get_cause(),
                   (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    /* The handler forced MPELP=1 before mret; hardware must have
     * cleared it on mret (VSLPE=0). */
    uintptr_t ms = mstatus_read();
    uintptr_t mpelp = (ms & MSTATUS_MPELP_BIT) ? 1 : 0;
    printf("    mstatus.MPELP after MRET (VSLPE=0) = %lu\n",
           (unsigned long)mpelp);

    TEST_ASSERT("mstatus.MPELP cleared by hardware on MRET (VSLPE=0)",
                mpelp == 0);

    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-21: ELP=NO_LP_EXPECTED, trap saves SPELP=0
 *
 * Set henvcfg.LPE=1. When ELP=NO_LP_EXPECTED at trap time,
 * vsstatus.SPELP should be set to 0 (NO_LP_EXPECTED).
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_21);
bool test_hcfi_lp_21(void) {
    TEST_BEGIN("HCFI-LP-21: ELP=NO_LP_EXPECTED, trap saves SPELP=0");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);
    setup_deleg_to_hs(BIT(CAUSE_STORE_PAGE_FAULT));

    /* Make test_fault_page invalid in VS-stage so the store faults. */
    vs_pte_modify(&ctx, (uintptr_t)test_fault_page, PT_LEVEL_4K, 0);

    /* Clear mstatus.SPELP to start fresh */
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_SPELP_BIT) : "memory");

    /* Trigger a store fault (not an indirect jump, so ELP stays
     * NO_LP_EXPECTED). The trap should save SPELP=0. */
    trap_expect_begin();
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_store,
                                           (uintptr_t)test_fault_page);
    trap_expect_end();

    /* Verify via the status snapshot taken at HS trap entry: the live
     * register is meaningless here because the framework handler
     * clears SPELP before sret for clean recovery. */
    uintptr_t snap = trap_get_status_snap();
    uintptr_t spelp = (snap & MSTATUS_SPELP_BIT) ? 1 : 0;
    printf("    mstatus.SPELP at HS trap entry = %lu (expect 0)\n",
           (unsigned long)spelp);

    TEST_ASSERT_EQ("store page fault trapped to HS",
                   (uintptr_t)trap_get_cause(),
                   (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT("SPELP=0 when ELP was NO_LP_EXPECTED at trap",
                spelp == 0);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-22: VS-mode SRET to VU-mode, ELP restored
 *
 * Set henvcfg.LPE=1, senvcfg.LPE=1. VS-mode with SPELP=1,
 * SRET to VU-mode (SPP=0). ELP should be restored from SPELP.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_22);
bool test_hcfi_lp_22(void) {
    TEST_BEGIN("HCFI-LP-22: VS-mode SRET to VU-mode, ELP restored (VULPE=1)");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    senvcfg_set(SENVCFG_LPE);
    mseccfg_set(MSECCFG_MLPE);

    /* VU target: NOP (non-LPAD) + SD zero,0(t0) (faulting store that
     * delivers the return trap), in test_exec_page (covered by a 4KB
     * VS-stage mapping). Make the page U=1 executable so VU-mode can
     * run it. Note: the CFI code buffer cannot be used here because
     * it is covered by a 2MB S-only VS-stage megapage. */
    uint32_t *code = (uint32_t *)test_exec_page;
    emit_nop(code);
    emit_sd_zero_t0(code + 1);
    flush_cfi_code();
    vs_pte_modify(&ctx, (uintptr_t)code, PT_LEVEL_4K, VS_FLAGS_RWXU_AD);

    /* software-check and store page fault -> VS handler; resume at
     * vs_landing_fn (S-level page) after the trap, since the faulting
     * PC sits in a U-only page that VS-mode cannot execute from. */
    setup_deleg_to_vs_ex(BIT(CAUSE_SOFTWARE_CHECK) | BIT(CAUSE_STORE_PAGE_FAULT),
                         (uintptr_t)vs_exc_handler);
    g_vs_exc_recovery = (uintptr_t)vs_landing_fn;
    g_vu_fault_addr = (uintptr_t)test_fault_page;

    /* Set SPELP=1 to simulate saved LP_EXPECTED */
    vsstatus_write(vsstatus_read() | VSSTATUS_SPELP_BIT);

    /* VS-mode SRET to VU (SPP=0): with VULPE = senvcfg.LPE = 1, ELP is
     * restored from vsstatus.SPELP (=LP_EXPECTED) per
     * norm:Zicfilp_pelp_trap_return, so the non-LPAD NOP at the VU
     * target must raise a software-check exception. */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_sret_to_vu,
                                           (uintptr_t)code);
    (void)result;

    printf("    VS handler: triggered=%d cause=%lu spelp=%lu tval=%lu\n",
           (int)g_vs_exc_triggered, (unsigned long)g_vs_exc_cause,
           (unsigned long)g_vs_exc_spelp, (unsigned long)g_vs_exc_tval);

    TEST_ASSERT("software-check in VU proves ELP restored on SRET",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("SPELP captured LP_EXPECTED at the VU trap",
                g_vs_exc_spelp == 1);
    TEST_ASSERT_EQ("vstval = landing pad fault (2)",
                   g_vs_exc_tval, (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    g_vs_exc_recovery = 0;
    senvcfg_clear(SENVCFG_LPE);
    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    /* Clean up vsstatus.SPELP so later tests start from a known state */
    vsstatus_write(vsstatus_read() & ~VSSTATUS_SPELP_BIT);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-23: VS-mode SRET to VU-mode, VULPE=0
 *
 * Set senvcfg.LPE=0. VS-mode SRET to VU-mode.
 * ELP should be set to NO_LP_EXPECTED (VULPE=0).
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_23);
bool test_hcfi_lp_23(void) {
    TEST_BEGIN("HCFI-LP-23: VS-mode SRET to VU-mode, VULPE=0");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    senvcfg_clear(SENVCFG_LPE);
    mseccfg_set(MSECCFG_MLPE);

    /* VU target: NOP (non-LPAD) + SD zero,0(t0) (faulting store that
     * delivers the return trap), in test_exec_page (covered by a 4KB
     * VS-stage mapping). Make the page U=1 executable so VU-mode can
     * run it. */
    uint32_t *code = (uint32_t *)test_exec_page;
    emit_nop(code);
    emit_sd_zero_t0(code + 1);
    flush_cfi_code();
    vs_pte_modify(&ctx, (uintptr_t)code, PT_LEVEL_4K, VS_FLAGS_RWXU_AD);

    /* software-check and store page fault -> VS handler; resume at
     * vs_landing_fn (S-level page) after the trap. */
    setup_deleg_to_vs_ex(BIT(CAUSE_SOFTWARE_CHECK) | BIT(CAUSE_STORE_PAGE_FAULT),
                         (uintptr_t)vs_exc_handler);
    g_vs_exc_recovery = (uintptr_t)vs_landing_fn;
    g_vu_fault_addr = (uintptr_t)test_fault_page;

    /* Set SPELP=1 */
    vsstatus_write(vsstatus_read() | VSSTATUS_SPELP_BIT);

    /* VS-mode SRET to VU (SPP=0): with VULPE = senvcfg.LPE = 0, ELP is
     * set to NO_LP_EXPECTED, so the NOP executes normally and the
     * faulting store (cause=15) delivers the return trap. */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_sret_to_vu,
                                           (uintptr_t)code);
    (void)result;

    printf("    VS handler: triggered=%d cause=%lu\n",
           (int)g_vs_exc_triggered, (unsigned long)g_vs_exc_cause);

    TEST_ASSERT("no software-check in VU (VULPE=0)",
                !(g_vs_exc_triggered &&
                  g_vs_exc_cause == CAUSE_SOFTWARE_CHECK));
    TEST_ASSERT("VU store fault reached (NOP executed normally)",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_STORE_PAGE_FAULT);

    g_vs_exc_recovery = 0;
    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    /* Clean up vsstatus.SPELP so later tests start from a known state */
    vsstatus_write(vsstatus_read() & ~VSSTATUS_SPELP_BIT);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-24: V=0, vsstatus.SPELP does not affect behavior
 *
 * When V=0, vsstatus.SPELP should have no effect on ELP behavior.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_24);
bool test_hcfi_lp_24(void) {
    TEST_BEGIN("HCFI-LP-24: V=0, vsstatus.SPELP does not affect behavior");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    /* Enable HS-level xLPE (menvcfg.LPE); set vsstatus.SPELP=1 as
     * noise that must be ignored while V=0; keep mstatus.SPELP=0. */
    menvcfg_set(MENVCFG_LPE);
    mseccfg_set(MSECCFG_MLPE);
    vsstatus_write(vsstatus_read() | VSSTATUS_SPELP_BIT);
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_SPELP_BIT) : "memory");

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* HS-mode (V=0): xLPE = menvcfg.LPE = 1, so an indirect jump to a
     * non-LPAD target must fault regardless of vsstatus.SPELP=1. */
    goto_priv(PRIV_S);
    trap_expect_begin();
    asm volatile(
        "jalr ra, %0, 0\n\t"
        :
        : "r"(code)
        : "ra", "memory"
    );
    trap_expect_end();

    TEST_ASSERT("HS-mode LP fault works with vsstatus.SPELP=1 present",
                trap_was_triggered() &&
                trap_get_cause() == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("tval = landing pad fault (2)",
                   trap_get_tval(), (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    /* HS-mode indirect jump to an LPAD target succeeds */
    emit_lpad(code);
    emit_ret(code + 1);
    flush_cfi_code();

    trap_expect_begin();
    asm volatile(
        "jalr ra, %0, 0\n\t"
        :
        : "r"(code)
        : "ra", "memory"
    );
    trap_expect_end();

    TEST_ASSERT("HS-mode jump to LPAD succeeds",
                !trap_was_triggered());

    /* vsstatus.SPELP must be untouched by the V=0 activity */
    TEST_ASSERT("vsstatus.SPELP unchanged by V=0 activity",
                (vsstatus_read() & VSSTATUS_SPELP_BIT) != 0);

    goto_priv(PRIV_M);

    /* Clear vsstatus.SPELP */
    vsstatus_write(vsstatus_read() & ~VSSTATUS_SPELP_BIT);
    menvcfg_clear(MENVCFG_LPE);
    mseccfg_clear(MSECCFG_MLPE);
    HYP_TEST_END();
}
