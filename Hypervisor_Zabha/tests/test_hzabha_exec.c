/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4.1 - HS/VS/VU-mode normal execution and cause=22 exclusion
 *             (HZABHA-01 ~ HZABHA-04)
 *
 * Spec basis:
 *   norm:Zabha_rd_sign_extension - byte/half AMOs sign-extend the old
 *     value into rd (byte -> 8-bit, half -> 16-bit) and ignore the
 *     XLEN-1:2^(width+3) bits of rs2. Unchanged under virtualization.
 *   norm:hlsv_virtinst - only HLV/HLVX/HSV raise virtual-instruction
 *     (cause=22); no hypervisor.adoc clause gates byte/half AMOs, so a
 *     guest byte/half AMO must NEVER report cause=22.
 * =================================================================== */

static volatile uint64_t hzabha_hs_slot[8];

/* ------------------------------------------------------------------
 * HZABHA-01: HS-mode executes all 9 AMOs for .b and .h normally.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_01_hs_exec);
bool test_hzabha_01_hs_exec(void)
{
    TEST_BEGIN("HZABHA-01: HS-mode all 9 AMOs (.b/.h) execute normally");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");

    /* First case ONLY: config declares Zabha, so probe the DUT to
     * confirm it really implements byte/halfword AMOs (aligned with the
     * ZABHA_SUPPORTED declaration). A trap-armed amoadd.b in M-mode
     * raising illegal-instruction would mean the DUT does NOT implement
     * Zabha despite the config. No other case probes. */
    {
        static volatile uint64_t hzabha_probe_slot;
        uintptr_t paddr = (uintptr_t)&hzabha_probe_slot;
        uintptr_t pr;
        M_TRAP_EXPECT_BEGIN();
        asm volatile(
            ".option push\n\t.option norvc\n\t"
            "amoadd.b %0, %2, (%1)\n\t"
            ".option pop\n\t"
            : "=r"(pr) : "r"(paddr), "r"(1UL) : "memory");
        bool ptrapped = trap_was_triggered();
        uintptr_t pcause = ptrapped ? trap_get_cause() : 0;
        trap_expect_end();
        TEST_ASSERT("DUT really implements Zabha (aligned with config)",
                    !(ptrapped && pcause == CAUSE_ILLEGAL_INST));
    }

    uintptr_t addr = (uintptr_t)&hzabha_hs_slot[0];
    *(volatile uint64_t *)addr = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hz_vs_all_amo_bh, addr);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("all 18 byte/half AMOs executed in HS-mode", !fired);

    /* rd sign-extension check: amoadd.b on a byte with bit7 set. */
    volatile uint8_t *bp = (volatile uint8_t *)&hzabha_hs_slot[4];
    *bp = 0x80u;
    uintptr_t rd_b = run_in_priv(PRIV_S, hz_vs_amo_add_b, (uintptr_t)bp);
    TEST_ASSERT_EQ("amoadd.b rd == sign-extended 8-bit old value",
                   rd_b, (uintptr_t)(intptr_t)(int8_t)0x80);

    volatile uint16_t *hp = (volatile uint16_t *)&hzabha_hs_slot[6];
    *hp = 0x8000u;
    uintptr_t rd_h = run_in_priv(PRIV_S, hz_vs_amo_add_h, (uintptr_t)hp);
    TEST_ASSERT_EQ("amoadd.h rd == sign-extended 16-bit old value",
                   rd_h, (uintptr_t)(intptr_t)(int16_t)0x8000);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-02: VS-mode byte/half AMO set executes, never cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_02_vs_exec_no_cause22);
bool test_hzabha_02_vs_exec_no_cause22(void)
{
    TEST_BEGIN("HZABHA-02: VS-mode byte/half AMOs execute, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_all_amo_bh, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VS-mode byte/half AMO set took no trap", !fired);
    TEST_ASSERT_NEQ("VS-mode byte/half AMO did not report cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-03: VU-mode byte/half AMO set executes, never cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_03_vu_exec_no_cause22);
bool test_hzabha_03_vu_exec_no_cause22(void)
{
    TEST_BEGIN("HZABHA-03: VU-mode byte/half AMOs execute, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)two_stage_run_in_vu(&ctx, hz_vs_all_amo_bh, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VU-mode byte/half AMO set took no trap", !fired);
    TEST_ASSERT_NEQ("VU-mode byte/half AMO did not report cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-04: VS-mode byte/half AMO semantics match HS-mode: rd 8/16-bit
 *            sign-extension and rs2 high-bit ignoring.
 * ------------------------------------------------------------------ */
static volatile uintptr_t g_hz_bh_rd;

static uintptr_t hzabha_bh_semantic_b(uintptr_t addr)
{
    uintptr_t rd = hz_vs_amo_add_b(addr);
    g_hz_bh_rd = rd;
    return rd;
}
static uintptr_t hzabha_bh_semantic_h(uintptr_t addr)
{
    uintptr_t rd = hz_vs_amo_add_h(addr);
    g_hz_bh_rd = rd;
    return rd;
}

TEST_REGISTER(test_hzabha_04_vs_hs_semantic_parity);
bool test_hzabha_04_vs_hs_semantic_parity(void)
{
    TEST_BEGIN("HZABHA-04: VS-mode byte/half AMO semantics == HS-mode");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* HS-mode byte run: old byte 0x81 -> rd sign-extended. */
    uintptr_t hs_addr = (uintptr_t)&hzabha_hs_slot[0];
    *(volatile uint8_t *)hs_addr = 0x81u;
    uintptr_t hs_rd_b = run_in_priv(PRIV_S, hzabha_bh_semantic_b, hs_addr);
    uintptr_t hs_rd = g_hz_bh_rd;

    /* VS-mode byte run. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t vs_addr = (uintptr_t)test_data_area;
    *(volatile uint8_t *)vs_addr = 0x81u;
    g_hz_bh_rd = 0;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hzabha_bh_semantic_b, vs_addr);
    bool fired = trap_was_triggered();
    trap_expect_end();
    uintptr_t vs_rd = g_hz_bh_rd;
    ts2_finish(&ctx);

    TEST_ASSERT("VS-mode byte AMO took no trap", !fired);
    TEST_ASSERT_EQ("HS amoadd.b rd sign-extended 8-bit old value",
                   hs_rd_b, (uintptr_t)(intptr_t)(int8_t)0x81);
    TEST_ASSERT_EQ("VS amoadd.b rd == HS rd", vs_rd, hs_rd);

    /* Half-word parity. */
    *(volatile uint16_t *)hs_addr = 0x8001u;
    uintptr_t hs_rd_h = run_in_priv(PRIV_S, hzabha_bh_semantic_h, hs_addr);
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint16_t *)vs_addr = 0x8001u;
    g_hz_bh_rd = 0;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hzabha_bh_semantic_h, vs_addr);
    bool fired_h = trap_was_triggered();
    trap_expect_end();
    uintptr_t vs_rd_h = g_hz_bh_rd;
    ts2_finish(&ctx);

    TEST_ASSERT("VS-mode half AMO took no trap", !fired_h);
    TEST_ASSERT_EQ("HS amoadd.h rd sign-extended 16-bit old value",
                   hs_rd_h, (uintptr_t)(intptr_t)(int16_t)0x8001);
    TEST_ASSERT_EQ("VS amoadd.h rd == HS rd", vs_rd_h, hs_rd_h);

    HYP_TEST_END();
}
