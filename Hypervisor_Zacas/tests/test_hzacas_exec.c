/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 3.1 - HS/VS/VU-mode normal execution and cause=22 exclusion
 *             (HZACAS-01 ~ HZACAS-04)
 *
 * Spec basis:
 *   norm:Zacas_rv64_amocas-d_op - amocas loads mem[rs1], compares with rd;
 *     if equal, stores rs2 to mem[rs1]; rd always receives the loaded
 *     value. These semantics are unchanged under virtualization.
 *   norm:hlsv_virtinst - only HLV/HLVX/HSV raise virtual-instruction
 *     (cause=22); no hypervisor.adoc clause gates amocas, so a guest
 *     amocas must NEVER report cause=22 (mandatory negative assertion).
 * =================================================================== */

/* 16-byte alignment is mandatory: amocas.q requires its operand address
 * to be naturally aligned to 16 bytes (norm:Zacas_amocas_rs1_addr_alignment).
 * A plain uint64_t[] only guarantees 8-byte alignment. */
static volatile uint64_t hzacas_hs_slot[8] __attribute__((aligned(16)));

/* ------------------------------------------------------------------
 * HZACAS-01: HS-mode amocas.w/d/q success and failure paths.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_01_hs_exec);
bool test_hzacas_01_hs_exec(void)
{
    TEST_BEGIN("HZACAS-01: HS-mode amocas.w/d/q success + failure");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");

    /* First case ONLY: config declares Zacas, so probe the DUT to
     * confirm it really implements amocas (aligned with the
     * ZACAS_SUPPORTED declaration). A trap-armed amocas.w in M-mode
     * raising illegal-instruction would mean the DUT does NOT implement
     * Zacas despite the config. No other case probes. */
    {
        static volatile uint64_t hzacas_probe_slot;
        uintptr_t paddr = (uintptr_t)&hzacas_probe_slot;
        uintptr_t prd = 0x1111ULL;   /* matches -> success path */
        uintptr_t psw = 0x2222ULL;
        hzacas_probe_slot = 0x1111ULL;
        M_TRAP_EXPECT_BEGIN();
        asm volatile(
            ".option push\n\t.option norvc\n\t"
            "amocas.w %0, %2, (%1)\n\t"
            ".option pop\n\t"
            : "+r"(prd) : "r"(paddr), "r"(psw) : "memory");
        bool ptrapped = trap_was_triggered();
        uintptr_t pcause = ptrapped ? trap_get_cause() : 0;
        trap_expect_end();
        TEST_ASSERT("DUT really implements Zacas (aligned with config)",
                    !(ptrapped && pcause == CAUSE_ILLEGAL_INST));
    }

    /* amocas.w success: cmp matches memory -> store swap, rd = old. */
    uintptr_t waddr = (uintptr_t)&hzacas_hs_slot[0];
    *(volatile uint32_t *)waddr = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;
    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hz_vs_amocas_w_probe, waddr);
    bool fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("amocas.w (success) executed without exception", !fired);
    TEST_ASSERT_EQ("amocas.w success: memory == swap value",
                   *(volatile uint32_t *)waddr, (uintptr_t)0x00002000u);
    TEST_ASSERT_EQ("amocas.w success: rd == old value",
                   hz_cas_loaded, (uintptr_t)0x00001000u);

    /* amocas.w failure: cmp mismatches -> no store, rd = loaded. */
    *(volatile uint32_t *)waddr = 0x00003000u;
    hz_cas_cmp = 0x00009999u;   /* mismatch */
    hz_cas_swap = 0x00004000u;
    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hz_vs_amocas_w_probe, waddr);
    bool fired_f = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("amocas.w (failure) executed without exception", !fired_f);
    TEST_ASSERT_EQ("amocas.w failure: memory unchanged",
                   *(volatile uint32_t *)waddr, (uintptr_t)0x00003000u);
    TEST_ASSERT_EQ("amocas.w failure: rd == loaded value",
                   hz_cas_loaded, (uintptr_t)0x00003000u);

