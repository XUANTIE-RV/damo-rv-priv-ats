/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.6 - henvcfg.FIOM / ADUE interaction (HZLASR-29 ~ HZLASR-33)
 *
 * Spec basis:
 *   norm:henvcfg_fiom_order - FIOM=1 + V=1 orders an aq/rl atomic that
 *     accesses a device-I/O-ordered region. Zalasr ALWAYS carries aq/rl
 *     (norm:ldaq_aq_required / norm:sdrl_rl_required), so the ordering
 *     modification is always applicable without constructing a specific
 *     aq/rl variant (unlike Groups 1-4). The ordering effect is a
 *     multi-hart property; a single hart checks executability + data
 *     semantics only.
 *   norm:henvcfg_adue_op - ADUE=0 => VS-stage behaves as Svade (raise a
 *     page-fault instead of a hardware A/D update); ADUE=1 (Svadu) =>
 *     hardware updates A/D.
 *   A load-acquire is a PURE read: it only touches A, never the data-page
 *   D bit (HZLASR-31/33). A store-release ALWAYS writes, so ADUE=0 + D=0
 *   => store page-fault (15), MANDATORY (HZLASR-32, analogue of HZAMO-28).
 * =================================================================== */

#define HZ_HENVCFG_FIOM   (1UL << 0)

static volatile uintptr_t g_hz_fiom_rd;

/* Combined load-acquire + store-release probe for the FIOM cases: reads
 * the preset value first, then overwrites it. Both must execute. */
static inline uintptr_t hzlasr_fiom_probe(uintptr_t addr)
{
    uintptr_t rd = hz_vs_lw_aq(addr);
    g_hz_fiom_rd = rd;
    (void)hz_vs_sw_rl(addr);
    return rd;
}

