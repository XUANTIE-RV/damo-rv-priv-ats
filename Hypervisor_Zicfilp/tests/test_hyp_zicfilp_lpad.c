/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group A3: VS-mode Landing Pad functionality and software-check exception
 *
 * Tests:
 *   HCFI-LP-25: VS-mode legal indirect call to LPAD target
 *   HCFI-LP-26: VS-mode illegal indirect call triggers LP Fault
 *   HCFI-LP-27: VS-mode legal indirect jump (non-call) to LPAD target
 *   HCFI-LP-28: VS-mode illegal indirect jump triggers LP Fault
 *   HCFI-LP-29: VS-mode LP Fault vs instruction access-fault priority
 *   HCFI-LP-30: VS-mode LP Fault vs illegal-instruction priority
 *   HCFI-LP-31: VS-mode interrupt during indirect jump, ELP saved
 *   HCFI-LP-32: VS-mode C.JALR illegal jump triggers LP Fault
 *   HCFI-LP-33: VS-mode C.JR illegal jump triggers LP Fault
 *
 * Norm references:
 *   norm:lpad_sw_exception - ELP=LP_EXPECTED, non-LPAD target -> sw-check
 *   norm:Zicfilp_exception_priority - sw-check > illegal-inst, < inst access-fault
 *   norm:Zicfilp_forward_traps - ELP preserved on trap during indirect jump
 */

/* ===================================================================
 * HCFI-LP-25: VS-mode legal indirect call to LPAD target
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_25);
bool test_hcfi_lp_25(void) {
    TEST_BEGIN("HCFI-LP-25: VS-mode legal indirect call to LPAD target");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    uint32_t *code = get_cfi_code_buf();
    emit_lpad(code);
    emit_ret(code + 1);
    flush_cfi_code();

    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT("legal indirect call to LPAD succeeds",
                result == 0);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-26: VS-mode illegal indirect call triggers LP Fault
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_26);
bool test_hcfi_lp_26(void) {
    TEST_BEGIN("HCFI-LP-26: VS-mode illegal indirect call triggers LP Fault");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

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
 * HCFI-LP-27: VS-mode legal indirect jump (non-call) to LPAD target
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_27);
bool test_hcfi_lp_27(void) {
    TEST_BEGIN("HCFI-LP-27: VS-mode legal indirect jump to LPAD target");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    uint32_t *code = get_cfi_code_buf();
    emit_lpad(code);
    emit_ret(code + 1);
    flush_cfi_code();

    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT("legal indirect jump to LPAD succeeds",
                result == 0);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-28: VS-mode illegal indirect jump triggers LP Fault
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_28);
bool test_hcfi_lp_28(void) {
    TEST_BEGIN("HCFI-LP-28: VS-mode illegal indirect jump triggers LP Fault");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT_EQ("software-check exception triggered (non-call jump)",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("tval = landing pad fault (2)",
                   trap_get_tval(), (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-29: VS-mode LP Fault vs instruction access-fault priority
 *
 * instruction access-fault has higher priority than software-check.
 * VS-mode indirect jump to inaccessible address should report
 * instruction access-fault, not software-check.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_29);
bool test_hcfi_lp_29(void) {
    TEST_BEGIN("HCFI-LP-29: LP Fault vs instruction access-fault priority");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Jump to an unmapped address. The JALR sets ELP=LP_EXPECTED, but
     * the instruction fetch fault (page/access fault) has higher
     * priority than the software-check (norm:Zicfilp_exception_priority).
     * Uses a recovery-aware trampoline since the faulting PC itself is
     * unmappable and cannot be skipped past. */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_unmapped,
                                           (uintptr_t)0x0);

    /* Should get instruction access-fault or page fault, not software-check.
     * The exact cause depends on whether the address is mapped at G-stage. */
    printf("    trap cause = %lu\n", (unsigned long)result);
    TEST_ASSERT("trap was triggered",
                result != 0);
    TEST_ASSERT("instruction access/page fault (not software-check)",
                result != CAUSE_SOFTWARE_CHECK);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-30: VS-mode LP Fault vs illegal-instruction priority
 *
 * software-check has higher priority than illegal-instruction.
 * VS-mode indirect jump to target containing illegal instruction
 * should report software-check, not illegal-instruction.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_30);
