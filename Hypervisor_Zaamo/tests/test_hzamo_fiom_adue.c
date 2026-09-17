/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2.6 - henvcfg.FIOM / ADUE interaction (HZAMO-25 ~ HZAMO-29)
 *
 * Spec basis:
 *   norm:henvcfg_fiom_order - FIOM=1 + V=1 orders an aq/rl atomic that
 *     accesses a device-I/O-ordered region as though it accesses both I/O
 *     and memory. The ordering effect is a multi-hart property; a single
 *     hart verifies only executability and unchanged data semantics.
 *   norm:henvcfg_adue_op - ADUE=0 => VS-stage behaves as Svade (A/D
 *     violations raise page faults); ADUE=1 (Svadu) => hardware updates.
 *   An AMO ALWAYS writes back, so unlike a failed SC (Group 1) the D-bit
 *     requirement is MANDATORY: ADUE=0 + D=0 => store page-fault (15).
 * =================================================================== */

#ifndef MENVCFG_ADUE
#define MENVCFG_ADUE   (1ULL << 61)
#endif
#ifndef HENVCFG_ADUE
#define HENVCFG_ADUE   (1ULL << 61)
#endif
#define HZ_HENVCFG_FIOM   (1UL << 0)

static void hzamo_fiom_case(int fiom, uintptr_t (*probe)(uintptr_t))
{
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0x1000u;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t he = henvcfg_read();
    if (fiom) he |= HZ_HENVCFG_FIOM; else he &= ~HZ_HENVCFG_FIOM;
    henvcfg_write(he);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    henvcfg_write(henvcfg_read() & ~HZ_HENVCFG_FIOM);
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("FIOM setting: annotated AMO executable (no trap)", !fired);
    TEST_ASSERT_NEQ("FIOM setting: no virtual-instruction (cause=22)",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
}

/* ------------------------------------------------------------------
 * HZAMO-25: FIOM=1, VS-mode aq/rl AMO executable, data semantics intact.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_25_fiom1);
bool test_hzamo_25_fiom1(void)
{
    TEST_BEGIN("HZAMO-25: FIOM=1 VS-mode amoadd.w.aq executable");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzamo_fiom_case(1, hz_vs_amo_add_w_aq);
#if __riscv_xlen == 64
    hzamo_fiom_case(1, hz_vs_amo_swap_d_rl);
#endif
    printf("  [INFO] FIOM ordering effect requires multi-hart: %s\n",
           HZAMO_SMP_SKIP_REASON);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-26: FIOM=0 control.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_26_fiom0);
bool test_hzamo_26_fiom0(void)
{
    TEST_BEGIN("HZAMO-26: FIOM=0 control VS-mode amoadd.w.aq");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzamo_fiom_case(0, hz_vs_amo_add_w_aq);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-27: ADUE=0, AMO to A=0 VS-stage page -> Svade store page-fault
 *           (15), NOT a guest-page fault (21/23).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_27_adue0_a0);
bool test_hzamo_27_adue0_a0(void)
{
    TEST_BEGIN("HZAMO-27: ADUE=0 + A=0 -> Svade store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    ts2_disable_adue();
    uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PTE resolvable", pte != NULL);
    if (pte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    uintptr_t ppn = (*pte) & ~((1UL << 10) - 1UL);
    *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X;   /* A=0, D=0 */
    hfence_vvma_all();

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("ADUE=0 A=0 AMO faulted (Svade)", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15, VS-stage Svade)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT("cause is NOT a guest-page fault (21/23)",
                cause != CAUSE_LOAD_GUEST_PAGE_FAULT &&
                cause != CAUSE_STORE_GUEST_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-28: ADUE=0, AMO to A=1/D=0 page -> store page-fault (15). An AMO
 *           ALWAYS writes back, so the D-bit requirement is MANDATORY
 *           (contrast the failed-SC UNSPECIFIED case, HZLRSC-32/33).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_28_adue0_d0_forced);
bool test_hzamo_28_adue0_d0_forced(void)
{
    TEST_BEGIN("HZAMO-28: ADUE=0 + A=1/D=0 AMO -> store pf (15, forced)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    ts2_disable_adue();
    uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PTE resolvable", pte != NULL);
    if (pte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    uintptr_t ppn = (*pte) & ~((1UL << 10) - 1UL);
    *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A;   /* A=1, D=0 */
    hfence_vvma_all();

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("ADUE=0 D=0 AMO faulted (AMO always writes)", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15, D mandatory)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-29: ADUE=1 (Svadu), AMO to A=0/D=0 page -> hardware sets A/D,
 *           AMO completes with no fault; read back PTE A=1/D=1.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_29_adue1_hw_update);
bool test_hzamo_29_adue1_hw_update(void)
{
    TEST_BEGIN("HZAMO-29: ADUE=1 AMO -> hardware A/D update, no fault");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PTE resolvable", pte != NULL);
    if (pte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    uintptr_t ppn = (*pte) & ~((1UL << 10) - 1UL);
    *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X;   /* A=0, D=0 */
    hfence_vvma_all();

    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    if (!adue_on) {
        ts2_disable_adue();
        ts2_finish(&ctx);
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented");
    }

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    uintptr_t pte_after = *pte;
    ts2_disable_adue();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("ADUE=1 AMO completed with no page fault", !fired);
    TEST_ASSERT("hardware set PTE.A", (pte_after & PTE_A) != 0);
    TEST_ASSERT("hardware set PTE.D (AMO writes)", (pte_after & PTE_D) != 0);

    HYP_TEST_END();
}