static void hzlasr_fiom_case(int fiom)
{
    uintptr_t va = (uintptr_t)test_data_area;
    hzlasr_store_le32(va, 0x00005678u);
    hz_st_val = 0x0000000000009abcULL;
    g_hz_fiom_rd = 0;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t he = henvcfg_read();
    if (fiom) he |= HZ_HENVCFG_FIOM; else he &= ~HZ_HENVCFG_FIOM;
    henvcfg_write(he);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hzlasr_fiom_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    henvcfg_write(henvcfg_read() & ~HZ_HENVCFG_FIOM);
    uintptr_t rd = g_hz_fiom_rd;
    uintptr_t mem = hzlasr_load_le32(va);
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("FIOM setting: load-acquire/store-release executable", !fired);
    TEST_ASSERT_NEQ("FIOM setting: no virtual-instruction (cause=22)",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("FIOM setting: lw.aq data semantics unchanged",
                   rd, ZALASR_W_SIGNEXT(0x00005678u));
    TEST_ASSERT_EQ("FIOM setting: sw.rl wrote the value",
                   mem, (uintptr_t)0x00009abcu);
}

/* ------------------------------------------------------------------
 * HZLASR-29: FIOM=1, VS-mode load-acquire/store-release (always aq/rl).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_29_fiom1);
bool test_hzlasr_29_fiom1(void)
{
    TEST_BEGIN("HZLASR-29: FIOM=1 VS-mode load-acquire/store-release executable");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzlasr_fiom_case(1);
    printf("  [INFO] FIOM ordering effect requires multi-hart: %s\n",
           HZLASR_SMP_SKIP_REASON);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-30: FIOM=0 control.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_30_fiom0);
bool test_hzlasr_30_fiom0(void)
{
    TEST_BEGIN("HZLASR-30: FIOM=0 control VS-mode load-acquire/store-release");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzlasr_fiom_case(0);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-31: ADUE=0, load-acquire to an A=0 VS-stage page (R=1) -> Svade
 *            load page-fault (13, record-type); a pure read never touches
 *            the D bit and must NOT be mis-reported as a guest-page fault.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_31_adue0_load_acq_a0);
bool test_hzlasr_31_adue0_load_acq_a0(void)
{
    TEST_BEGIN("HZLASR-31: ADUE=0 + A=0 load-acquire -> Svade load pf (record)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
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
    *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X | PTE_D;   /* A=0, D=1 */
    hfence_vvma_all();
    hzlasr_store_le32(va, 0x00005678u);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("ADUE=0 A=0 load-acquire faulted (Svade)", fired);
    hzlasr_record_load_cause("load-acquire A-update (expect load pf 13)", cause);
    TEST_ASSERT("cause is NOT a guest-page fault (21/23)",
                cause != CAUSE_LOAD_GUEST_PAGE_FAULT &&
                cause != CAUSE_STORE_GUEST_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-32: ADUE=0, store-release to an A=1/D=0 page -> store page-fault
 *            (15). A store-release ALWAYS writes, so the D-bit requirement
 *            is MANDATORY (analogue of HZAMO-28, not UNSPECIFIED).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_32_adue0_store_rel_d0_forced);
bool test_hzlasr_32_adue0_store_rel_d0_forced(void)
{
    TEST_BEGIN("HZLASR-32: ADUE=0 + A=1/D=0 store-release -> store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
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
    hzlasr_store_le32(va, 0x00005678u);
    hz_st_val = 0x00009abcu;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("ADUE=0 D=0 store-release faulted (always writes)", fired);
    TEST_ASSERT_EQ("cause == store/AMO page-fault (15, D mandatory)",
                   cause, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-33: ADUE=1 (Svadu): load-acquire sets A only (NOT D),
 *            store-release sets A+D; both complete with no fault.
 *            SKIP if the platform does not implement Svadu.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_33_adue1_hw_update);
bool test_hzlasr_33_adue1_hw_update(void)
{
    TEST_BEGIN("HZLASR-33: ADUE=1 load-acquire A-only, store-release A+D");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;

    /* (a) load-acquire: hardware sets A, NOT D. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("(a) VS leaf PTE resolvable", pte != NULL);
    if (pte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    uintptr_t ppn = (*pte) & ~((1UL << 10) - 1UL);
    *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X;   /* A=0, D=0 */
    hfence_vvma_all();
    hzlasr_store_le32(va, 0x00005678u);

    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    if (!adue_on) {
        ts2_disable_adue();
        ts2_finish(&ctx);
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented");
    }

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, va);
    bool fired_l = trap_was_triggered();
    uintptr_t cause_l = fired_l ? trap_get_cause() : 0;
    trap_expect_end();
    uintptr_t pte_l = *pte;
    ts2_disable_adue();
    ts2_finish(&ctx);

    if (fired_l)
        printf("  UNEXPECTED TRAP (load-acquire): cause=%lu\n",
               (unsigned long)cause_l);
    TEST_ASSERT("(a) ADUE=1 load-acquire completed with no page fault", !fired_l);
    TEST_ASSERT("(a) hardware set PTE.A", (pte_l & PTE_A) != 0);
    TEST_ASSERT("(a) hardware did NOT set PTE.D (load-acquire is a pure read)",
                (pte_l & PTE_D) == 0);

    /* (b) store-release: hardware sets A+D. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("(b) VS leaf PTE resolvable", pte != NULL);
    if (pte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    ppn = (*pte) & ~((1UL << 10) - 1UL);
    *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X;   /* A=0, D=0 */
    hfence_vvma_all();
    hzlasr_store_le32(va, 0x00005678u);
    hz_st_val = 0x00009abcu;
    ts2_enable_adue();

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va);
    bool fired_s = trap_was_triggered();
    uintptr_t cause_s = fired_s ? trap_get_cause() : 0;
    trap_expect_end();
    uintptr_t pte_s = *pte;
    ts2_disable_adue();
    ts2_finish(&ctx);

    if (fired_s)
        printf("  UNEXPECTED TRAP (store-release): cause=%lu\n",
               (unsigned long)cause_s);
    TEST_ASSERT("(b) ADUE=1 store-release completed with no page fault", !fired_s);
    TEST_ASSERT("(b) hardware set PTE.A", (pte_s & PTE_A) != 0);
    TEST_ASSERT("(b) hardware set PTE.D (store-release writes)",
                (pte_s & PTE_D) != 0);

    HYP_TEST_END();
}
