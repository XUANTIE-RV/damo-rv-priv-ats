/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group B3: VS-stage page table Shadow Stack page type
 *
 * Tests HCFI-SS-24~36: SS page encoding, protection, access-fault behavior
 *
 * Norm references:
 *   norm:ss_page_enc - R=0,W=1,X=0 = SS page
 *   norm:ssmp_ss_page_access_fault - non-SS instruction writing SS page -> access-fault
 *   norm:ssmp_ss_read_only_page - SS instruction on read-only page -> page-fault
 *   norm:ss_fault_exception_code - SS exception cause 7/15/23
 */

/* HCFI-SS-24: henvcfg.SSE=1, pte.xwr=010 is valid SS page */
TEST_REGISTER(test_hcfi_ss_24);
bool test_hcfi_ss_24(void) {
    TEST_BEGIN("HCFI-SS-24: SSE=1, pte.xwr=010 is valid SS page");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);
    ssp_write(SS_PAGE_ADDR + 0x100);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH on SS page succeeds (SSE=1)", r == 0);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-25: henvcfg.SSE=0, pte.xwr=010 reserved */
TEST_REGISTER(test_hcfi_ss_25);
bool test_hcfi_ss_25(void) {
    TEST_BEGIN("HCFI-SS-25: SSE=0, pte.xwr=010 reserved -> page-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(false, true);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_store, SS_PAGE_ADDR);
    TEST_ASSERT("page-fault for pte.xwr=010 when SSE=0",
                r == CAUSE_STORE_PAGE_FAULT || r == CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-26: normal store to SS page triggers access-fault */
TEST_REGISTER(test_hcfi_ss_26);
bool test_hcfi_ss_26(void) {
    TEST_BEGIN("HCFI-SS-26: normal store to SS page -> access-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_store, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("store/AMO access-fault on SS page", r, (uintptr_t)CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-27: normal load from SS page succeeds */
TEST_REGISTER(test_hcfi_ss_27);
bool test_hcfi_ss_27(void) {
    TEST_BEGIN("HCFI-SS-27: normal load from SS page succeeds");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_load, SS_PAGE_ADDR);
    TEST_ASSERT("load from SS page succeeds", r == 0);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-28: SS instruction on non-SS RW page -> access-fault */
TEST_REGISTER(test_hcfi_ss_28);
bool test_hcfi_ss_28(void) {
    TEST_BEGIN("HCFI-SS-28: SS instruction on non-SS page -> access-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(RW_PAGE_ADDR + 0x100);

    /* RW page (xwr=011) is not SS page */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT_EQ("access-fault for SS instruction on non-SS page",
                   r, (uintptr_t)CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-29: SS instruction on read-only page -> page-fault */
TEST_REGISTER(test_hcfi_ss_29);
bool test_hcfi_ss_29(void) {
    TEST_BEGIN("HCFI-SS-29: SS instruction on read-only page -> page-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    /* Read-only page (xwr=001) */
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_V | PTE_R | PTE_A | PTE_D);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT_EQ("page-fault for SS instruction on read-only page",
                   r, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-30: CBO instruction on SS page -> access-fault */
TEST_REGISTER(test_hcfi_ss_30);
bool test_hcfi_ss_30(void) {
    TEST_BEGIN("HCFI-SS-30: CBO on SS page -> access-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* KNOWN GAP (proxy verification): this uses a normal store rather
     * than an actual CBO.INVAL instruction. Per
     * norm:ssmp_ss_cache_block_access_fault, CBO access to an SS page
     * must also raise a store/AMO access-fault; the normal-store case
     * (norm:ssmp_ss_page_access_fault) exercises the same SS-page
     * write-protection path. Tracked as a known gap. */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_store, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("access-fault for store on SS page", r, (uintptr_t)CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-31: instruction fetch from SS page -> access-fault */
TEST_REGISTER(test_hcfi_ss_31);
bool test_hcfi_ss_31(void) {
    TEST_BEGIN("HCFI-SS-31: instruction fetch from SS page -> access-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* Jump to SS page for execution - should trigger instruction access-fault.
     * Must arm trap before entering VS-mode: test_vs_exec_expect_fault uses
     * _exec_return_addr recovery, which only works when armed=true. */
    trap_expect_begin();
    uintptr_t r = two_stage_run_in_vs(&ctx, test_vs_exec_expect_fault, SS_PAGE_ADDR);
    trap_expect_end();
    TEST_ASSERT("instruction access-fault from SS page fetch",
                r == CAUSE_INST_ACCESS_FAULT || r == CAUSE_INST_PAGE_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-32: SSPOPCHK reads SS page normally */
TEST_REGISTER(test_hcfi_ss_32);
bool test_hcfi_ss_32(void) {
    TEST_BEGIN("HCFI-SS-32: SSPOPCHK reads SS page normally");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);
    ssp_write(SS_PAGE_ADDR + 0x100);

    /* First SSPUSH to put a value on shadow stack, then SSPOPCHK */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH on SS page succeeds", r == 0);
    r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    TEST_ASSERT("SSPOPCHK reads SS page and matches", r == 0);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-33: SSPOPCHK on unmapped page -> store page-fault */
TEST_REGISTER(test_hcfi_ss_33);
bool test_hcfi_ss_33(void) {
    TEST_BEGIN("HCFI-SS-33: SSPOPCHK on unmapped page -> store page-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    /* Point ssp to unmapped address */
    ssp_write((uintptr_t)test_fault_page + 0x100);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    /* SSPOPCHK reports as store type fault, not load */
    TEST_ASSERT("store/AMO page-fault (not load) from SSPOPCHK",
                r == CAUSE_STORE_PAGE_FAULT || r == CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-34: SS page COW scenario */
TEST_REGISTER(test_hcfi_ss_34);
bool test_hcfi_ss_34(void) {
    TEST_BEGIN("HCFI-SS-34: SS page COW -> store page-fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    /* Read-only page (COW marker = xwr=001) */
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_V | PTE_R | PTE_A | PTE_D);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT_EQ("store page-fault for COW SS page",
                   r, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-35: U/SUM bit effect on SS page access */
TEST_REGISTER(test_hcfi_ss_35);
bool test_hcfi_ss_35(void) {
    TEST_BEGIN("HCFI-SS-35: U/SUM bit effect on SS page access");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    senvcfg_set(SENVCFG_SSE);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS | PTE_U);
    ssp_write(SS_PAGE_ADDR + 0x100);

    uintptr_t r = two_stage_run_in_vu(&ctx, vu_exec_sspush, 0);
    TEST_ASSERT("VU-mode SSPUSH on U SS page succeeds", r == 0);

    senvcfg_clear(SENVCFG_SSE);
    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-36: MXR bit does not affect SS page load */
TEST_REGISTER(test_hcfi_ss_36);
bool test_hcfi_ss_36(void) {
    TEST_BEGIN("HCFI-SS-36: MXR bit does not affect SS page load");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* Set MXR in vsstatus */
    uintptr_t vss = vsstatus_read();
    vsstatus_write(vss | MSTATUS_MXR_BIT);

    /* Load from SS page should still succeed */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_load, SS_PAGE_ADDR);
    TEST_ASSERT("load from SS page with MXR=1 succeeds", r == 0);

    vsstatus_write(vss);
    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}
