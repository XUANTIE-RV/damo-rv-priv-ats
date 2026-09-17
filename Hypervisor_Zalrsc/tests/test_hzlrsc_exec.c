/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.1 - HS/VS/VU-mode normal execution and cause=22 exclusion
 *             (HZLRSC-01 ~ HZLRSC-04)
 *
 * Spec basis:
 *   norm:lr_w_op / norm:sc_w_success / norm:lr_sc_rv64 - LR/SC base
 *     semantics are unchanged under virtualization.
 *   norm:hlsv_virtinst - only HLV/HLVX/HSV raise virtual-instruction
 *     (cause=22); LR/SC have no such clause, so a guest LR/SC must
 *     NEVER report cause=22 (mandatory negative assertion).
 * =================================================================== */

/* HS-mode LR/SC target (physical, satp=Bare in HS-mode). */
static volatile uint64_t hzlrsc_hs_slot[4];

/* Semantic-comparison record (written by the probe, read in M-mode). */
static volatile uintptr_t g_hz_sem_lr;
static volatile uintptr_t g_hz_sem_sc;
static volatile uintptr_t g_hz_sem_mem;

/* Known initial word: bit31 set so a correct .w LR sign-extends rd to
 * 0xFFFFFFFF_80000001 (norm:lr_sc_rv64). */
#define HZ_SEM_INIT_W   0x80000001UL
#define HZ_SEM_NEW_W    0x09ABCDEFUL

/* LR.W + SC.W recording the architecturally visible results. Runs in
 * HS-mode (run_in_priv) or VS-mode (two_stage_run_in_vs) unchanged. */
static uintptr_t hz_lrsc_w_semantic(uintptr_t addr)
{
    uintptr_t lr, sc = 1;
    for (int i = 0; i < HZLRSC_PAIR_TRIES && sc != 0; i++)
        HZ_PAIR_W("", "", lr, sc, addr, (uintptr_t)HZ_SEM_NEW_W);
    g_hz_sem_lr  = lr;
    g_hz_sem_sc  = sc;
    g_hz_sem_mem = *(volatile uint32_t *)addr;
    return sc;
}

/* ------------------------------------------------------------------
 * HZLRSC-01: HS-mode lr.w/sc.w and lr.d/sc.d execute normally.
 * ------------------------------------------------------------------ */
static uintptr_t hz_hs_lrsc_w(uintptr_t addr)
{
    uintptr_t lr, sc = 1;
    for (int i = 0; i < HZLRSC_PAIR_TRIES && sc != 0; i++)
        HZ_PAIR_W("", "", lr, sc, addr, 0x12345678UL);
    return sc;
}
#if __riscv_xlen == 64
static uintptr_t hz_hs_lrsc_d(uintptr_t addr)
{
    uintptr_t lr, sc = 1;
    for (int i = 0; i < HZLRSC_PAIR_TRIES && sc != 0; i++)
        HZ_PAIR_D("", "", lr, sc, addr, 0x0F1E2D3C4B5A6978ULL);
    return sc;
}
#endif

