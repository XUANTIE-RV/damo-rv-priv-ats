/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group B4: G-stage translation and Shadow Stack interaction
 *
 * Tests HCFI-SS-37~42: G-stage pte.xwr=010 reserved, RW permission,
 * read-only/none permission guest-page-fault behavior.
 *
 * Norm references:
 *   norm:active_g_stage_pte - G-stage needs RW for SS instructions
 */

/* HCFI-SS-37: G-stage pte.xwr=010 stays reserved */
TEST_REGISTER(test_hcfi_ss_37);
bool test_hcfi_ss_37(void) {
    TEST_BEGIN("HCFI-SS-37: G-stage pte.xwr=010 reserved -> guest-page-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    /* Set VS-stage to SS page, but G-stage to xwr=010 (reserved in G-stage) */
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);
    g_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS | PTE_U);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT_EQ("guest-page-fault for G-stage pte.xwr=010",
                   r, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-38: G-stage RW permission, SS instruction normal */
TEST_REGISTER(test_hcfi_ss_38);
bool test_hcfi_ss_38(void) {
    TEST_BEGIN("HCFI-SS-38: G-stage RW, SS instruction normal");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    /* VS-stage: SS page, G-stage: RW (default from ts2_setup_full) */
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH with G-stage RW succeeds", r == 0);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-39: G-stage read-only, SS instruction -> guest-page-fault */
TEST_REGISTER(test_hcfi_ss_39);
bool test_hcfi_ss_39(void) {
    TEST_BEGIN("HCFI-SS-39: G-stage read-only, SS -> guest-page-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);
    g_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_V | PTE_R | PTE_U | PTE_A | PTE_D);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT_EQ("store/AMO guest-page-fault for G-stage read-only",
                   r, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-40: G-stage no permission, SS -> guest-page-fault */
TEST_REGISTER(test_hcfi_ss_40);
bool test_hcfi_ss_40(void) {
    TEST_BEGIN("HCFI-SS-40: G-stage no permission, SS -> guest-page-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);
    /* G-stage: invalid page */
    g_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, 0);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT_EQ("guest-page-fault for G-stage no permission",
                   r, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-41: G-stage guest-page-fault delegation */
TEST_REGISTER(test_hcfi_ss_41);
bool test_hcfi_ss_41(void) {
    TEST_BEGIN("HCFI-SS-41: G-stage guest-page-fault traps to HS, GVA=1");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);
    /* Delegate store guest-page-fault to HS (medeleg only; hedeleg[23]
     * is read-only zero per the H extension, so it cannot go to VS). */
    setup_deleg_to_hs(BIT(CAUSE_STORE_GUEST_PAGE_FAULT));

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);
    g_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, 0);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT_EQ("guest-page-fault trapped to HS",
                   r, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    /* GVA should be 1 for guest-page-fault, and the trap must have
     * come from VS-mode (hstatus.SPV=1 at trap entry). */
    TEST_ASSERT("hstatus.GVA=1 for guest-page-fault", trap_get_gva());
    TEST_ASSERT("hstatus.SPV=1 (trap from VS-mode)",
                trap_get_spv());

    cfi_restore_henvcfg(orig_h); clear_all_deleg(); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-42: SSPOPCHK triggers G-stage guest-page-fault */
TEST_REGISTER(test_hcfi_ss_42);
bool test_hcfi_ss_42(void) {
    TEST_BEGIN("HCFI-SS-42: SSPOPCHK G-stage guest-page-fault (store type)");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);
    clear_all_deleg();

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);
    g_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_V | PTE_R | PTE_U | PTE_A | PTE_D);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    /* SSPOPCHK should report as store/AMO guest-page-fault, not load */
    TEST_ASSERT_EQ("store/AMO guest-page-fault from SSPOPCHK",
                   r, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}
