/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group B5: satp/vsatp Bare mode Shadow Stack behavior
 *
 * Tests HCFI-SS-43~48: vsatp.mode=Bare, SS instructions trigger access-fault
 *
 * Norm references:
 *   norm:satp_mode_bare - Bare mode + <M -> store/AMO access-fault
 */

/* HCFI-SS-43: vsatp.mode=Bare, VS-mode SSPUSH -> access-fault */
TEST_REGISTER(test_hcfi_ss_43);
bool test_hcfi_ss_43(void) {
    TEST_BEGIN("HCFI-SS-43: vsatp.mode=Bare, VS SSPUSH -> access-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    /* Use Bare VS-stage (no VS-stage page tables) */
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT_EQ("store/AMO access-fault in Bare mode",
                   r, (uintptr_t)CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-44: vsatp.mode=Bare, VS-mode SSPOPCHK -> access-fault */
TEST_REGISTER(test_hcfi_ss_44);
bool test_hcfi_ss_44(void) {
    TEST_BEGIN("HCFI-SS-44: vsatp.mode=Bare, VS SSPOPCHK -> access-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    TEST_ASSERT_EQ("store/AMO access-fault in Bare mode",
                   r, (uintptr_t)CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-45: vsatp.mode=Bare, VS-mode SSAMOSWAP -> access-fault */
TEST_REGISTER(test_hcfi_ss_45);
bool test_hcfi_ss_45(void) {
    TEST_BEGIN("HCFI-SS-45: vsatp.mode=Bare, VS SSAMOSWAP -> access-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_ssamoswap_w, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("store/AMO access-fault for SSAMOSWAP in Bare mode",
                   r, (uintptr_t)CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-46: vsatp.mode=Bare, VU-mode SS -> access-fault */
TEST_REGISTER(test_hcfi_ss_46);
bool test_hcfi_ss_46(void) {
    TEST_BEGIN("HCFI-SS-46: vsatp.mode=Bare, VU SSPUSH -> access-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    senvcfg_set(SENVCFG_SSE);
    ssp_write(SS_PAGE_ADDR + 0x100);

    uintptr_t r = two_stage_run_in_vu(&ctx, vu_exec_sspush, 0);
    TEST_ASSERT_EQ("store/AMO access-fault in VU Bare mode",
                   r, (uintptr_t)CAUSE_STORE_ACCESS_FAULT);

    senvcfg_clear(SENVCFG_SSE);
    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-47: vsatp.mode non-Bare, SS instructions normal */
TEST_REGISTER(test_hcfi_ss_47);
bool test_hcfi_ss_47(void) {
    TEST_BEGIN("HCFI-SS-47: vsatp.mode=Sv39, SS instructions normal");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH with Sv39 succeeds (no Bare-mode fault)", r == 0);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-48: vsatp.mode=Bare, ssp CSR still accessible */
TEST_REGISTER(test_hcfi_ss_48);
bool test_hcfi_ss_48(void) {
    TEST_BEGIN("HCFI-SS-48: vsatp.mode=Bare, ssp CSR still accessible");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_read_ssp, 0);
    TEST_ASSERT("ssp CSR accessible in Bare mode", r == 0);

    r = two_stage_run_in_vs(&ctx, vs_write_ssp, 0xDEAD);
    TEST_ASSERT("ssp CSR writable in Bare mode", r == 0);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}
