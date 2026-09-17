/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 3.6 - henvcfg.FIOM / ADUE interaction (HZACAS-27 ~ HZACAS-33)
 *
 * Spec basis:
 *   norm:henvcfg_fiom_order - FIOM=1 + V=1 orders an aq/rl atomic that
 *     accesses a device-I/O-ordered region as though it accesses both I/O
 *     and memory (multi-hart property; single hart checks executability).
 *   norm:henvcfg_adue_op - ADUE=0 => VS-stage behaves as Svade; ADUE=1
 *     (Svadu) => hardware updates A/D.
 *   A SUCCESS CAS always writes back, so ADUE=0 + D=0 => store page-fault
 *     (15), MANDATORY (HZACAS-30, analogue of HZAMO-28).
 *   A FAILED CAS's D-bit side effect is a SPEC GAP: unlike Zalrsc's
 *     norm:sc_failed_side_effects (explicitly UNSPECIFIED), zacas.adoc does
 *     not state the failed-CAS D-bit behaviour, so HZACAS-32/33 are
 *     record-type and note the normative difference.
 * =================================================================== */

#ifndef MENVCFG_ADUE
#define MENVCFG_ADUE   (1ULL << 61)
#endif
#ifndef HENVCFG_ADUE
#define HENVCFG_ADUE   (1ULL << 61)
#endif
#define HZ_HENVCFG_FIOM   (1UL << 0)

static void hzacas_fiom_case(int fiom, uintptr_t (*probe)(uintptr_t))
{
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;

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
    TEST_ASSERT("FIOM setting: annotated amocas executable (no trap)", !fired);
    TEST_ASSERT_NEQ("FIOM setting: no virtual-instruction (cause=22)",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
}

/* ------------------------------------------------------------------
 * HZACAS-27: FIOM=1, VS-mode aq/rl amocas executable, data semantics intact.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_27_fiom1);
bool test_hzacas_27_fiom1(void)
{
    TEST_BEGIN("HZACAS-27: FIOM=1 VS-mode amocas.w.aq executable");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzacas_fiom_case(1, hz_vs_amocas_w_aq);
    printf("  [INFO] FIOM ordering effect requires multi-hart: %s\n",
           HZACAS_SMP_SKIP_REASON);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-28: FIOM=0 control.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_28_fiom0);
bool test_hzacas_28_fiom0(void)
{
    TEST_BEGIN("HZACAS-28: FIOM=0 control VS-mode amocas.w.aq");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzacas_fiom_case(0, hz_vs_amocas_w_aq);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-29: ADUE=0, amocas to A=0 VS-stage page -> Svade store page
 *            fault (15), NOT a guest-page fault.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_29_adue0_a0);
bool test_hzacas_29_adue0_a0(void)
{
    TEST_BEGIN("HZACAS-29: ADUE=0 + A=0 amocas -> Svade store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
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
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("ADUE=0 A=0 amocas faulted (Svade)", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15, VS-stage Svade)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT("cause is NOT a guest-page fault (21/23)",
                cause != CAUSE_LOAD_GUEST_PAGE_FAULT &&
                cause != CAUSE_STORE_GUEST_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-30: ADUE=0, SUCCESS amocas to A=1/D=0 page -> store page-fault
 *            (15). A success CAS always writes, so the D-bit requirement
 *            is MANDATORY (analogue of HZAMO-28).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_30_adue0_d0_forced);
bool test_hzacas_30_adue0_d0_forced(void)
{
    TEST_BEGIN("HZACAS-30: ADUE=0 + A=1/D=0 success amocas -> store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
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
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;   /* match -> success path (writes) */
    hz_cas_swap = 0x00002000u;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("ADUE=0 D=0 success amocas faulted (CAS writes)", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15, D mandatory)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-31: ADUE=1 (Svadu), amocas to A=0/D=0 page -> hardware sets A/D,
 *            amocas completes with no fault. SKIP if no Svadu.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_31_adue1_hw_update);
bool test_hzacas_31_adue1_hw_update(void)
{
    TEST_BEGIN("HZACAS-31: ADUE=1 amocas -> hardware A/D update, no fault");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
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
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;

    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    if (!adue_on) {
        ts2_disable_adue();
        ts2_finish(&ctx);
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented");
    }

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    uintptr_t pte_after = *pte;
    ts2_disable_adue();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("ADUE=1 amocas completed with no page fault", !fired);
    TEST_ASSERT("hardware set PTE.A", (pte_after & PTE_A) != 0);
    TEST_ASSERT("hardware set PTE.D (success CAS writes)",
                (pte_after & PTE_D) != 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-32 (record-type): ADUE=1, FAILED CAS (compare mismatch) VS-stage
 *            D-bit side effect. SPEC GAP: zacas.adoc does not state this is
 *            UNSPECIFIED (unlike Zalrsc norm:sc_failed_side_effects), so
 *            both outcomes are recorded and the normative difference noted.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_32_failed_cas_d_side_effect);
bool test_hzacas_32_failed_cas_d_side_effect(void)
{
    TEST_BEGIN("HZACAS-32: (record) ADUE=1 failed-CAS VS-stage D side effect");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PTE resolvable", pte != NULL);
    if (pte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    uintptr_t ppn = (*pte) & ~((1UL << 10) - 1UL);
    *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A;   /* W=1, D=0 */
    hfence_vvma_all();
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00009999u;   /* mismatch -> guaranteed-failed CAS */
    hz_cas_swap = 0x00002000u;

    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    if (!adue_on) {
        ts2_disable_adue();
        ts2_finish(&ctx);
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented");
    }

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    trap_expect_end();

    uintptr_t d_after = (*pte) & PTE_D;
    ts2_disable_adue();
    ts2_finish(&ctx);

    printf("  [RECORD] failed-CAS VS-stage D bit after: %s (trap=%d); "
           "SPEC GAP - zacas.adoc does not state this is UNSPECIFIED "
           "(unlike Zalrsc norm:sc_failed_side_effects); both recorded\n",
           d_after ? "SET" : "clear", (int)fired);
    TEST_ASSERT("record-type case executed", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-33 (record-type): ADUE=1, FAILED CAS G-stage D-bit side effect.
 *            Same SPEC GAP as HZACAS-32, recorded for the G-stage PTE.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_33_failed_cas_gstage_d_side_effect);
bool test_hzacas_33_failed_cas_gstage_d_side_effect(void)
{
    TEST_BEGIN("HZACAS-33: (record) ADUE=1 failed-CAS G-stage D side effect");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
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
    hfence_vvma_all();
    hfence_gvma_all();
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00009999u;   /* mismatch -> guaranteed-failed CAS */
    hz_cas_swap = 0x00002000u;

    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    if (!adue_on) {
        ts2_disable_adue();
        ts2_finish(&ctx);
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented");
    }

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    trap_expect_end();

    uintptr_t gd_after = (*gpte) & PTE_D;
    ts2_disable_adue();
    ts2_finish(&ctx);

    printf("  [RECORD] failed-CAS G-stage D bit after: %s (trap=%d); "
           "SPEC GAP (no UNSPECIFIED statement in zacas.adoc); recorded\n",
           gd_after ? "SET" : "clear", (int)fired);
    TEST_ASSERT("record-type case executed", true);

    HYP_TEST_END();
}
