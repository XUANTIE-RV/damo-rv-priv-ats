/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.7 - henvcfg.FIOM / ADUE interaction (HZLRSC-30 ~ HZLRSC-34)
 *
 * Spec basis:
 *   norm:henvcfg_fiom_order - when FIOM=1 and V=1, an atomic instruction
 *     with aq/rl accessing a device-I/O-ordered region is ordered as
 *     though it accesses both device I/O and memory. The ordering effect
 *     is a multi-hart property; a single hart verifies only that the
 *     instruction executes and its data semantics are unchanged.
 *   norm:sc_failed_side_effects - whether a FAILED SC produces implicit
 *     translation side effects (e.g. setting a PTE D bit) is UNSPECIFIED.
 *   norm:henvcfg_adue_op - ADUE=0 => VS-stage behaves as Svade (A/D
 *     violations raise page faults); ADUE=1 (Svadu) => hardware updates.
 * =================================================================== */

#ifndef MENVCFG_ADUE
#define MENVCFG_ADUE   (1ULL << 61)
#endif
#ifndef HENVCFG_ADUE
#define HENVCFG_ADUE   (1ULL << 61)
#endif
#define HZ_HENVCFG_FIOM   (1UL << 0)

/* lr.w.aq + sc.w.rl pair (FIOM ordering variants). */
static uintptr_t hz_vs_lrsc_w_aq_rl(uintptr_t addr)
{
    uintptr_t lr, sc = 1;
    for (int i = 0; i < HZLRSC_PAIR_TRIES && sc != 0; i++)
        HZ_PAIR_W(".aq", ".rl", lr, sc, addr, 0x12345678UL);
    return sc;
}

