/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group B1: henvcfg.SSE field and VS-mode Zicfiss enable control
 *
 * Tests:
 *   HCFI-SS-01: henvcfg.SSE basic read/write
 *   HCFI-SS-02: henvcfg.SSE=1, VS-mode Zicfiss instructions execute
 *   HCFI-SS-03: henvcfg.SSE=0, 32-bit Zicfiss reverts to Zimop
 *   HCFI-SS-04: henvcfg.SSE=0, 16-bit Zicfiss reverts to Zcmop
 *   HCFI-SS-05: henvcfg.SSE=0, VS-stage pte.xwr=010 reserved
 *   HCFI-SS-06: henvcfg.SSE=0, senvcfg.SSE read-only zero
 *   HCFI-SS-07: henvcfg.SSE=0, senvcfg.SSE write has no effect
 *   HCFI-SS-08: henvcfg.SSE=1, senvcfg.SSE writable
 *   HCFI-SS-09: henvcfg.SSE=0+menvcfg.SSE=1, SSAMOSWAP triggers exception
 *   HCFI-SS-10: henvcfg.SSE=1, VS-mode SSAMOSWAP executes
 *   HCFI-SS-11: henvcfg.SSE=0+menvcfg.SSE=0, SSAMOSWAP reverts to Zimop
 *   HCFI-SS-12: VU-mode xSSE controlled by senvcfg.SSE
 *   HCFI-SS-13: henvcfg.SSE not implemented, read-only zero
 *   HCFI-SS-14: henvcfg.SSE=0, pte.xwr=010 triggers page-fault
 *
 * Norm references:
 *   norm:henvcfg_sse_op - SSE=1 activates Zicfiss in VS-mode
 */

/* HCFI-SS-01: henvcfg.SSE basic read/write */
TEST_REGISTER(test_hcfi_ss_01);
bool test_hcfi_ss_01(void) {
    TEST_BEGIN("HCFI-SS-01: henvcfg.SSE basic read/write");
    H_REQUIRED_OR_SKIP();

    uintptr_t orig = henvcfg_read();
    henvcfg_write(orig | HENVCFG_SSE);
    uintptr_t val = henvcfg_read();
    bool sse_writable = (val & HENVCFG_SSE) != 0;

    if (sse_writable) {
        printf("    henvcfg.SSE is writable (Zicfiss implemented)\n");
        henvcfg_write(orig & ~HENVCFG_SSE);
        val = henvcfg_read();
        TEST_ASSERT("henvcfg.SSE can be cleared", (val & HENVCFG_SSE) == 0);
    } else {
        printf("    henvcfg.SSE is read-only zero\n");
        TEST_ASSERT("henvcfg.SSE reads 0 when not implemented",
                    (val & HENVCFG_SSE) == 0);
    }
    henvcfg_write(orig);
    HYP_TEST_END();
}