bool test_hcfi_lp_30(void) {
    TEST_BEGIN("HCFI-LP-30: LP Fault vs illegal-instruction priority");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    /* Prepare code with an illegal instruction (not LPAD) */
    uint32_t *code = get_cfi_code_buf();
    *code = 0x00000000;   /* all-zeros is illegal */
    emit_ret(code + 1);
    flush_cfi_code();

    /* Per norm:Zicfilp_exception_priority, software-check exception
     * has higher priority than illegal-instruction. So we expect
     * cause=18 (software-check), not cause=2 (illegal-instruction).
     * Uses a recovery-aware trampoline: the all-zeros word decodes as
     * 16-bit instructions, so the default skip-based recovery would
     * land on another illegal halfword and fault again. */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_unmapped,
                                           (uintptr_t)code);

    TEST_ASSERT_EQ("software-check (not illegal-instruction)",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-31: VS-mode interrupt during indirect jump, ELP saved
 *
 * When an interrupt is delivered after JALR but before LPAD decode,
 * ELP=LP_EXPECTED should be saved to vsstatus.SPELP (trap to VS)
 * or mstatus.SPELP (trap to HS).
 *
 * Note: This is difficult to test deterministically as the interrupt
 * must arrive in the narrow window between JALR and LPAD decode.
 * We verify the mechanism by checking that a delegated interrupt
 * during VS-mode preserves the ELP state.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_31);
bool test_hcfi_lp_31(void) {
    TEST_BEGIN("HCFI-LP-31: interrupt during indirect jump, ELP saved");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    /* KNOWN GAP (proxy verification): injecting an asynchronous
     * interrupt into the narrow JALR->LPAD decode window is
     * non-deterministic and not attempted here. The ELP-save mechanism
     * is instead verified via a synchronous software-check exception
     * (which preserves ELP identically per norm:Zicfilp_pelp_trap). */
    printf("    Note: async interrupt injection not implemented (known gap)\n");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);
    setup_deleg_to_hs(BIT(CAUSE_SOFTWARE_CHECK));

    /* Clear mstatus.SPELP */
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_SPELP_BIT) : "memory");

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    /* LPAD safety net: absorbs stale ELP=LP_EXPECTED left by
     * simulators whose sret does not restore ELP from SPELP
     * (QEMU VS->HS trap SPELP bug).  See HCFI-LP-15 comment. */
    emit_lpad(code + 1);
    emit_ret(code + 2);
    flush_cfi_code();

    /* VS-mode indirect jump to non-LPAD - triggers LP fault.
     * At trap entry, SPELP should be set to LP_EXPECTED. */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_jalr_to_target,
                                           (uintptr_t)code);

    /* The HS-mode handler clears SPELP before sret for clean recovery,
     * so verify against the status snapshot taken at trap entry. */
    uintptr_t snap = trap_get_status_snap();
    uintptr_t spelp = (snap & MSTATUS_SPELP_BIT) ? 1 : 0;
    printf("    mstatus.SPELP = %lu (ELP was saved at trap)\n",
           (unsigned long)spelp);

    TEST_ASSERT_EQ("LP fault triggered", result,
                   (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("SPELP saved ELP=LP_EXPECTED at trap entry",
                spelp == 1);

    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_SPELP_BIT) : "memory");
    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    clear_all_deleg();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-32: VS-mode C.JALR illegal jump triggers LP Fault
 *
 * C.JALR (compressed indirect jump with link) to non-LPAD target
 * should trigger software-check exception when LPE=1.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_32);
bool test_hcfi_lp_32(void) {
    TEST_BEGIN("HCFI-LP-32: VS-mode C.JALR illegal jump triggers LP Fault");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* Use C.JALR (compressed jalr ra, rs1, 0 = 0x9002) */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_cjalr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT_EQ("software-check from C.JALR",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("tval = landing pad fault (2)",
                   trap_get_tval(), (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * HCFI-LP-33: VS-mode C.JR illegal jump triggers LP Fault
 *
 * C.JR (compressed indirect jump without link) to non-LPAD target
 * should trigger software-check exception when LPE=1.
 * =================================================================== */
TEST_REGISTER(test_hcfi_lp_33);
bool test_hcfi_lp_33(void) {
    TEST_BEGIN("HCFI-LP-33: VS-mode C.JR illegal jump triggers LP Fault");

    H_REQUIRED_OR_SKIP();
    ZICFILP_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_henvcfg = cfi_setup_vs_lpe(true, true);
    mseccfg_set(MSECCFG_MLPE);

    uint32_t *code = get_cfi_code_buf();
    emit_nop(code);
    emit_ret(code + 1);
    flush_cfi_code();

    /* Use C.JR (compressed jr rs1, 0 = 0x8002) */
    uintptr_t result = two_stage_run_in_vs(&ctx, vs_cjr_to_target,
                                           (uintptr_t)code);

    TEST_ASSERT_EQ("software-check from C.JR",
                   result, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("tval = landing pad fault (2)",
                   trap_get_tval(), (uintptr_t)SWCHECK_LANDING_PAD_FAULT);

    mseccfg_clear(MSECCFG_MLPE);
    cfi_restore_henvcfg(orig_henvcfg);
    ts2_finish(&ctx);
    HYP_TEST_END();
}
