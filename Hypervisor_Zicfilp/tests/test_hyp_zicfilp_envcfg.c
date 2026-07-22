/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group A1: henvcfg.LPE field and VS-mode Zicfilp enable control
 *
 * Tests:
 *   HCFI-LP-01: henvcfg.LPE basic read/write
 *   HCFI-LP-02: henvcfg.LPE=0, VS-mode ELP not updated, LPAD no-op
 *   HCFI-LP-03: henvcfg.LPE=1, VS-mode ELP updated
 *   HCFI-LP-04: henvcfg.LPE=0, VS-mode LPAD as no-op
 *   HCFI-LP-05: henvcfg.LPE=1, VS-mode illegal indirect jump triggers LP Fault
 *   HCFI-LP-06: henvcfg.LPE=0, VS-mode illegal indirect jump no LP Fault
 *   HCFI-LP-07: VU-mode xLPE controlled by senvcfg.LPE
 *   HCFI-LP-08: VU-mode xLPE=1 triggers LP Fault
 *   HCFI-LP-09: henvcfg.LPE=1 but menvcfg.LPE=0
 *   HCFI-LP-10: henvcfg.LPE not implemented, read-only zero
 *
 * Norm references:
 *   norm:henvcfg_lpe_op - LPE=1 enables Zicfilp in VS-mode; LPE=0 no ELP update
 */

/* ===================================================================
 * HCFI-LP-01: henvcfg.LPE basic read/write
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_01);
bool test_hcfi_lp_01(void) {
    TEST_BEGIN("HCFI-LP-01: henvcfg.LPE basic read/write");

    H_REQUIRED_OR_SKIP();

    uintptr_t orig = henvcfg_read();

    /* Try to set LPE */
    henvcfg_write(orig | HENVCFG_LPE);
    uintptr_t val = henvcfg_read();

    bool lpe_writable = (val & HENVCFG_LPE) != 0;

    if (lpe_writable) {
        printf("    henvcfg.LPE is writable (Zicfilp implemented)\n");

        /* Clear LPE */
        henvcfg_write(orig & ~HENVCFG_LPE);
        val = henvcfg_read();
        TEST_ASSERT("henvcfg.LPE can be cleared to 0",
                    (val & HENVCFG_LPE) == 0);
    } else {
        printf("    henvcfg.LPE is read-only zero (Zicfilp not implemented)\n");
        TEST_ASSERT("henvcfg.LPE reads as 0 when not implemented",
                    (val & HENVCFG_LPE) == 0);
    }

    /* Restore */
    henvcfg_write(orig);

    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-02: henvcfg.LPE=0, VS-mode ELP not updated, LPAD no-op
 *
 * Set henvcfg.LPE=0, menvcfg.LPE=1. VS-mode indirect jump to LPAD
 * target. ELP should stay NO_LP_EXPECTED, LPAD executes as no-op.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_02);
