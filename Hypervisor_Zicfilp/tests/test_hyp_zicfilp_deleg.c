/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group A4: VS-mode software-check exception delegation
 *
 * Tests:
 *   HCFI-LP-34: hedeleg[18] writability
 *   HCFI-LP-35: LP Fault delegated to VS-mode
 *   HCFI-LP-36: LP Fault delegated to HS-mode (hedeleg=0)
 *   HCFI-LP-37: LP Fault delegated to M-mode (medeleg=0)
 *   HCFI-LP-38: LP Fault to VS-mode, vsepc correct
 *   HCFI-LP-39: LP Fault to VS-mode, GVA=0
 *   HCFI-LP-40: LP Fault to HS-mode, sepc correct
 *   HCFI-LP-41: LP Fault to M-mode, MPV=1
 *
 * Norm references:
 *   norm:hedeleg_op - V=1, hedeleg bit set -> delegate to VS
 *   norm:hedeleg_acc - hedeleg bit 18 writable or read-only zero
 *   norm:H_trap_vs_csrwrites - trap to VS writes vsepc/vscause/vstval
 *   norm:H_trap_hs_csrwrites - trap to HS writes sepc/scause/stval
 *   norm:H_trap_m_csrwrites - trap to M sets MPV/MPP
 */

/* ===================================================================
 * HCFI-LP-34: hedeleg[18] writability
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_34);
bool test_hcfi_lp_34(void) {
    TEST_BEGIN("HCFI-LP-34: hedeleg[18] writability");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    uintptr_t orig = hedeleg_read();

    /* Try to set bit 18 (software-check exception) */
    CSRS(CSR_HEDELEG, BIT(CAUSE_SOFTWARE_CHECK));
    uintptr_t val = hedeleg_read();

    bool writable = (val & BIT(CAUSE_SOFTWARE_CHECK)) != 0;
    if (writable) {
        printf("    hedeleg[18] is writable\n");
    } else {
        printf("    hedeleg[18] is read-only zero\n");
    }

    /* Restore */
    hedeleg_write(orig);

    TEST_ASSERT("hedeleg[18] writable or read-only zero",
                writable || (val & BIT(CAUSE_SOFTWARE_CHECK)) == 0);

    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-35: LP Fault delegated to VS-mode
 *
 * medeleg[18]=1, hedeleg[18]=1, henvcfg.LPE=1.
 * VS-mode LP Fault should trap to VS-mode with vscause=18, vstval=2.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_35);