TEST_REGISTER(test_hzlrsc_01_hs_exec);
bool test_hzlrsc_01_hs_exec(void)
{
    TEST_BEGIN("HZLRSC-01: HS-mode lr.w/sc.w + lr.d/sc.d succeed");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");

    /* First case ONLY: config declares Zalrsc/A, so probe the DUT to
     * confirm it really implements the reservation instructions
     * (aligned with the ZALRSC_SUPPORTED/A_SUPPORTED declaration). A
     * trap-armed lr.w/sc.w pair in M-mode raising illegal-instruction
     * would mean the DUT does NOT implement it despite the config. No
     * other case probes. */
    {
        static volatile uint64_t hzlrsc_probe_slot;
        uintptr_t paddr = (uintptr_t)&hzlrsc_probe_slot;
        uintptr_t plr, psc;
        M_TRAP_EXPECT_BEGIN();
        asm volatile(
            ".option push\n\t.option norvc\n\t"
            "lr.w %0, (%2)\n\t"
            "sc.w %1, %3, (%2)\n\t"
            ".option pop\n\t"
            : "=&r"(plr), "=&r"(psc)
            : "r"(paddr), "r"(0x12345678UL) : "memory");
        bool ptrapped = trap_was_triggered();
        uintptr_t pcause = ptrapped ? trap_get_cause() : 0;
        trap_expect_end();
        TEST_ASSERT("DUT really implements Zalrsc/A (aligned with config)",
                    !(ptrapped && pcause == CAUSE_ILLEGAL_INST));
    }

    uintptr_t waddr = (uintptr_t)&hzlrsc_hs_slot[0];
    uintptr_t daddr = (uintptr_t)&hzlrsc_hs_slot[2];
    *(volatile uint32_t *)waddr = 0;
    *(volatile uint64_t *)daddr = 0;

    uintptr_t sc_w = run_in_priv(PRIV_S, hz_hs_lrsc_w, waddr);
    TEST_ASSERT_EQ("sc.w success (rd=0)", sc_w, (uintptr_t)0);
    TEST_ASSERT_EQ("memory updated by sc.w",
                   *(volatile uint32_t *)waddr, (uintptr_t)0x12345678UL);

#if __riscv_xlen == 64
    uintptr_t sc_d = run_in_priv(PRIV_S, hz_hs_lrsc_d, daddr);
    TEST_ASSERT_EQ("sc.d success (rd=0)", sc_d, (uintptr_t)0);
    TEST_ASSERT_EQ("memory updated by sc.d",
                   *(volatile uint64_t *)daddr,
                   (uintptr_t)0x0F1E2D3C4B5A6978ULL);
#endif

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-02: VS-mode LR/SC succeed and never report cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_02_vs_exec_no_cause22);
bool test_hzlrsc_02_vs_exec_no_cause22(void)
{
    TEST_BEGIN("HZLRSC-02: VS-mode lr/sc succeed, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0;
#if __riscv_xlen == 64
    uintptr_t va_d = (uintptr_t)test_data_area + 16;
    *(volatile uint64_t *)va_d = 0;
#endif

    trap_expect_begin();
    uintptr_t sc_w = two_stage_run_in_vs(&ctx, hz_vs_lrsc_w_retry, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("no trap in VS-mode (esp. no virtual-instruction 22)",
                !fired);
    TEST_ASSERT_NEQ("VS-mode LR/SC did not report cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("sc.w success in VS-mode (rd=0)", sc_w, (uintptr_t)0);
    TEST_ASSERT_EQ("VS-mode memory updated",
                   *(volatile uint32_t *)va, (uintptr_t)0x12345678UL);

#if __riscv_xlen == 64
    trap_expect_begin();
    uintptr_t sc_d = two_stage_run_in_vs(&ctx, hz_vs_lrsc_d_retry, va_d);
    bool fired_d = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("no trap for VS-mode lr.d/sc.d", !fired_d);
    TEST_ASSERT_EQ("sc.d success in VS-mode (rd=0)", sc_d, (uintptr_t)0);
#endif

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-03: VU-mode LR/SC succeed and never report cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_03_vu_exec_no_cause22);
bool test_hzlrsc_03_vu_exec_no_cause22(void)
{
    TEST_BEGIN("HZLRSC-03: VU-mode lr.w/sc.w succeed, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0;

    trap_expect_begin();
    uintptr_t sc = two_stage_run_in_vu(&ctx, hz_vs_lrsc_w_retry, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("no trap in VU-mode (esp. no virtual-instruction 22)",
                !fired);
    TEST_ASSERT_NEQ("VU-mode LR/SC did not report cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("sc.w success in VU-mode (rd=0)", sc, (uintptr_t)0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-04: VS-mode LR/SC semantics match the non-virtualized (HS)
 *            run: loaded value sign-extension, SC result, final memory.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_04_vs_hs_semantic_parity);
bool test_hzlrsc_04_vs_hs_semantic_parity(void)
{
    TEST_BEGIN("HZLRSC-04: VS-mode LR/SC semantics == HS-mode");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* HS-mode run on a physical slot. */
    uintptr_t hs_addr = (uintptr_t)&hzlrsc_hs_slot[0];
    *(volatile uint32_t *)hs_addr = HZ_SEM_INIT_W;
    g_hz_sem_lr = g_hz_sem_sc = g_hz_sem_mem = 0;
    (void)run_in_priv(PRIV_S, hz_lrsc_w_semantic, hs_addr);
    uintptr_t hs_lr = g_hz_sem_lr, hs_sc = g_hz_sem_sc, hs_mem = g_hz_sem_mem;

    /* VS-mode run on the two-stage-mapped test region. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t vs_addr = (uintptr_t)test_data_area;
    *(volatile uint32_t *)vs_addr = HZ_SEM_INIT_W;
    g_hz_sem_lr = g_hz_sem_sc = g_hz_sem_mem = 0;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_lrsc_w_semantic, vs_addr);
    bool fired = trap_was_triggered();
    trap_expect_end();
    uintptr_t vs_lr = g_hz_sem_lr, vs_sc = g_hz_sem_sc, vs_mem = g_hz_sem_mem;
    ts2_finish(&ctx);

    TEST_ASSERT("VS-mode semantic run took no trap", !fired);
    /* .w LR sign-extends the loaded word (norm:lr_sc_rv64). */
    TEST_ASSERT_EQ("HS lr.w rd sign-extended",
                   hs_lr, HZ_W_SIGNEXT(HZ_SEM_INIT_W));
    TEST_ASSERT_EQ("VS lr.w rd == HS lr.w rd", vs_lr, hs_lr);
    TEST_ASSERT_EQ("VS sc.w result == HS sc.w result", vs_sc, hs_sc);
    TEST_ASSERT_EQ("VS final memory == HS final memory", vs_mem, hs_mem);
    TEST_ASSERT_EQ("final memory holds the SC-written value",
                   vs_mem, (uintptr_t)HZ_SEM_NEW_W);

    HYP_TEST_END();
}
