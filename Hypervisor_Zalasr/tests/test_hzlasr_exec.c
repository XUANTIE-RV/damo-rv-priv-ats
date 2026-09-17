/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.1 - HS/VS/VU-mode normal execution and cause=22 exclusion
 *             (HZLASR-01 ~ HZLASR-04)
 *
 * Spec basis:
 *   norm:zalasr_atomic_ordered / norm:ldaq_atomic_load_op /
 *   norm:sdrl_atomic_store_op - Zalasr instructions are standalone atomic
 *     ordered loads/stores; load-acquire only reads (into rd, sign
 *     extended), store-release only writes (the low bits of rs2). Their
 *     architecture-visible semantics are unchanged under virtualization.
 *   norm:zalasr_signext_rd / norm:ldaq_signext_rule - a load-acquire whose
 *     operand is narrower than XLEN sign-extends the value into rd.
 *   norm:zalasr_ignore_rs2_upper - a store-release ignores the upper bits
 *     of rs2.
 *   norm:hlsv_virtinst - only HLV/HLVX/HSV raise virtual-instruction
 *     (cause=22); no hypervisor.adoc clause gates load-acquire /
 *     store-release, so a guest Zalasr instruction must NEVER report
 *     cause=22.
 *
 * This sub-group is the SOLE authoritative source for the HS/VS/VU-mode
 * executability of Zalasr (the former Zalasr_test_plan.md Group 7 cases
 * ZALASR-52~54 were migrated here).
 * =================================================================== */

static volatile uint64_t hzlasr_hs_slot[8];