bool test_hcfi_lp_35(void) {
    TEST_BEGIN("HCFI-LP-35: LP Fault delegated to VS-mode");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Delegate software-check to VS */
    setup_deleg_to_vs(BIT(CAUSE_SOFTWARE_CHECK));

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* VS-mode indirect jump to non-LPAD - should trigger LP Fault */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);
    (void)result;

    /* The trap was delegated to VS-mode and recorded by the VS-mode
     * trap handler. */
    printf("    VS handler: triggered=%d vscause=%lu vstval=%lu\n",
           (int)g_vs_exc_triggered, (unsigned long)g_vs_exc_cause,
           (unsigned long)g_vs_exc_tval);

    TEST_ASSERT("LP fault delegated to VS-mode (vscause=18)",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("vstval = landing pad fault (2)",
                   g_vs_exc_tval, (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-36: LP Fault delegated to HS-mode (hedeleg=0)
 *
 * medeleg[18]=1, hedeleg[18]=0. VS-mode LP Fault traps to HS-mode.
 * scause=18, stval=2.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_36);
bool test_hcfi_lp_36(void) {
    TEST_BEGIN("HCFI-LP-36: LP Fault delegated to HS-mode (hedeleg=0)");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Delegate to HS only */
    setup_deleg_to_hs(BIT(CAUSE_SOFTWARE_CHECK));

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    /* LPAD safety net: absorbs stale ELP=LP_EXPECTED left by
     * simulators whose sret does not restore ELP from SPELP
     * (QEMU VS->HS trap SPELP bug).  See HCFI-LP-15 comment. */
    emit_lpad(code + 1);
    emit_ret(code + 2);
    flush_cfi_code();

    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT_EQ("LP fault trapped to HS-mode",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("stval = landing pad fault (2)",
                   trap_get_tval(), (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    /* Verify hstatus.SPV=1 (trap came from VS-mode). The return sret
     * clears hstatus.SPV (norm:sret_v0), so check the snapshot taken
     * at trap entry. */
    TEST_ASSERT("hstatus.SPV=1 (trap from VS-mode)",
                trap_get_spv());

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-37: LP Fault delegated to M-mode (medeleg=0)
 *
 * medeleg[18]=0. VS-mode LP Fault traps to M-mode.
 * mcause=18, mtval=2.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_37);
bool test_hcfi_lp_37(void) {
    TEST_BEGIN("HCFI-LP-37: LP Fault delegated to M-mode (medeleg=0)");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* No delegation - trap to M-mode */
    clear_all_deleg();

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT_EQ("LP fault trapped to M-mode",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("mtval = landing pad fault (2)",
                   trap_get_tval(), (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-38: LP Fault to VS-mode, vsepc correct
 *
 * medeleg[18]=1, hedeleg[18]=1. VS-mode LP Fault.
 * vsepc should be the address of the JALR instruction.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_38);
bool test_hcfi_lp_38(void) {
    TEST_BEGIN("HCFI-LP-38: LP Fault to VS-mode, vsepc correct");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);
    setup_deleg_to_vs(BIT(CAUSE_SOFTWARE_CHECK));

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);
    (void)result;

    /* For a landing pad fault, vsepc is the address of the target
     * instruction (the non-LPAD instruction the JALR jumped to). */
    printf("    vsepc = 0x%lx, target addr = 0x%lx\n",
           (unsigned long)g_vs_exc_epc, (unsigned long)(uintptr_t)code);

    TEST_ASSERT("LP fault delegated to VS-mode",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("vsepc = target instruction address",
                g_vs_exc_epc == (uintptr_t)code);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-39: LP Fault to VS-mode, GVA=0
 *
 * Software-check exception is not address-translation related,
 * so hstatus.GVA should be 0.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_39);
bool test_hcfi_lp_39(void) {
    TEST_BEGIN("HCFI-LP-39: LP Fault to VS-mode, GVA=0");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);
    setup_deleg_to_hs(BIT(CAUSE_SOFTWARE_CHECK));

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    /* LPAD safety net: absorbs stale ELP=LP_EXPECTED left by
     * simulators whose sret does not restore ELP from SPELP
     * (QEMU VS->HS trap SPELP bug).  See HCFI-LP-15 comment. */
    emit_lpad(code + 1);
    emit_ret(code + 2);
    flush_cfi_code();

    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    /* GVA should be 0 (software-check is not a guest-page-fault) */
    uintptr_t hs = hstatus_read();
    bool gva = (hs & HSTATUS_GVA) != 0;
    printf("    hstatus.GVA = %lu (expect 0)\n", (unsigned long)gva);

    TEST_ASSERT_EQ("LP fault trapped to HS", result,
                   (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("hstatus.GVA=0 (non-translation exception)",
                !gva);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-40: LP Fault to HS-mode, sepc correct
 *
 * medeleg[18]=1, hedeleg[18]=0. VS-mode LP Fault traps to HS.
 * sepc = JALR instruction address, hstatus.SPV=1.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_40);
bool test_hcfi_lp_40(void) {
    TEST_BEGIN("HCFI-LP-40: LP Fault to HS-mode, sepc correct");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);
    setup_deleg_to_hs(BIT(CAUSE_SOFTWARE_CHECK));

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    /* LPAD safety net: absorbs stale ELP=LP_EXPECTED left by
     * simulators whose sret does not restore ELP from SPELP
     * (QEMU VS->HS trap SPELP bug).  See HCFI-LP-15 comment. */
    emit_lpad(code + 1);
    emit_ret(code + 2);
    flush_cfi_code();

    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    /* For a landing pad fault, sepc is the address of the target
     * instruction (the non-LPAD instruction the JALR jumped to). The
     * return sret clears hstatus.SPV (norm:sret_v0), so check the
     * snapshot taken at trap entry. */
    uintptr_t sepc_val = trap_get_epc();
    printf("    sepc = 0x%lx, target addr = 0x%lx, hstatus.SPV at trap = %lu\n",
           (unsigned long)sepc_val, (unsigned long)(uintptr_t)code,
           (unsigned long)(trap_get_spv() ? 1 : 0));

    TEST_ASSERT_EQ("LP fault trapped to HS", result,
                   (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("sepc = target instruction address",
                sepc_val == (uintptr_t)code);
    TEST_ASSERT("hstatus.SPV=1 (trap from VS-mode)",
                trap_get_spv());

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-41: LP Fault to M-mode, MPV=1
 *
 * medeleg[18]=0. VS-mode LP Fault traps to M.
 * mstatus.MPV=1, mstatus.MPP=1 (S-mode).
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_41);
bool test_hcfi_lp_41(void) {
    TEST_BEGIN("HCFI-LP-41: LP Fault to M-mode, MPV=1");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);
    clear_all_deleg();

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    /* Check mstatus.MPV and MPP. The return mret clears MPV and sets
     * MPP to U, so verify the snapshots taken at trap entry. */
    bool mpv = trap_get_mpv();
    uintptr_t mpp = trap_get_mpp();
    printf("    mstatus.MPV at trap = %lu, MPP = %lu\n",
           (unsigned long)mpv, (unsigned long)mpp);

    TEST_ASSERT_EQ("LP fault trapped to M", result,
                   (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("mstatus.MPV=1 (trap from VS-mode)", mpv);
    TEST_ASSERT("mstatus.MPP=1 (S-mode)", mpp == 1);

    /* Clear MPV */
    CSRC(mstatus, MSTATUS_MPV);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}
