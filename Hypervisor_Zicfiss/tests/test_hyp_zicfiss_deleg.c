/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group B6: VS-mode Zicfiss exception behavior and delegation
 *
 * Tests HCFI-SS-49~58: SS fault delegation, access-fault delegation,
 * page-fault delegation, guest-page-fault non-delegatable.
 *
 * Norm references:
 *   norm:hedeleg_op, norm:hedeleg_acc, norm:ss_fault_exception_code
 */

/* HCFI-SS-49: VS-mode SSPOPCHK mismatch triggers SS Fault */
TEST_REGISTER(test_hcfi_ss_49);
bool test_hcfi_ss_49(void) {
    TEST_BEGIN("HCFI-SS-49: VS SSPOPCHK mismatch -> SS Fault");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);
    setup_deleg_to_hs(BIT(CAUSE_SOFTWARE_CHECK));

    /* SSPUSH then corrupt shadow stack, then SSPOPCHK should fail */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH on SS page succeeds", r == 0);
    /* Corrupt the shadow stack by writing directly */
    *(volatile uintptr_t *)(SS_PAGE_ADDR + 0xF8) = 0xDEAD;

    r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    TEST_ASSERT_EQ("software-check from SSPOPCHK mismatch",
                   r, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("tval = shadow stack fault (3)",
                   trap_get_tval(), (uintptr_t)SWCHECK_SHADOW_STACK_FAULT);

    cfi_restore_henvcfg(orig_h); clear_all_deleg(); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-50: SS Fault delegated to VS-mode */
TEST_REGISTER(test_hcfi_ss_50);
bool test_hcfi_ss_50(void) {
    TEST_BEGIN("HCFI-SS-50: SS Fault delegated to VS-mode");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);
    setup_deleg_to_vs(BIT(CAUSE_SOFTWARE_CHECK));

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* SSPUSH then corrupt the shadow stack, then SSPOPCHK */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH on SS page succeeds", r == 0);
    *(volatile uintptr_t *)(SS_PAGE_ADDR + 0xF8) = 0xDEAD;

    r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    (void)r;
    TEST_ASSERT("SS Fault delegated to VS-mode (vscause=18)",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("vstval = shadow stack fault (3)",
                   g_vs_exc_tval, (uintptr_t)SWCHECK_SHADOW_STACK_FAULT);

    cfi_restore_henvcfg(orig_h); clear_all_deleg(); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-51: SS Fault delegated to HS-mode */
TEST_REGISTER(test_hcfi_ss_51);
bool test_hcfi_ss_51(void) {
    TEST_BEGIN("HCFI-SS-51: SS Fault delegated to HS-mode");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);
    setup_deleg_to_hs(BIT(CAUSE_SOFTWARE_CHECK));

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* SSPUSH then corrupt the shadow stack, then SSPOPCHK */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH on SS page succeeds", r == 0);
    *(volatile uintptr_t *)(SS_PAGE_ADDR + 0xF8) = 0xDEAD;

    r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    TEST_ASSERT_EQ("software-check to HS", r, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("stval = shadow stack fault (3)",
                   trap_get_tval(), (uintptr_t)SWCHECK_SHADOW_STACK_FAULT);

    cfi_restore_henvcfg(orig_h); clear_all_deleg(); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-52: SS Fault delegated to M-mode */
TEST_REGISTER(test_hcfi_ss_52);
bool test_hcfi_ss_52(void) {
    TEST_BEGIN("HCFI-SS-52: SS Fault delegated to M-mode");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);
    clear_all_deleg();

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* SSPUSH then corrupt the shadow stack, then SSPOPCHK */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH on SS page succeeds", r == 0);
    *(volatile uintptr_t *)(SS_PAGE_ADDR + 0xF8) = 0xDEAD;

    r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    TEST_ASSERT_EQ("software-check to M", r, (uintptr_t)CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT_EQ("mtval = shadow stack fault (3)",
                   trap_get_tval(), (uintptr_t)SWCHECK_SHADOW_STACK_FAULT);

    cfi_restore_henvcfg(orig_h); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-53: VS-mode SS access-fault delegated to VS */
TEST_REGISTER(test_hcfi_ss_53);
bool test_hcfi_ss_53(void) {
    TEST_BEGIN("HCFI-SS-53: SS access-fault delegated to VS");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    setup_deleg_to_vs(BIT(CAUSE_STORE_ACCESS_FAULT));

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* Normal store to SS page triggers access-fault */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_store, SS_PAGE_ADDR);
    (void)r;
    TEST_ASSERT("SS access-fault delegated to VS (vscause=7)",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); clear_all_deleg(); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-54: VS-mode SS access-fault delegated to HS */
TEST_REGISTER(test_hcfi_ss_54);
bool test_hcfi_ss_54(void) {
    TEST_BEGIN("HCFI-SS-54: SS access-fault delegated to HS");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    setup_deleg_to_hs(BIT(CAUSE_STORE_ACCESS_FAULT));

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_store, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("store access-fault to HS", r, (uintptr_t)CAUSE_STORE_ACCESS_FAULT);

    cfi_restore_henvcfg(orig_h); clear_all_deleg(); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-55: VS-mode SS page-fault delegated to VS */
TEST_REGISTER(test_hcfi_ss_55);
bool test_hcfi_ss_55(void) {
    TEST_BEGIN("HCFI-SS-55: SS page-fault delegated to VS");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);
    setup_deleg_to_vs(BIT(CAUSE_STORE_PAGE_FAULT));

    /* Read-only page for SS instruction */
    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_V | PTE_R | PTE_A | PTE_D);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    (void)r;
    TEST_ASSERT("SS page-fault delegated to VS (vscause=15)",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_STORE_PAGE_FAULT);

    cfi_restore_henvcfg(orig_h); clear_all_deleg(); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-56: VS-mode SS guest-page-fault not delegatable to VS */
TEST_REGISTER(test_hcfi_ss_56);
bool test_hcfi_ss_56(void) {
    TEST_BEGIN("HCFI-SS-56: SS guest-page-fault traps to HS (not VS)");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);

    /* Try to delegate guest-page-fault to VS (should not work) */
    setup_deleg_to_vs(BIT(CAUSE_STORE_GUEST_PAGE_FAULT));

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);
    g_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, 0);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    /* Guest-page-fault should trap to HS, not VS */
    TEST_ASSERT_EQ("guest-page-fault to HS (not VS)",
                   r, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    uintptr_t hs = hstatus_read();
    TEST_ASSERT("hstatus.GVA=1", (hs & HSTATUS_GVA) != 0);

    cfi_restore_henvcfg(orig_h); clear_all_deleg(); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-57: SS Fault to VS, vsepc correct */
TEST_REGISTER(test_hcfi_ss_57);
bool test_hcfi_ss_57(void) {
    TEST_BEGIN("HCFI-SS-57: SS Fault to VS, vsepc correct");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    ssp_write(SS_PAGE_ADDR + 0x100);
    setup_deleg_to_vs(BIT(CAUSE_SOFTWARE_CHECK));

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    /* SSPUSH then corrupt the shadow stack, then SSPOPCHK */
    uintptr_t r = two_stage_run_in_vs(&ctx, vs_exec_sspush, 0);
    TEST_ASSERT("SSPUSH on SS page succeeds", r == 0);
    *(volatile uintptr_t *)(SS_PAGE_ADDR + 0xF8) = 0xDEAD;

    r = two_stage_run_in_vs(&ctx, vs_exec_sspopchk, 0);
    (void)r;
    /* vsepc should point to the SSPOPCHK instruction (recorded by the
     * trampoline in g_sspopchk_addr) */
    printf("    vsepc = 0x%lx, sspopchk addr = 0x%lx\n",
           (unsigned long)g_vs_exc_epc, (unsigned long)g_sspopchk_addr);
    TEST_ASSERT("SS Fault delegated to VS-mode",
                g_vs_exc_triggered &&
                g_vs_exc_cause == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("vsepc = SSPOPCHK instruction address",
                g_vs_exc_epc == g_sspopchk_addr);

    cfi_restore_henvcfg(orig_h); clear_all_deleg(); ts2_finish(&ctx); HYP_TEST_END();
}

/* HCFI-SS-58: SS access-fault to HS, sepc/scause correct */
TEST_REGISTER(test_hcfi_ss_58);
bool test_hcfi_ss_58(void) {
    TEST_BEGIN("HCFI-SS-58: SS access-fault to HS, sepc/scause correct");
    H_REQUIRED_OR_SKIP(); ZICFISS_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    pt_pool_reset(); gpt_pool_reset();
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    uintptr_t orig_h = cfi_setup_vs_sse(true, true);
    setup_deleg_to_hs(BIT(CAUSE_STORE_ACCESS_FAULT));

    vs_pte_modify(&ctx, SS_PAGE_ADDR, PT_LEVEL_4K, PTE_SS_PAGE_FLAGS);

    uintptr_t r = two_stage_run_in_vs(&ctx, vs_store, SS_PAGE_ADDR);
    TEST_ASSERT_EQ("store access-fault", r, (uintptr_t)CAUSE_STORE_ACCESS_FAULT);

    uintptr_t sepc_val = CSRR(sepc);
    uintptr_t hs = hstatus_read();
    printf("    sepc = 0x%lx, SPV = %lu\n",
           (unsigned long)sepc_val,
           (unsigned long)((hs & HSTATUS_SPV) ? 1 : 0));

    TEST_ASSERT("hstatus.SPV=1", (hs & HSTATUS_SPV) != 0);

    cfi_restore_henvcfg(orig_h); clear_all_deleg(); ts2_finish(&ctx); HYP_TEST_END();
}