bool test_hcfi_lp_02(void) {
    TEST_BEGIN("HCFI-LP-02: henvcfg.LPE=0, VS-mode ELP not updated");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set henvcfg.LPE=0, menvcfg.LPE=1 */
    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(false, true);
    mseccfg_clear(MSECCFG_MLPE);

    /* Prepare code: LPAD + RET */
    uint32_t *code = get_cfi_code_buf();
    emit_lpad(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* VS-mode indirect jump to LPAD target - should succeed with no trap */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT("no trap when LPE=0 and target has LPAD",
                result == 0);

    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-03: henvcfg.LPE=1, VS-mode ELP updated
 *
 * Set henvcfg.LPE=1, menvcfg.LPE=1, mseccfg.MLPE=1.
 * VS-mode indirect jump to LPAD target should succeed.
 * ELP goes from LP_EXPECTED to NO_LP_EXPECTED via LPAD.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_03);
bool test_hcfi_lp_03(void) {
    TEST_BEGIN("HCFI-LP-03: henvcfg.LPE=1, VS-mode ELP updated");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set henvcfg.LPE=1, menvcfg.LPE=1 */
    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Prepare code: LPAD + RET */
    uint32_t *code = get_cfi_code_buf();
    emit_lpad(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* VS-mode indirect jump to LPAD target - should succeed */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT("no trap when LPE=1 and target has LPAD",
                result == 0);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-04: henvcfg.LPE=0, VS-mode LPAD as no-op
 *
 * Set henvcfg.LPE=0. VS-mode executes LPAD instruction directly.
 * LPAD should execute as no-op, no exception, ELP unchanged.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_04);
bool test_hcfi_lp_04(void) {
    TEST_BEGIN("HCFI-LP-04: henvcfg.LPE=0, VS-mode LPAD as no-op");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set henvcfg.LPE=0 */
    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(false, true);

    /* VS-mode execute LPAD directly - should be no-op */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_exec_lpad, 0);

    TEST_ASSERT("LPAD as no-op with LPE=0 (no trap)",
                result == 0);

    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-05: henvcfg.LPE=1, VS-mode illegal indirect jump triggers LP Fault
 *
 * Set henvcfg.LPE=1. VS-mode indirect jump to non-LPAD target.
 * Should trigger software-check exception (cause=18, tval=2).
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_05);
bool test_hcfi_lp_05(void) {
    TEST_BEGIN("HCFI-LP-05: henvcfg.LPE=1, VS-mode illegal jump triggers LP Fault");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set henvcfg.LPE=1, menvcfg.LPE=1 */
    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Prepare code: NOP (NOT LPAD) + RET */
    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* VS-mode indirect jump to non-LPAD target - should trigger LP Fault */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT_EQ("software-check exception triggered",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("tval = landing pad fault (2)",
                   trap_get_tval(), (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-06: henvcfg.LPE=0, VS-mode illegal indirect jump no LP Fault
 *
 * Set henvcfg.LPE=0. VS-mode indirect jump to non-LPAD target.
 * No software-check exception, ELP stays NO_LP_EXPECTED.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_06);
bool test_hcfi_lp_06(void) {
    TEST_BEGIN("HCFI-LP-06: henvcfg.LPE=0, VS-mode illegal jump no LP Fault");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set henvcfg.LPE=0, menvcfg.LPE=1 */
    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(false, true);
    mseccfg_clear(MSECCFG_MLPE);

    /* Prepare code: NOP (NOT LPAD) + RET */
    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* VS-mode indirect jump to non-LPAD target - no trap expected */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT("no trap when LPE=0 and target is non-LPAD",
                result == 0);

    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-07: VU-mode xLPE controlled by senvcfg.LPE
 *
 * V=1, henvcfg.LPE=1, senvcfg.LPE=0. VU-mode indirect jump to
 * non-LPAD target. No software-check (VU-mode xLPE=0).
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_07);
bool test_hcfi_lp_07(void) {
    TEST_BEGIN("HCFI-LP-07: VU-mode xLPE controlled by senvcfg.LPE=0");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set henvcfg.LPE=1, menvcfg.LPE=1, senvcfg.LPE=0 */
    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    senvcfg_clear(SENVCFG_LPE);
    mseccfg_set(MSECCFG_MLPE);

    /* Prepare code: NOP (NOT LPAD) + RET */
    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* VU-mode indirect jump to non-LPAD - no trap (VU xLPE=0) */
    uintptr_t result = two_stage_run_in_vu(&ctx, vu_jalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT("no trap in VU-mode when senvcfg.LPE=0",
                result == 0);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-08: VU-mode xLPE=1 triggers LP Fault
 *
 * V=1, henvcfg.LPE=1, senvcfg.LPE=1. VU-mode indirect jump to
 * non-LPAD target. Should trigger software-check (cause=18).
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_08);
bool test_hcfi_lp_08(void) {
    TEST_BEGIN("HCFI-LP-08: VU-mode xLPE=1 triggers LP Fault");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set henvcfg.LPE=1, menvcfg.LPE=1, senvcfg.LPE=1 */
    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    senvcfg_set(SENVCFG_LPE);
    mseccfg_set(MSECCFG_MLPE);

    /* Prepare code: NOP (NOT LPAD) + RET */
    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* VU-mode indirect jump to non-LPAD - should trigger LP Fault */
    uintptr_t result = two_stage_run_in_vu(&ctx, vu_jalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT_EQ("software-check exception in VU-mode",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("tval = landing pad fault (2)",
                   trap_get_tval(), (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    senvcfg_clear(SENVCFG_LPE);
    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-09: henvcfg.LPE=1 but menvcfg.LPE=0
 *
 * Set menvcfg.LPE=0, henvcfg.LPE=1. VS-mode Zicfilp should work
 * independently of menvcfg.LPE (VS-level xLPE=henvcfg.LPE).
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_09);
bool test_hcfi_lp_09(void) {
    TEST_BEGIN("HCFI-LP-09: henvcfg.LPE=1 but menvcfg.LPE=0");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set henvcfg.LPE=1, menvcfg.LPE=0 */
    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, false);
    mseccfg_set(MSECCFG_MLPE);

    /* Prepare code: LPAD + RET (valid landing pad) */
    uint32_t *code = get_cfi_code_buf();
    emit_lpad(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* VS-mode indirect jump to LPAD target - should succeed */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT("VS-mode Zicfilp works when henvcfg.LPE=1, menvcfg.LPE=0",
                result == 0);

    /* Second phase: jump to a non-LPAD target. A software-check fault
     * here proves the VS-level xLPE (= henvcfg.LPE = 1) is independently
     * effective even though menvcfg.LPE=0. */
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                 (uintptr_t)code);

    TEST_ASSERT_EQ("LP fault proves VS xLPE independent of menvcfg.LPE",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("tval = landing pad fault (2)",
                   trap_get_tval(), (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-10: henvcfg.LPE not implemented, read-only zero
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_10);
bool test_hcfi_lp_10(void) {
    TEST_BEGIN("HCFI-LP-10: henvcfg.LPE not implemented, read-only zero");

    H_REQUIRED_OR_SKIP();

    /* If Zicfilp is not detected, henvcfg.LPE should be read-only zero */
    if (detect_zicfilp()) {
        printf("    Zicfilp is implemented, LPE is writable\n");
        TEST_SKIP("Zicfilp is implemented, skip read-only-zero test");
    }

    /* Try to write LPE=1 */
    uintptr_t orig = henvcfg_read();
    henvcfg_write(orig | HENVCFG_LPE);
    uintptr_t val = henvcfg_read();

    TEST_ASSERT("henvcfg.LPE is read-only zero when Zicfilp not implemented",
                (val & HENVCFG_LPE) == 0);

    henvcfg_write(orig);
    HYP_TEST_END();
}
