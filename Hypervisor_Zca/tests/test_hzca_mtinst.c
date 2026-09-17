/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.6 - mtinst compressed transformed on an M-mode trap
 *             Group 1.7 - control: interrupt writes zero
 *             (HZCA-21 ~ HZCA-24)
 *
 * Spec basis:
 *   norm:H_trap_xtinst - "the appropriate trap instruction CSR, mtinst
 *     or htinst" share the same transformed/pseudoinstruction value
 *     rules. When a guest compressed load/store fault is NOT delegated
 *     (medeleg[cause]=0) it traps into M-mode and hardware writes the
 *     same compressed transformed value into mtinst.
 *   norm:H_trap_xtinst_interrupt - on an interrupt the trap instruction
 *     register is ALWAYS zero.
 *
 * The framework snapshots mtinst (M-mode entry) or htinst (HS-mode
 * entry) into the same trap record; trap_get_htinst() returns whichever
 * applies (hyp_capture_m / hyp_capture_s in common/trap.c).
 * =================================================================== */

#ifndef PLATFORM_MSIP_ADDR
#define PLATFORM_MSIP_ADDR  PLATFORM_CLINT_BASE
#endif

/* ------------------------------------------------------------------
 * HZCA-21: guest compressed LOAD fault not delegated -> traps into
 *          M-mode; mtinst = 0 or the compressed transformed value.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_21_mtinst_load);
bool test_hzca_21_mtinst_load(void)
{
    TEST_BEGIN("HZCA-21: c.lw fault into M-mode -> mtinst 0/transformed");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    /* to_hs = false -> medeleg[21] clear -> M-mode entry -> mtinst. */
    hzca_trap_t t = hzca_fire_mem_fault(hz_vs_c_lw, va, HZ_G_INV,
                                        CAUSE_LOAD_GUEST_PAGE_FAULT, false);

    TEST_ASSERT("c.lw guest fault fired into M-mode", t.fired);
    TEST_ASSERT_EQ("cause == load guest-page-fault (21)", t.cause,
                   (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    hzca_check_trap_inst_compressed(t.epc, HZCA_ENC_C_LW_A0);
    hzca_assert_xtinst_compressed("mtinst == 0 or compressed transformed lw",
                                  t.xtinst, HZCA_EXP_LW_A0_A0, t.tval, va);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-22: guest compressed STORE fault not delegated -> M-mode;
 *          mtinst = 0 or the compressed transformed store value.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_22_mtinst_store);
bool test_hzca_22_mtinst_store(void)
{
    TEST_BEGIN("HZCA-22: c.sw fault into M-mode -> mtinst 0/transformed");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    /* to_hs = false -> medeleg[23] clear -> M-mode entry -> mtinst. */
    hzca_trap_t t = hzca_fire_mem_fault(hz_vs_c_sw, va, HZ_G_RU,
                                        CAUSE_STORE_GUEST_PAGE_FAULT, false);

    TEST_ASSERT("c.sw guest fault fired into M-mode", t.fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)", t.cause,
                   (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    hzca_check_trap_inst_compressed(t.epc, HZCA_ENC_C_SW_A0);
    hzca_assert_xtinst_compressed("mtinst == 0 or compressed transformed sw",
                                  t.xtinst, HZCA_EXP_SW_A0_A0, t.tval, va);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-23: mtinst and htinst transformed values are consistent for the
 *          same compressed memory-access fault under two routings.
 *
 * The bit-for-bit comparison is only meaningful when BOTH runs write a
 * nonzero transformed value; either side writing zero is a legal
 * implementation choice and is recorded without a failure verdict.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_23_mtinst_htinst_consistent);
bool test_hzca_23_mtinst_htinst_consistent(void)
{
    TEST_BEGIN("HZCA-23: mtinst and htinst transformed values consistent");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    /* (a) delegated to HS-mode -> htinst. */
    hzca_trap_t hs = hzca_fire_mem_fault(hz_vs_c_lw, va, HZ_G_INV,
                                         CAUSE_LOAD_GUEST_PAGE_FAULT, true);
    /* (b) not delegated -> M-mode -> mtinst. */
    hzca_trap_t m  = hzca_fire_mem_fault(hz_vs_c_lw, va, HZ_G_INV,
                                         CAUSE_LOAD_GUEST_PAGE_FAULT, false);

    TEST_ASSERT("(a) htinst run fired", hs.fired);
    TEST_ASSERT("(b) mtinst run fired", m.fired);

    printf("  [INFO] htinst(HS)=0x%lx  mtinst(M)=0x%lx\n",
           (unsigned long)hs.xtinst, (unsigned long)m.xtinst);

    if (hs.xtinst != 0 && m.xtinst != 0) {
        TEST_ASSERT_EQ("nonzero htinst == nonzero mtinst (shared rules)",
                       hs.xtinst, m.xtinst);
    } else {
        printf("  [INFO] at least one side wrote zero (legal); "
               "bit-for-bit consistency not observable this run\n");
    }
    /* Both must individually satisfy the 0-or-golden rule regardless. */
    uintptr_t golden = hzca_golden_from(HZCA_EXP_LW_A0_A0, hs.tval, va);
    TEST_ASSERT("htinst == 0 or golden", hs.xtinst == 0 || hs.xtinst == golden);
    uintptr_t golden_m = hzca_golden_from(HZCA_EXP_LW_A0_A0, m.tval, va);
    TEST_ASSERT("mtinst == 0 or golden", m.xtinst == 0 || m.xtinst == golden_m);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-24: control - an interrupt that traps into M-mode in a V=1
 *          context writes mtinst = 0 (strict).
 *
 * A machine software interrupt (MSIP) is pended via the CLINT and
 * mie.MSIE enabled; on entry to VS-mode it is taken into M-mode (machine
 * interrupts are never delegated). The framework's M-mode handler
 * auto-clears MSIP, so it fires exactly once.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_24_mtinst_interrupt_zero);
bool test_hzca_24_mtinst_interrupt_zero(void)
{
    TEST_BEGIN("HZCA-24: mtinst == 0 on M-mode interrupt (V=1 context)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    uintptr_t saved_mie;
    asm volatile ("csrr %0, mie" : "=r"(saved_mie));
    uintptr_t saved_mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

    /* Keep M-mode interrupts disabled (mstatus.MIE=0) while arming the
     * source in M-mode, so MSIP is NOT taken here; a machine interrupt
     * is globally enabled once the hart drops to VS-mode (privilege < M)
     * regardless of mstatus.MIE, so it is taken there with hstatus.SPV=1
     * (norm: requires the trap to arise in the V=1 context). */
    asm volatile ("csrc mstatus, %0" :: "r"(MSTATUS_MIE_BIT));

    /* Enable mie.MSIE and pend MSIP for hart 0. */
    volatile uint32_t *msip = (volatile uint32_t *)PLATFORM_MSIP_ADDR;
    asm volatile ("csrs mie, %0" :: "r"(1UL << IRQ_M_SOFTWARE));
    *msip = 1;
    asm volatile ("fence" ::: "memory");

    /* Enter VS-mode; the pending machine software interrupt is taken
     * into M-mode (V=1 at entry). vs_nop_fn is a safe VS-mode payload. */
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, vs_nop_fn, 0);
    bool fired = trap_was_triggered();
    uintptr_t cause  = fired ? trap_get_cause()  : 0;
    uintptr_t mtinst = fired ? trap_get_htinst() : 0;
    /* A trap into M-mode records the prior V bit in mstatus.MPV, not
     * hstatus.SPV (SPEC hypervisor.adoc: a trap into M-mode writes MPV;
     * hstatus.SPV is written only for traps into HS-mode). */
    bool mpv = trap_get_mpv();
    trap_expect_end();

    /* Disarm (the handler already cleared MSIP; be explicit). */
    *msip = 0;
    asm volatile ("fence" ::: "memory");
    asm volatile ("csrw mie, %0" :: "r"(saved_mie));
    asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));
    ts2_finish(&ctx);

    TEST_ASSERT("machine software interrupt fired in V=1 context", fired);
    TEST_ASSERT_EQ("cause == machine software interrupt",
                   cause, (uintptr_t)(CAUSE_INTERRUPT_BIT | IRQ_M_SOFTWARE));
    TEST_ASSERT("interrupt taken from V=1 (mstatus.MPV at M-mode entry)", mpv);
    TEST_ASSERT_EQ("mtinst == 0 on interrupt (strict)", mtinst, 0);

    HYP_TEST_END();
}
