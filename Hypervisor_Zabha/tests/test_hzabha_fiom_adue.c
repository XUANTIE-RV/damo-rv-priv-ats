/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4.6 - henvcfg.FIOM / ADUE interaction (HZABHA-27 ~ HZABHA-31)
 *
 * Spec basis:
 *   norm:henvcfg_fiom_order - FIOM=1 + V=1 orders an aq/rl atomic that
 *     accesses a device-I/O-ordered region (multi-hart property; single
 *     hart checks executability).
 *   norm:henvcfg_adue_op - ADUE=0 => VS-stage behaves as Svade; ADUE=1
 *     (Svadu) => hardware updates A/D.
 *   A byte/half AMO ALWAYS writes back, so ADUE=0 + D=0 => store page
 *   fault (15), MANDATORY (HZABHA-30, analogue of HZAMO-28).
 * =================================================================== */

#ifndef MENVCFG_ADUE
#define MENVCFG_ADUE   (1ULL << 61)
#endif
#ifndef HENVCFG_ADUE
#define HENVCFG_ADUE   (1ULL << 61)
#endif
#define HZ_HENVCFG_FIOM   (1UL << 0)

static void hzabha_fiom_case(int fiom, uintptr_t (*probe)(uintptr_t))
{
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

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
    TEST_ASSERT("FIOM setting: annotated byte/half AMO executable", !fired);
    TEST_ASSERT_NEQ("FIOM setting: no virtual-instruction (cause=22)",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
}

/* ------------------------------------------------------------------
 * HZABHA-27: FIOM=1, VS-mode aq/rl byte/half AMO executable.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_27_fiom1);
bool test_hzabha_27_fiom1(void)
{
    TEST_BEGIN("HZABHA-27: FIOM=1 VS-mode amoadd.b.aq executable");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzabha_fiom_case(1, hz_vs_amo_add_b_aq);
    printf("  [INFO] FIOM ordering effect requires multi-hart: %s\n",
           HZABHA_SMP_SKIP_REASON);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-28: FIOM=0 control.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_28_fiom0);
bool test_hzabha_28_fiom0(void)
{
    TEST_BEGIN("HZABHA-28: FIOM=0 control VS-mode amoadd.b.aq");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzabha_fiom_case(0, hz_vs_amo_add_b_aq);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-29: ADUE=0, byte/half AMO to A=0 VS-stage page -> Svade store
 *            page fault (15), NOT a guest-page fault.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_29_adue0_a0);
bool test_hzabha_29_adue0_a0(void)
{
    TEST_BEGIN("HZABHA-29: ADUE=0 + A=0 byte/half AMO -> Svade store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
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
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_b, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("ADUE=0 A=0 byte AMO faulted (Svade)", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15, VS-stage Svade)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT("cause is NOT a guest-page fault (21/23)",
                cause != CAUSE_LOAD_GUEST_PAGE_FAULT &&
                cause != CAUSE_STORE_GUEST_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-30: ADUE=0, byte/half AMO to A=1/D=0 page -> store page-fault
 *            (15). A byte/half AMO always writes, so the D-bit requirement
 *            is MANDATORY (analogue of HZAMO-28).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_30_adue0_d0_forced);
bool test_hzabha_30_adue0_d0_forced(void)
{
    TEST_BEGIN("HZABHA-30: ADUE=0 + A=1/D=0 byte/half AMO -> store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
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
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_h, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("ADUE=0 D=0 half AMO faulted (AMO always writes)", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15, D mandatory)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-31: ADUE=1 (Svadu), byte/half AMO to A=0/D=0 page -> hardware
 *            sets A/D, AMO completes with no fault. SKIP if no Svadu.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_31_adue1_hw_update);
bool test_hzabha_31_adue1_hw_update(void)
{
    TEST_BEGIN("HZABHA-31: ADUE=1 byte/half AMO -> hardware A/D update");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
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
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    if (!adue_on) {
        ts2_disable_adue();
        ts2_finish(&ctx);
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented");
    }

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_b, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    uintptr_t pte_after = *pte;
    ts2_disable_adue();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("ADUE=1 byte AMO completed with no page fault", !fired);
    TEST_ASSERT("hardware set PTE.A", (pte_after & PTE_A) != 0);
    TEST_ASSERT("hardware set PTE.D (AMO writes)", (pte_after & PTE_D) != 0);

    HYP_TEST_END();
}
