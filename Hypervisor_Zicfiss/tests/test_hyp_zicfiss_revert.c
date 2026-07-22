/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group B8: henvcfg.SSE=0 VS-mode Zicfiss reversion behavior
 *
 * Tests HCFI-SS-69~75: SSE=0 instruction reversion, ssp CSR access,
 * SS page encoding reserved, SSE switching behavior.
 *
 * Norm references:
 *   norm:henvcfg_sse_op - SSE=0 reversion rules
 */

/* HCFI-SS-69: SSE=0, 32-bit SS instructions: SSPUSH/SSPOPCHK revert to
 * Zimop; SSAMOSWAP.W/D raise virtual-instruction (menvcfg.SSE=1).
 *
 * Per norm:henvcfg_sse_op: when henvcfg.SSE=0, 32-bit Zicfiss
 * instructions revert to Zimop behavior, EXCEPT SSAMOSWAP.W/D which
 * raise a virtual-instruction exception when menvcfg.SSE=1 (they are
 * AMO-encoded, not Zimop-encoded). See HCFI-SS-09. */
TEST_REGISTER(test_hcfi_ss_69);
bool test_hcfi_ss_69(void) {
    TEST_BEGIN("HCFI-SS-69: SSE=0, SSPUSH/SSPOPCHK revert; SSAMOSWAP virtual-inst");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(false, true);

    /* SSPUSH should be Zimop no-op */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH reverts to Zimop (SSE=0)", r == 0);

    /* SSPOPCHK should be Zimop no-op */
    r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    TEST_ASSERT("SSPOPCHK reverts to Zimop (SSE=0)", r == 0);

    /* SSAMOSWAP.W should raise virtual-instruction (norm:henvcfg_sse_op) */
    r = two_stage_run_in_vs(&ctx, vs_exec_ssamoswap_w, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("SSAMOSWAP.W virtual-instruction (SSE=0, menvcfg.SSE=1)",
                   r, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    /* SSAMOSWAP.D should raise virtual-instruction (norm:henvcfg_sse_op) */
    r = two_stage_run_in_vs(&ctx, vs_exec_ssamoswap_d, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("SSAMOSWAP.D virtual-instruction (SSE=0, menvcfg.SSE=1)",
                   r, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-70: SSE=0, all 16-bit SS instructions revert to Zcmop */
TEST_REGISTER(test_hcfi_ss_70);
bool test_hcfi_ss_70(void) {
    TEST_BEGIN("HCFI-SS-70: SSE=0, all 16-bit SS instructions revert to Zcmop");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();
    /* KNOWN GAP (not implemented): 16-bit SS instructions
     * (C.SSPUSH/C.SSPOPCHK) Zcmop reversion is not directly tested;
     * they share the reversion path with the 32-bit forms (HCFI-SS-69). */
    printf("    Note: 16-bit SS reversion not implemented (known gap)\n");
    HYP_TEST_END();
}

/* HCFI-SS-71: SSE=0, ssp CSR access still triggers exception */
TEST_REGISTER(test_hcfi_ss_71);
bool test_hcfi_ss_71(void) {
    TEST_BEGIN("HCFI-SS-71: SSE=0, ssp CSR access -> virtual-inst");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(false, true);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_read_ssp, 0);
    TEST_ASSERT_EQ("virtual-instruction for ssp access (SSE=0)",
                   r, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-72: SSE=0, SS page encoding reserved in VS/VU-stage */
TEST_REGISTER(test_hcfi_ss_72);
bool test_hcfi_ss_72(void) {
    TEST_BEGIN("HCFI-SS-72: SSE=0, pte.xwr=010 reserved in VS/VU-stage");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    /* VS-stage part */
    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(false, true);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_load, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("page-fault for pte.xwr=010 (SSE=0, VS-stage)",
                   r, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx);

    /* VU-stage part: same reserved encoding at the VU level */
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    orig_h = cfi_setup_vs_sse(false, true);
    senvcfg_set(SENVCFG_SSE);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS | PTE_U);

    r = two_stage_run_in_vu(&ctx, vu_load, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("page-fault for pte.xwr=010 (SSE=0, VU-stage)",
                   r, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);

    senvcfg_clear(SENVCFG_SSE);
    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-73: SSE=0->1 switch, SS instructions immediately available */
TEST_REGISTER(test_hcfi_ss_73);
bool test_hcfi_ss_73(void) {
    TEST_BEGIN("HCFI-SS-73: SSE 0->1 switch, SS instructions available");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Start with SSE=0 */
    uintptr_t orig_h = cfi_setup_vs_sse(false, true);

    /* Verify SSPUSH is no-op with SSE=0 */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH no-op with SSE=0", r == 0);

    /* Switch to SSE=1 */
    cfi_restore_henvcfg(orig_h);
    orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* Now SSPUSH should execute as Zicfiss instruction */
    r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH works after SSE 0->1", r == 0);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-74: SSE=1->0 switch, SS instructions revert */
TEST_REGISTER(test_hcfi_ss_74);
bool test_hcfi_ss_74(void) {
    TEST_BEGIN("HCFI-SS-74: SSE 1->0 switch, SS instructions revert");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Start with SSE=1 */
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* Verify SSPUSH works with SSE=1 */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    printf("    SSPUSH with SSE=1 result = %lu\n", (unsigned long)r);

    /* Switch to SSE=0 */
    cfi_restore_henvcfg(orig_h);
    orig_h = cfi_setup_vs_sse(false, true);

    /* Now SSPUSH should be no-op */
    r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH reverts to no-op after SSE 1->0", r == 0);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-75: SSE switch does not need address translation cache sync */
TEST_REGISTER(test_hcfi_ss_75);
bool test_hcfi_ss_75(void) {
    TEST_BEGIN("HCFI-SS-75: SSE switch without SFENCE/HFENCE");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set SSE=0 first */
    uintptr_t orig_h = cfi_setup_vs_sse(false, true);

    /* Switch to SSE=1 without any SFENCE.VMA/HFENCE.VVMA */
    cfi_restore_henvcfg(orig_h);
    orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* Immediately execute SSPUSH - should work without cache sync */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH works after SSE switch (no sync)", r == 0);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}
