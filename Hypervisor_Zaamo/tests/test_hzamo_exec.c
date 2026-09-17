/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2.1 - HS/VS/VU-mode normal execution and cause=22 exclusion
 *             (HZAMO-01 ~ HZAMO-04)
 *
 * Spec basis:
 *   amo_rmw_semantics / norm:amo_operand_size - an AMO atomically loads
 *     the old value into rd, applies the operator with rs2, and stores the
 *     result back; RV64 .w AMOs sign-extend rd and ignore rs2[63:32].
 *     These semantics are unchanged under virtualization.
 *   norm:hlsv_virtinst - only HLV/HLVX/HSV raise virtual-instruction
 *     (cause=22); no hypervisor.adoc clause gates AMOs, so a guest AMO
 *     must NEVER report cause=22 (mandatory negative assertion).
 * =================================================================== */

static volatile uint64_t hzamo_hs_slot[8];

/* Semantic record (written by the probe, read in M-mode). */
static volatile uintptr_t g_hz_amo_rd;
static volatile uintptr_t g_hz_amo_mem;

/* Initial word with bit31 set: a correct .w AMO sign-extends the old
 * value into rd (norm:amo_operand_size) and ignores rs2[63:32]. */
#define HZ_AMO_INIT_W   0x80000001UL
#define HZ_AMO_ADDEND   0x00000001UL

/* amoadd.w recording rd (sign-extended old value) and final memory.
 * Uses the raw mnemonic form with an XLEN-wide rd so the RV64 .w
 * sign-extension (norm:amo_operand_size) is preserved; mem_amo_add_w
 * returns uint32_t and would truncate it. */
static uintptr_t hz_amo_add_w_semantic(uintptr_t addr)
{
    uintptr_t rd;
    HZ_AMO_W("amoadd", "", rd, addr, (uintptr_t)HZ_AMO_ADDEND);
    g_hz_amo_rd  = rd;
    g_hz_amo_mem = *(volatile uint32_t *)addr;
    return rd;
}

/* Run the full 9-op .w AMO set (HS-mode). Returns 0. */
static uintptr_t hz_hs_all_amo_w(uintptr_t addr)
{
    return hz_vs_all_amo_w(addr);
}

/* ------------------------------------------------------------------
 * HZAMO-01: HS-mode executes all 9 AMOs (.w, plus .d on RV64) normally.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_01_hs_exec);
bool test_hzamo_01_hs_exec(void)
{
    TEST_BEGIN("HZAMO-01: HS-mode all 9 AMOs (.w/.d) execute normally");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");

    /* First case ONLY: config declares Zaamo/A, so probe the DUT to
     * confirm it really implements the AMO instructions (aligned with
     * the ZAAMO_SUPPORTED/A_SUPPORTED declaration). A trap-armed
     * amoadd.w in M-mode raising illegal-instruction would mean the DUT
     * does NOT implement it despite the config. No other case probes. */
    {
        static volatile uint64_t hzamo_probe_slot;
        uintptr_t paddr = (uintptr_t)&hzamo_probe_slot;
        uintptr_t pold;
        M_TRAP_EXPECT_BEGIN();
        asm volatile(
            ".option push\n\t.option norvc\n\t"
            "amoadd.w %0, %2, (%1)\n\t"
            ".option pop\n\t"
            : "=r"(pold) : "r"(paddr), "r"(1UL) : "memory");
        bool ptrapped = trap_was_triggered();
        uintptr_t pcause = ptrapped ? trap_get_cause() : 0;
        trap_expect_end();
        TEST_ASSERT("DUT really implements Zaamo/A (aligned with config)",
                    !(ptrapped && pcause == CAUSE_ILLEGAL_INST));
    }

    uintptr_t waddr = (uintptr_t)&hzamo_hs_slot[0];
    *(volatile uint32_t *)waddr = 0x1000u;

    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hz_hs_all_amo_w, waddr);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("all 9 .w AMOs executed in HS-mode without exception", !fired);

    /* amoadd.w data semantics: rd = old value, memory = old + addend. */
    *(volatile uint32_t *)waddr = 0x0000002Au;
    uintptr_t rd = run_in_priv(PRIV_S, hz_amo_add_w_semantic, waddr);
    TEST_ASSERT_EQ("amoadd.w rd == old value", rd, (uintptr_t)0x2Au);
    TEST_ASSERT_EQ("amoadd.w memory == old + addend",
                   *(volatile uint32_t *)waddr, (uintptr_t)0x2Bu);