/* ------------------------------------------------------------------
 * HZLASR-01: HS-mode executes the load-acquire / store-release set
 *            normally; rd sign-extension and rs2-high-ignore hold.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_01_hs_exec);
bool test_hzlasr_01_hs_exec(void)
{
    TEST_BEGIN("HZLASR-01: HS-mode load-acquire/store-release execute normally");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");

    /* First case ONLY: config declares Zalasr, so probe the DUT to
     * confirm it really implements load-acquire (aligned with the
     * ZALASR_SUPPORTED declaration). A trap-armed lw.aq in M-mode
     * raising illegal-instruction would mean the DUT does NOT implement
     * Zalasr despite the config. No other case probes. */
    {
        static volatile uint64_t hzlasr_probe_slot;
        uintptr_t paddr = (uintptr_t)&hzlasr_probe_slot;
        uintptr_t pr = 0;
        hzlasr_store_le32(paddr, 0x00005678u);
        M_TRAP_EXPECT_BEGIN();
        ZALASR_LOAD(ZALASR_F3_W, ZALASR_F7_LD_AQ, pr, paddr);
        bool ptrapped = trap_was_triggered();
        uintptr_t pcause = ptrapped ? trap_get_cause() : 0;
        trap_expect_end();
        (void)pr;
        TEST_ASSERT("DUT really implements Zalasr (aligned with config)",
                    !(ptrapped && pcause == CAUSE_ILLEGAL_INST));
    }

    uintptr_t addr = (uintptr_t)&hzlasr_hs_slot[0];
    hzlasr_store_le64(addr, 0x0011223344556677ULL);
    hz_st_val = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)run_in_priv(PRIV_S, hz_all_load_acq, addr);
    (void)run_in_priv(PRIV_S, hz_all_store_rel, addr);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("HS-mode load-acquire/store-release set took no trap", !fired);

    /* load-acquire rd sign-extension: a byte with bit7 set. */
    uintptr_t baddr = (uintptr_t)&hzlasr_hs_slot[4];
    hzlasr_store8(baddr, 0x80u);
    uintptr_t rd_b = run_in_priv(PRIV_S, hz_vs_lb_aq, baddr);
    TEST_ASSERT_EQ("lb.aq rd == sign-extended 8-bit loaded value",
                   rd_b, ZALASR_B_SIGNEXT(0x80u));

    /* store-release writes the low bits of rs2, ignoring the upper bits. */
    uintptr_t saddr = (uintptr_t)&hzlasr_hs_slot[6];
    hzlasr_store8(saddr, 0x00u);
    hz_st_val = 0xDEADBEEFCAFEBABEULL;
    (void)run_in_priv(PRIV_S, hz_vs_sb_rl, saddr);
    TEST_ASSERT_EQ("sb.rl stored rs2 low 8 bits (upper ignored)",
                   (uintptr_t)hzlasr_load8(saddr), (uintptr_t)0xBEu);
    hz_st_val = 0x0011223344556677ULL;

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-02: VS-mode Zalasr set executes, never cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_02_vs_exec_no_cause22);
bool test_hzlasr_02_vs_exec_no_cause22(void)
{
    TEST_BEGIN("HZLASR-02: VS-mode load-acquire/store-release, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    hzlasr_store_le64(va, 0x0011223344556677ULL);
    hz_st_val = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_all_zalasr, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VS-mode Zalasr set took no trap", !fired);
    TEST_ASSERT_NEQ("VS-mode Zalasr did not report cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-03: VU-mode Zalasr set executes, never cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_03_vu_exec_no_cause22);
bool test_hzlasr_03_vu_exec_no_cause22(void)
{
    TEST_BEGIN("HZLASR-03: VU-mode load-acquire/store-release, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    hzlasr_store_le64(va, 0x0011223344556677ULL);
    hz_st_val = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)two_stage_run_in_vu(&ctx, hz_all_zalasr, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VU-mode Zalasr set took no trap", !fired);
    TEST_ASSERT_NEQ("VU-mode Zalasr did not report cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-04: VS-mode Zalasr semantics match HS-mode: load-acquire rd
 *            sign-extension, store-release rs2-high-ignore.
 * ------------------------------------------------------------------ */
static volatile uintptr_t g_hz_ld_rd;

static inline uintptr_t hzlasr_sem_lb(uintptr_t addr)
{
    uintptr_t rd = hz_vs_lb_aq(addr);
    g_hz_ld_rd = rd;
    return rd;
}
static inline uintptr_t hzlasr_sem_sb(uintptr_t addr)
{
    (void)hz_vs_sb_rl(addr);
    return 0;
}

TEST_REGISTER(test_hzlasr_04_vs_hs_semantic_parity);
bool test_hzlasr_04_vs_hs_semantic_parity(void)
{
    TEST_BEGIN("HZLASR-04: VS-mode Zalasr semantics == HS-mode");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* (1) load-acquire byte sign-extension parity. */
    uintptr_t hs_addr = (uintptr_t)&hzlasr_hs_slot[0];
    hzlasr_store8(hs_addr, 0x81u);
    uintptr_t hs_rd = run_in_priv(PRIV_S, hzlasr_sem_lb, hs_addr);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t vs_addr = (uintptr_t)test_data_area;
    hzlasr_store8(vs_addr, 0x81u);
    g_hz_ld_rd = 0;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hzlasr_sem_lb, vs_addr);
    bool fired_l = trap_was_triggered();
    trap_expect_end();
    uintptr_t vs_rd = g_hz_ld_rd;
    ts2_finish(&ctx);

    TEST_ASSERT("VS-mode lb.aq took no trap", !fired_l);
    TEST_ASSERT_EQ("HS lb.aq rd sign-extended 8-bit value",
                   hs_rd, ZALASR_B_SIGNEXT(0x81u));
    TEST_ASSERT_EQ("VS lb.aq rd == HS rd (sign extension unchanged)",
                   vs_rd, hs_rd);

    /* (2) store-release rs2-high-ignore parity. */
    hz_st_val = 0xDEADBEEFCAFE005Au;   /* only the low byte 0x5A is stored */
    hzlasr_store8(hs_addr, 0x00u);
    (void)run_in_priv(PRIV_S, hzlasr_sem_sb, hs_addr);
    uintptr_t hs_mem = hzlasr_load8(hs_addr);

    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzlasr_store8(vs_addr, 0x00u);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hzlasr_sem_sb, vs_addr);
    bool fired_s = trap_was_triggered();
    trap_expect_end();
    uintptr_t vs_mem = hzlasr_load8(vs_addr);
    ts2_finish(&ctx);

    TEST_ASSERT("VS-mode sb.rl took no trap", !fired_s);
    TEST_ASSERT_EQ("HS sb.rl stored rs2 low 8 bits (upper ignored)",
                   hs_mem, (uintptr_t)0x5Au);
    TEST_ASSERT_EQ("VS sb.rl stored value == HS", vs_mem, hs_mem);

    hz_st_val = 0x0011223344556677ULL;
    HYP_TEST_END();
}