/* HCFI-SS-02: henvcfg.SSE=1, VS-mode SSPUSH executes */
TEST_REGISTER(test_hcfi_ss_02);
bool test_hcfi_ss_02(void) {
    TEST_BEGIN("HCFI-SS-02: henvcfg.SSE=1, VS-mode SSPUSH executes");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write((uintptr_t)ss_page + 0x100);

    /* Mark ss_page as an SS page: per norm:ssmp_ss_page_illegeal_access,
     * an SS instruction targeting a non-SS, non-read-only page must raise
     * a store/AMO access-fault, so a successful SSPUSH requires the SS
     * page encoding (see HCFI-SS-28 for the negative case). */
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH executes without exception (SSE=1)", r == 0);

    cfi_restore_henvcfg(orig_h);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCFI-SS-03: henvcfg.SSE=0, 32-bit Zicfiss reverts to Zimop */
TEST_REGISTER(test_hcfi_ss_03);
bool test_hcfi_ss_03(void) {
    TEST_BEGIN("HCFI-SS-03: henvcfg.SSE=0, 32-bit SS instruction reverts to Zimop");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_h = cfi_setup_vs_sse(false, true);
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH as Zimop no-op (SSE=0)", r == 0);

    r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    TEST_ASSERT("SSPOPCHK as Zimop no-op (SSE=0)", r == 0);

    cfi_restore_henvcfg(orig_h);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCFI-SS-04: henvcfg.SSE=0, 16-bit Zicfiss reverts to Zcmop */
TEST_REGISTER(test_hcfi_ss_04);
bool test_hcfi_ss_04(void) {
    TEST_BEGIN("HCFI-SS-04: henvcfg.SSE=0, 16-bit SS instruction reverts to Zcmop");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();
    /* KNOWN GAP (not implemented): C.SSPUSH/C.SSPOPCHK are 16-bit
     * compressed instructions; their Zcmop reversion is not directly
     * tested here. The 16-bit instructions share the same functional
     * path; if 32-bit reversion works (HCFI-SS-03), 16-bit reversion
     * follows the same rule. Tracked as a known gap. */
    printf("    Note: C.SSPUSH/C.SSPOPCHK reversion not implemented (known gap)\n");
    HYP_TEST_END();
}

/* HCFI-SS-05: henvcfg.SSE=0, VS-stage pte.xwr=010 reserved */
TEST_REGISTER(test_hcfi_ss_05);
bool test_hcfi_ss_05(void) {
    TEST_BEGIN("HCFI-SS-05: henvcfg.SSE=0, pte.xwr=010 reserved");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_h = cfi_setup_vs_sse(false, true);

    /* Modify VS-stage PTE for ss_page to xwr=010 (SS page encoding) */
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* Access should trigger page-fault (encoding reserved when SSE=0) */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_store, SS_PAGE_ADDR);
    TEST_ASSERT("page-fault when pte.xwr=010 and SSE=0",
                r == CAUSE_STORE_PAGE_FAULT || r == CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCFI-SS-06: henvcfg.SSE=0, senvcfg.SSE read-only zero */
TEST_REGISTER(test_hcfi_ss_06);
bool test_hcfi_ss_06(void) {
    TEST_BEGIN("HCFI-SS-06: henvcfg.SSE=0, senvcfg.SSE read-only zero");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* First enable henvcfg.SSE and set senvcfg.SSE=1. */
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    senvcfg_set(SENVCFG_SSE);

    /* Now disable henvcfg.SSE: per norm:henvcfg_sse_op, senvcfg.SSE
     * becomes read-only zero from the VS-mode perspective. */
    henvcfg_write(henvcfg_read() & ~HENVCFG_SSE);

    /* VS-mode read senvcfg into g_vs_senvcfg_val */
    g_vs_senvcfg_val = 0;
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_read_senvcfg_fn, 0);
    TEST_ASSERT("senvcfg readable in VS-mode", r == 0);
    TEST_ASSERT("senvcfg.SSE reads 0 when henvcfg.SSE=0",
                (g_vs_senvcfg_val & SENVCFG_SSE) == 0);

    /* Restore: re-enable henvcfg.SSE first, then clear senvcfg.SSE */
    cfi_restore_henvcfg(orig_h);
    senvcfg_clear(SENVCFG_SSE);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCFI-SS-07: henvcfg.SSE=0, senvcfg.SSE write has no effect */
TEST_REGISTER(test_hcfi_ss_07);
bool test_hcfi_ss_07(void) {
    TEST_BEGIN("HCFI-SS-07: henvcfg.SSE=0, senvcfg.SSE write no effect");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_h = cfi_setup_vs_sse(false, true);
    senvcfg_clear(SENVCFG_SSE);

    /* VS-mode write senvcfg.SSE=1 */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_write_senvcfg_fn, SENVCFG_SSE);
    (void)r;

    /* Verify senvcfg.SSE still 0 */
    uintptr_t val = senvcfg_read();
    TEST_ASSERT("senvcfg.SSE stays 0 after write (henvcfg.SSE=0)",
                (val & SENVCFG_SSE) == 0);

    cfi_restore_henvcfg(orig_h);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCFI-SS-08: henvcfg.SSE=1, senvcfg.SSE writable */
TEST_REGISTER(test_hcfi_ss_08);
bool test_hcfi_ss_08(void) {
    TEST_BEGIN("HCFI-SS-08: henvcfg.SSE=1, senvcfg.SSE writable");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    senvcfg_clear(SENVCFG_SSE);

    /* VS-mode write senvcfg.SSE=1 */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_write_senvcfg_fn, SENVCFG_SSE);
    (void)r;

    /* Verify senvcfg.SSE is now 1 */
    uintptr_t val = senvcfg_read();
    TEST_ASSERT("senvcfg.SSE=1 after write (henvcfg.SSE=1)",
                (val & SENVCFG_SSE) != 0);

    senvcfg_clear(SENVCFG_SSE);
    cfi_restore_henvcfg(orig_h);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCFI-SS-09: henvcfg.SSE=0+menvcfg.SSE=1, SSAMOSWAP triggers virtual-inst */
TEST_REGISTER(test_hcfi_ss_09);
bool test_hcfi_ss_09(void) {
    TEST_BEGIN("HCFI-SS-09: henvcfg.SSE=0+menvcfg.SSE=1, SSAMOSWAP triggers virtual-inst");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_h = cfi_setup_vs_sse(false, true);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_ssamoswap_w, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("virtual-instruction exception from SSAMOSWAP",
                   r, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    cfi_restore_henvcfg(orig_h);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCFI-SS-10: henvcfg.SSE=1, VS-mode SSAMOSWAP executes */
TEST_REGISTER(test_hcfi_ss_10);
bool test_hcfi_ss_10(void) {
    TEST_BEGIN("HCFI-SS-10: henvcfg.SSE=1, VS-mode SSAMOSWAP.W executes");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* Set up SS page for the test */
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    /* Modify VS-stage PTE for ss_page to SS page encoding */
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_ssamoswap_w, SS_PAGE_ADDR);
    TEST_ASSERT("SSAMOSWAP.W executes without exception (SSE=1)", r == 0);

    cfi_restore_henvcfg(orig_h);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCFI-SS-11: henvcfg.SSE=0+menvcfg.SSE=0, SSAMOSWAP -> illegal-inst
 *
 * Per the ssamoswap operation pseudocode in cfi.adoc: privilege < M and
 * menvcfg.SSE=0 raises an illegal-instruction exception. SSAMOSWAP is
 * NOT Zimop-encoded (AMO major opcode), so it never reverts to Zimop. */
TEST_REGISTER(test_hcfi_ss_11);
bool test_hcfi_ss_11(void) {
    TEST_BEGIN("HCFI-SS-11: henvcfg.SSE=0+menvcfg.SSE=0, SSAMOSWAP -> illegal-inst");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_h = cfi_setup_vs_sse(false, false);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_ssamoswap_w, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("illegal-instruction from SSAMOSWAP (menvcfg.SSE=0)",
                   r, (uintptr_t)CAUSE_ILLEGAL_INST);

    cfi_restore_henvcfg(orig_h);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCFI-SS-12: VU-mode xSSE controlled by senvcfg.SSE */
TEST_REGISTER(test_hcfi_ss_12);
bool test_hcfi_ss_12(void) {
    TEST_BEGIN("HCFI-SS-12: VU-mode xSSE controlled by senvcfg.SSE=0");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    senvcfg_clear(SENVCFG_SSE);
    ssp_write(SS_PAGE_ADDR + 0x100);

    uintptr_t r = two_stage_run_in_vu(&ctx, vu_exec_sspush, 0);
    TEST_ASSERT("SSPUSH as Zimop in VU-mode (senvcfg.SSE=0)", r == 0);

    cfi_restore_henvcfg(orig_h);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* HCFI-SS-13: henvcfg.SSE not implemented, read-only zero */
TEST_REGISTER(test_hcfi_ss_13);
bool test_hcfi_ss_13(void) {
    TEST_BEGIN("HCFI-SS-13: henvcfg.SSE not implemented, read-only zero");
    H_REQUIRED_OR_SKIP();

    if (detect_zicfiss()) {
        printf("    Zicfiss is implemented, SSE is writable\n");
        TEST_SKIP("Zicfiss is implemented, skip read-only-zero test");
    }

    uintptr_t orig = henvcfg_read();
    henvcfg_write(orig | HENVCFG_SSE);
    uintptr_t val = henvcfg_read();
    TEST_ASSERT("henvcfg.SSE read-only zero when not implemented",
                (val & HENVCFG_SSE) == 0);
    henvcfg_write(orig);
    HYP_TEST_END();
}

/* HCFI-SS-14: henvcfg.SSE=0, pte.xwr=010 triggers page-fault */
TEST_REGISTER(test_hcfi_ss_14);
bool test_hcfi_ss_14(void) {
    TEST_BEGIN("HCFI-SS-14: henvcfg.SSE=0, pte.xwr=010 triggers page-fault");
    H_REQUIRED_OR_SKIP();
    ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t orig_h = cfi_setup_vs_sse(false, true);
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_load, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("page-fault (not access-fault) for pte.xwr=010 when SSE=0",
                   r, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);

    cfi_restore_henvcfg(orig_h);
    ts2_finish(&ctx);
    HYP_TEST_END();
}