#if __riscv_xlen == 64
    uintptr_t daddr = (uintptr_t)&hzamo_hs_slot[4];
    *(volatile uint64_t *)daddr = 0x1000ULL;
    trap_expect_begin();
    uintptr_t rd_d = run_in_priv(PRIV_S, ({
        uintptr_t (*f)(uintptr_t) = hz_vs_amo_add_d; f; }), daddr);
    bool fired_d = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("amoadd.d executed in HS-mode without exception", !fired_d);
    TEST_ASSERT_EQ("amoadd.d rd == old value", rd_d, (uintptr_t)0x1000ULL);
    TEST_ASSERT_EQ("amoadd.d memory == old + addend",
                   *(volatile uint64_t *)daddr,
                   (uintptr_t)(0x1000ULL + 0x0F1E2D3C4B5A6978ULL));
#endif

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-02: VS-mode AMO set executes, never cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_02_vs_exec_no_cause22);
bool test_hzamo_02_vs_exec_no_cause22(void)
{
    TEST_BEGIN("HZAMO-02: VS-mode AMOs execute, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_all_amo_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VS-mode AMO set took no trap", !fired);
    TEST_ASSERT_NEQ("VS-mode AMO did not report cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-03: VU-mode AMO set executes, never cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_03_vu_exec_no_cause22);
bool test_hzamo_03_vu_exec_no_cause22(void)
{
    TEST_BEGIN("HZAMO-03: VU-mode AMOs execute, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;

    trap_expect_begin();
    (void)two_stage_run_in_vu(&ctx, hz_vs_all_amo_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VU-mode AMO set took no trap", !fired);
    TEST_ASSERT_NEQ("VU-mode AMO did not report cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-04: VS-mode AMO semantics match HS-mode: rd old value with .w
 *           sign-extension, rs2 high bits ignored, final memory equal.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_04_vs_hs_semantic_parity);
bool test_hzamo_04_vs_hs_semantic_parity(void)
{
    TEST_BEGIN("HZAMO-04: VS-mode AMO semantics == HS-mode");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* HS-mode run. */
    uintptr_t hs_addr = (uintptr_t)&hzamo_hs_slot[0];
    *(volatile uint32_t *)hs_addr = HZ_AMO_INIT_W;
    g_hz_amo_rd = g_hz_amo_mem = 0;
    (void)run_in_priv(PRIV_S, hz_amo_add_w_semantic, hs_addr);
    uintptr_t hs_rd = g_hz_amo_rd, hs_mem = g_hz_amo_mem;

    /* VS-mode run. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t vs_addr = (uintptr_t)test_data_area;
    *(volatile uint32_t *)vs_addr = HZ_AMO_INIT_W;
    g_hz_amo_rd = g_hz_amo_mem = 0;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_amo_add_w_semantic, vs_addr);
    bool fired = trap_was_triggered();
    trap_expect_end();
    uintptr_t vs_rd = g_hz_amo_rd, vs_mem = g_hz_amo_mem;
    ts2_finish(&ctx);

    TEST_ASSERT("VS-mode semantic run took no trap", !fired);
    /* .w AMO sign-extends the old value into rd (norm:amo_operand_size). */
    TEST_ASSERT_EQ("HS amoadd.w rd sign-extended old value",
                   hs_rd, (uintptr_t)(intptr_t)(int32_t)HZ_AMO_INIT_W);
    TEST_ASSERT_EQ("VS amoadd.w rd == HS rd", vs_rd, hs_rd);
    TEST_ASSERT_EQ("VS final memory == HS final memory", vs_mem, hs_mem);

    HYP_TEST_END();
}