#if __riscv_xlen == 64
    /* amocas.d success. */
    uintptr_t daddr = (uintptr_t)&hzacas_hs_slot[4];
    *(volatile uint64_t *)daddr = 0x0000000010000000ULL;
    hz_cas_cmp = 0x0000000010000000ULL;
    hz_cas_swap = 0x0000000020000000ULL;
    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hz_vs_amocas_d_probe, daddr);
    bool fired_d = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("amocas.d (success) executed without exception", !fired_d);
    TEST_ASSERT_EQ("amocas.d success: memory == swap value",
                   *(volatile uint64_t *)daddr,
                   (uintptr_t)0x0000000020000000ULL);

    /* amocas.q success (register pair). */
    uintptr_t qaddr = (uintptr_t)&hzacas_hs_slot[0];   /* 16-byte span */
    *(volatile uint64_t *)(qaddr + 0) = 0x0000000000000011ULL;
    *(volatile uint64_t *)(qaddr + 8) = 0x0000000000000022ULL;
    hz_cas_cmp = 0x0000000000000011ULL;
    hz_cas_cmp_hi = 0x0000000000000022ULL;
    hz_cas_swap = 0x0000000000000033ULL;
    hz_cas_swap_hi = 0x0000000000000044ULL;
    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hz_vs_amocas_q_probe, qaddr);
    bool fired_q = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("amocas.q (success) executed without exception", !fired_q);
    TEST_ASSERT_EQ("amocas.q success: memory lo == swap lo",
                   *(volatile uint64_t *)(qaddr + 0),
                   (uintptr_t)0x0000000000000033ULL);
    TEST_ASSERT_EQ("amocas.q success: memory hi == swap hi",
                   *(volatile uint64_t *)(qaddr + 8),
                   (uintptr_t)0x0000000000000044ULL);
#endif

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-02: VS-mode amocas executes, never cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_02_vs_exec_no_cause22);
bool test_hzacas_02_vs_exec_no_cause22(void)
{
    TEST_BEGIN("HZACAS-02: VS-mode amocas executes, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0x00005000u;
    hz_cas_cmp = 0x00005000u;
    hz_cas_swap = 0x00006000u;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VS-mode amocas took no trap", !fired);
    TEST_ASSERT_NEQ("VS-mode amocas did not report cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("VS-mode amocas stored the swap value",
                   *(volatile uint32_t *)va, (uintptr_t)0x00006000u);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-03: VU-mode amocas.w/d executes, never cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_03_vu_exec_no_cause22);
bool test_hzacas_03_vu_exec_no_cause22(void)
{
    TEST_BEGIN("HZACAS-03: VU-mode amocas executes, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0x00007000u;
    hz_cas_cmp = 0x00007000u;
    hz_cas_swap = 0x00008000u;

    trap_expect_begin();
    (void)two_stage_run_in_vu(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VU-mode amocas took no trap", !fired);
    TEST_ASSERT_NEQ("VU-mode amocas did not report cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-04: VS-mode amocas semantics match HS-mode (success + failure,
 *            .w sign-extension, memory final value).
 * ------------------------------------------------------------------ */
static volatile uintptr_t g_hz_cas_mem;

static uintptr_t hzacas_cas_semantic(uintptr_t addr)
{
    uintptr_t rd = hz_vs_amocas_w_probe(addr);
    g_hz_cas_mem = *(volatile uint32_t *)addr;
    return rd;
}

TEST_REGISTER(test_hzacas_04_vs_hs_semantic_parity);
bool test_hzacas_04_vs_hs_semantic_parity(void)
{
    TEST_BEGIN("HZACAS-04: VS-mode amocas semantics == HS-mode");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* HS-mode success run. */
    uintptr_t hs_addr = (uintptr_t)&hzacas_hs_slot[0];
    *(volatile uint32_t *)hs_addr = 0x00001234u;
    hz_cas_cmp = 0x00001234u;
    hz_cas_swap = 0x00005678u;
    g_hz_cas_mem = 0;
    uintptr_t hs_rd = run_in_priv(PRIV_S, hzacas_cas_semantic, hs_addr);
    uintptr_t hs_mem = g_hz_cas_mem;

    /* VS-mode success run. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t vs_addr = (uintptr_t)test_data_area;
    *(volatile uint32_t *)vs_addr = 0x00001234u;
    hz_cas_cmp = 0x00001234u;
    hz_cas_swap = 0x00005678u;
    g_hz_cas_mem = 0;
    trap_expect_begin();
    uintptr_t vs_rd = two_stage_run_in_vs(&ctx, hzacas_cas_semantic, vs_addr);
    bool fired = trap_was_triggered();
    trap_expect_end();
    uintptr_t vs_mem = g_hz_cas_mem;
    ts2_finish(&ctx);

    TEST_ASSERT("VS-mode semantic run took no trap", !fired);
    TEST_ASSERT_EQ("VS amocas rd == HS rd (loaded old value)", vs_rd, hs_rd);
    TEST_ASSERT_EQ("VS amocas rd == old value", vs_rd, (uintptr_t)0x00001234u);
    TEST_ASSERT_EQ("VS final memory == HS final memory", vs_mem, hs_mem);
    TEST_ASSERT_EQ("final memory == swap value", vs_mem, (uintptr_t)0x00005678u);

    HYP_TEST_END();
}
