/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group B2: ssp CSR access control in VS/VU-mode
 *
 * Tests:
 *   HCFI-SS-15: VS-mode + henvcfg.SSE=0, ssp access -> virtual-inst
 *   HCFI-SS-16: VS-mode + henvcfg.SSE=1 + menvcfg.SSE=1, ssp normal
 *   HCFI-SS-17: VS-mode + menvcfg.SSE=0, ssp access -> illegal-inst
 *   HCFI-SS-18: VU-mode + henvcfg.SSE=0, ssp access -> virtual-inst
 *   HCFI-SS-19: VU-mode + senvcfg.SSE=0, ssp access -> virtual-inst
 *   HCFI-SS-20: VU-mode + all enable, ssp normal
 *   HCFI-SS-21: VU-mode + menvcfg.SSE=0, ssp access -> illegal-inst
 *   HCFI-SS-22: VS-mode write ssp, HS-mode can read
 *   HCFI-SS-23: VS-mode ssp alignment check
 *
 * Norm references:
 *   norm:zicfiss_ssp_csr - ssp access controlled by envcfg sse fields
 *   norm:zicfiss_m_menvcfg_sse - <M + menvcfg.SSE=0 -> illegal-inst
 *   norm:zicfiss_vs_henvcfg_sse - VS + henvcfg.SSE=0 -> virtual-inst
 *   norm:zicfiss_vu_henvcfg_senvcfg_sse - VU + henvcfg/senvcfg.SSE=0 -> virtual-inst
 *   norm:zicfiss_sse_access - otherwise access allowed
 */

/* HCFI-SS-15: VS-mode + henvcfg.SSE=0 -> virtual-instruction */
TEST_REGISTER(test_hcfi_ss_15);
bool test_hcfi_ss_15(void) {
    TEST_BEGIN("HCFI-SS-15: VS + henvcfg.SSE=0, ssp access -> virtual-inst");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(false, true);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_read_ssp, 0);
    TEST_ASSERT_EQ("virtual-instruction exception", r, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-16: VS-mode + henvcfg.SSE=1 + menvcfg.SSE=1, ssp normal */
TEST_REGISTER(test_hcfi_ss_16);
bool test_hcfi_ss_16(void) {
    TEST_BEGIN("HCFI-SS-16: VS + all SSE=1, ssp access normal");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_write_ssp, 0xCAFE);
    TEST_ASSERT("ssp write succeeds (SSE=1)", r == 0);

    r = two_stage_run_in_vs(&ctx, vs_read_ssp, 0);
    TEST_ASSERT("ssp read succeeds (SSE=1)", r == 0);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-17: VS-mode + menvcfg.SSE=0 -> illegal-instruction */
TEST_REGISTER(test_hcfi_ss_17);
bool test_hcfi_ss_17(void) {
    TEST_BEGIN("HCFI-SS-17: VS + menvcfg.SSE=0, ssp access -> illegal-inst");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, false);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_read_ssp, 0);
    TEST_ASSERT_EQ("illegal-instruction exception", r, (uintptr_t)CAUSE_ILLEGAL_INST);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-18: VU-mode + henvcfg.SSE=0 -> virtual-instruction */
TEST_REGISTER(test_hcfi_ss_18);
bool test_hcfi_ss_18(void) {
    TEST_BEGIN("HCFI-SS-18: VU + henvcfg.SSE=0, ssp access -> virtual-inst");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(false, true);
    senvcfg_set(SENVCFG_SSE);

    uintptr_t r = two_stage_run_in_vu(&ctx, vu_read_ssp, 0);
    TEST_ASSERT_EQ("virtual-instruction exception", r, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-19: VU-mode + senvcfg.SSE=0 -> virtual-instruction */
TEST_REGISTER(test_hcfi_ss_19);
bool test_hcfi_ss_19(void) {
    TEST_BEGIN("HCFI-SS-19: VU + senvcfg.SSE=0, ssp access -> virtual-inst");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    senvcfg_clear(SENVCFG_SSE);

    uintptr_t r = two_stage_run_in_vu(&ctx, vu_read_ssp, 0);
    TEST_ASSERT_EQ("virtual-instruction exception", r, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    senvcfg_set(SENVCFG_SSE);
    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-20: VU-mode + all enable, ssp normal */
TEST_REGISTER(test_hcfi_ss_20);
bool test_hcfi_ss_20(void) {
    TEST_BEGIN("HCFI-SS-20: VU + all SSE=1, ssp access normal");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    senvcfg_set(SENVCFG_SSE);

    uintptr_t r = two_stage_run_in_vu(&ctx, vu_write_ssp, 0xBEEF);
    TEST_ASSERT("ssp write in VU-mode succeeds", r == 0);

    r = two_stage_run_in_vu(&ctx, vu_read_ssp, 0);
    TEST_ASSERT("ssp read in VU-mode succeeds", r == 0);

    senvcfg_clear(SENVCFG_SSE);
    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-21: VU-mode + menvcfg.SSE=0 -> illegal-instruction */
TEST_REGISTER(test_hcfi_ss_21);
bool test_hcfi_ss_21(void) {
    TEST_BEGIN("HCFI-SS-21: VU + menvcfg.SSE=0, ssp access -> illegal-inst");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, false);

    uintptr_t r = two_stage_run_in_vu(&ctx, vu_read_ssp, 0);
    TEST_ASSERT_EQ("illegal-instruction exception", r, (uintptr_t)CAUSE_ILLEGAL_INST);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-22: VS-mode write ssp, HS-mode can read */
TEST_REGISTER(test_hcfi_ss_22);
bool test_hcfi_ss_22(void) {
    TEST_BEGIN("HCFI-SS-22: VS-mode write ssp, HS-mode reads same value");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    /* VS-mode writes ssp */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_write_ssp, 0x1234);
    TEST_ASSERT("ssp write in VS-mode succeeds", r == 0);

    /* HS-mode reads ssp */
    uintptr_t val = ssp_read();
    TEST_ASSERT_EQ("HS-mode reads ssp value from VS-mode write", val, (uintptr_t)0x1234);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-23: VS-mode ssp alignment check */
TEST_REGISTER(test_hcfi_ss_23);
bool test_hcfi_ss_23(void) {
    TEST_BEGIN("HCFI-SS-23: VS-mode ssp misaligned triggers access-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    /* Set ssp to non-XLEN-aligned value */
    ssp_write(SS_PAGE_ADDR + 1);  /* misaligned */

    /* SSPUSH should trigger store/AMO access-fault */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT_EQ("store/AMO access-fault from misaligned ssp",
                   r, (uintptr_t)CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}