/* Shared FIOM executability + data-semantics check. */
static void hzlrsc_fiom_case(int fiom)
{
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t he = henvcfg_read();
    if (fiom) he |= HZ_HENVCFG_FIOM; else he &= ~HZ_HENVCFG_FIOM;
    henvcfg_write(he);

    trap_expect_begin();
    uintptr_t sc = two_stage_run_in_vs(&ctx, hz_vs_lrsc_w_aq_rl, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    uintptr_t mem = *(volatile uint32_t *)va;
    henvcfg_write(henvcfg_read() & ~HZ_HENVCFG_FIOM);
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("FIOM setting: lr.w.aq/sc.w.rl executable (no trap)", !fired);
    TEST_ASSERT_NEQ("FIOM setting: no virtual-instruction (cause=22)",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("FIOM setting: SC succeeded (data semantics unchanged)",
                   sc, (uintptr_t)0);
    TEST_ASSERT_EQ("FIOM setting: memory updated",
                   mem, (uintptr_t)0x12345678UL);
    /* The FIOM *ordering* effect is a multi-hart property; the framework
     * runs only hart 0, so it is not asserted here (see plan NOTE). */
}

/* ------------------------------------------------------------------
 * HZLRSC-30: FIOM=1, VS-mode annotated LR/SC executable, data unchanged.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_30_fiom1);
bool test_hzlrsc_30_fiom1(void)
{
    TEST_BEGIN("HZLRSC-30: FIOM=1 VS-mode lr.w.aq/sc.w.rl executable");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzlrsc_fiom_case(1);
    printf("  [INFO] FIOM ordering effect requires multi-hart: %s\n",
           HZLRSC_SMP_SKIP_REASON);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-31: FIOM=0 control - same executability and data semantics.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_31_fiom0);
bool test_hzlrsc_31_fiom0(void)
{
    TEST_BEGIN("HZLRSC-31: FIOM=0 control VS-mode lr.w.aq/sc.w.rl");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzlrsc_fiom_case(0);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-32 / HZLRSC-33 (record-type): ADUE=1, failed SC's VS-stage and
 * G-stage PTE D-bit side effect is UNSPECIFIED (norm:sc_failed_side_effects).
 * Record the implementation choice; make no mandatory judgement.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_32_failed_sc_d_side_effect);
bool test_hzlrsc_32_failed_sc_d_side_effect(void)
{
    TEST_BEGIN("HZLRSC-32: (record) ADUE=1 failed-SC VS-stage D side effect");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    /* VS leaf: W=1, A=1, D=0 so a store would need a D update. */
    uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PTE resolvable", pte != NULL);
    if (pte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    uintptr_t ppn = (*pte) & ~((1UL << 10) - 1UL);
    *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A;   /* D=0 */

    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    if (!adue_on) {
        ts2_disable_adue();
        ts2_finish(&ctx);
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented");
    }

    /* Failed SC (no preceding LR): must not write memory, but whether it
     * sets the VS-stage D bit is UNSPECIFIED. */
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sc_w, va);
    bool fired = trap_was_triggered();
    trap_expect_end();

    uintptr_t d_after = (*pte) & PTE_D;
    ts2_disable_adue();
    ts2_finish(&ctx);

    printf("  [RECORD] failed-SC VS-stage D bit after: %s (trap=%d); "
           "both set and clear are UNSPECIFIED-compliant\n",
           d_after ? "SET" : "clear", (int)fired);
    /* Record-type: no mandatory assertion on the D-bit outcome. */
    TEST_ASSERT("record-type case executed", true);

    HYP_TEST_END();
}

TEST_REGISTER(test_hzlrsc_33_failed_sc_gstage_d_side_effect);
bool test_hzlrsc_33_failed_sc_gstage_d_side_effect(void)
{
    TEST_BEGIN("HZLRSC-33: (record) ADUE=1 failed-SC G-stage D side effect");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;   /* GPA == VA (identity) */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    uintptr_t *vspte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    uintptr_t *gpte = gpt_get_pte(&ctx.g_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS + G leaf PTEs resolvable", vspte != NULL && gpte != NULL);
    if (vspte == NULL || gpte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    uintptr_t vsppn = (*vspte) & ~((1UL << 10) - 1UL);
    *vspte = vsppn | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A;   /* D=0 */
    uintptr_t gppn = (*gpte) & ~((1UL << 10) - 1UL);
    *gpte = gppn | PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A;  /* D=0 */

    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    if (!adue_on) {
        ts2_disable_adue();
        ts2_finish(&ctx);
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented");
    }

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sc_w, va);
    bool fired = trap_was_triggered();
    trap_expect_end();

    uintptr_t gd_after = (*gpte) & PTE_D;
    ts2_disable_adue();
    ts2_finish(&ctx);

    printf("  [RECORD] failed-SC G-stage D bit after: %s (trap=%d); "
           "UNSPECIFIED (each stage independently)\n",
           gd_after ? "SET" : "clear", (int)fired);
    TEST_ASSERT("record-type case executed", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-34: ADUE=0, VS-stage PTE A=0 -> Svade page fault (LR->13,
 *            SC->15), NOT a guest-page fault (21/23).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_34_adue0_svade);
bool test_hzlrsc_34_adue0_svade(void)
{
    TEST_BEGIN("HZLRSC-34: ADUE=0 + A=0 -> Svade pf (LR 13 / SC 15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;

    /* LR (load) with A=0 -> load page-fault (13). */
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
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("ADUE=0 A=0 LR faulted (Svade)", fired);
    TEST_ASSERT_EQ("LR cause == load page-fault (13, VS-stage Svade)",
                   cause, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT("LR cause is NOT a guest-page fault (21/23)",
                cause != CAUSE_LOAD_GUEST_PAGE_FAULT &&
                cause != CAUSE_STORE_GUEST_PAGE_FAULT);

    /* SC (store) with A=0 -> store page-fault (15). */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    ts2_disable_adue();
    pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    if (pte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    ppn = (*pte) & ~((1UL << 10) - 1UL);
    *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X;   /* A=0, D=0 */
    hfence_vvma_all();

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sc_w, va);
    bool fired2 = trap_was_triggered();
    uintptr_t cause2 = fired2 ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("ADUE=0 A=0 SC faulted (Svade)", fired2);
    TEST_ASSERT_EQ("SC cause == store/AMO page-fault (15, VS-stage Svade)",
                   cause2, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT("SC cause is NOT a guest-page fault (21/23)",
                cause2 != CAUSE_LOAD_GUEST_PAGE_FAULT &&
                cause2 != CAUSE_STORE_GUEST_PAGE_FAULT);

    HYP_TEST_END();
}
