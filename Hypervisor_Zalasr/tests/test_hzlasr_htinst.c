/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.4 - htinst: transformed atomic instruction (unified opcode 0x2F
 *             format) vs pseudoinstruction (HZLASR-18 ~ HZLASR-24)
 *
 * Spec basis:
 *   norm:H_trap_xtinst_val / htinst_transformed_atomic - load-acquire and
 *     store-release live in the AMO opcode (0x2F) and are NOT in the
 *     transformedloadinst (opcode 0x03) / transformedstoreinst (0x23)
 *     mnemonic lists, so for an explicit access fault htinst is 0 or the
 *     transformed ATOMIC instruction: all fields preserved except bits19:15
 *     (= Addr.Offset). The transformed value MUST retain funct5=00110
 *     (load-acquire) / 00111 (store-release), aq/rl, funct3, rd/rs2, so
 *     HS-mode can tell a load-acquire from a store-release. HZLASR-18~21
 *     verify empirically that hyp_transform_mem_inst()'s opcode 0x2F branch
 *     computes the golden value correctly for funct5=00110/00111.
 *   norm:H_vm_gpapriv - an implicit VS-stage walk fault is reported for the
 *     original access type: load-acquire -> 21 (record), store-release -> 23
 *     (forced).
 *   norm:H_trap_xtinst_guestpage / _rw - an implicit-walk fault with nonzero
 *     htval requires a pseudoinstruction (read 0x00003000 / A-D write
 *     0x00003020); zero is NOT allowed.
 *   Zalasr atomicity => a single memory operation => Addr. Offset == 0.
 * =================================================================== */

static inline uintptr_t hzlasr_pte_gpa_in_pt(uintptr_t pt_page_base,
                                             uintptr_t vaddr, int level)
{
    return pt_page_base + (((vaddr >> (12 + level * 9)) & 0x1FFUL) * 8UL);
}

/* ------------------------------------------------------------------
 * HZLASR-18: guest load-acquire explicit G-stage fault -> htinst = 0 or
 *            transformed atomic retaining funct5=00110.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_18_load_acq_htinst_transformed);
bool test_hzlasr_18_load_acq_htinst_transformed(void)
{
    TEST_BEGIN("HZLASR-18: load-acquire explicit G-stage fault htinst");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, va);
    bool fired = trap_was_triggered();
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();

    TEST_ASSERT("load-acquire guest fault fired", fired);
    if (fired)
        check_xtinst_zero_or_golden("load-acquire htinst transformed-atomic",
                                    0x2FUL, va);
    if (htinst != 0) {
        TEST_ASSERT_EQ("nonzero htinst funct5 == load-acquire (00110)",
                       (htinst >> 27) & 0x1FUL, (uintptr_t)ZALASR_F5_LOAD);
        TEST_ASSERT_EQ("htinst aq bit (26) == 1 (load-acquire always aq)",
                       (htinst >> 26) & 0x1UL, (uintptr_t)1);
    } else {
        printf("  [INFO] load-acquire htinst=0 (allowed); funct5 not observable\n");
    }

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-19: guest store-release explicit G-stage fault -> htinst = 0 or
 *            transformed atomic retaining funct5=00111, rl=1.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_19_store_rel_htinst_transformed);
bool test_hzlasr_19_store_rel_htinst_transformed(void)
{
    TEST_BEGIN("HZLASR-19: store-release explicit G-stage fault htinst");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hz_st_val = 0x00009abcu;

    trap_expect_begin();
#if __riscv_xlen == 64
    (void)two_stage_run_in_vs(&ctx, hz_vs_sd_rl, va);
#else
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va);
#endif
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();

    TEST_ASSERT("store-release guest fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    if (fired)
        check_xtinst_zero_or_golden("store-release htinst transformed-atomic",
                                    0x2FUL, va);
    if (htinst != 0) {
        TEST_ASSERT_EQ("nonzero htinst funct5 == store-release (00111)",
                       (htinst >> 27) & 0x1FUL, (uintptr_t)ZALASR_F5_STORE);
        TEST_ASSERT_EQ("htinst rl bit (25) == 1 (store-release always rl)",
                       (htinst >> 25) & 0x1UL, (uintptr_t)1);
    } else {
        printf("  [INFO] store-release htinst=0 (allowed); funct5 not observable\n");
    }

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-20: htinst retains the funct5 load/store distinction and the
 *            aq (bit26) / rl (bit25) annotations across four variants.
 * ------------------------------------------------------------------ */
static void hzlasr_aqrl_case(uintptr_t (*probe)(uintptr_t), uintptr_t va,
                             unsigned exp_f5, unsigned exp_aq, unsigned exp_rl)
{
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);
    hz_st_val = 0x00009abcu;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, probe, va);
    bool fired = trap_was_triggered();
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();
    if (fired)
        check_xtinst_zero_or_golden("Zalasr aq/rl htinst", 0x2FUL, va);
    ts2_finish(&ctx);

    TEST_ASSERT("Zalasr aq/rl variant faulted", fired);
    if (htinst != 0) {
        TEST_ASSERT_EQ("htinst funct5 (bits31:27)",
                       (htinst >> 27) & 0x1FUL, (uintptr_t)exp_f5);
        TEST_ASSERT_EQ("htinst aq bit (26)",
                       (htinst >> 26) & 0x1UL, (uintptr_t)exp_aq);
        TEST_ASSERT_EQ("htinst rl bit (25)",
                       (htinst >> 25) & 0x1UL, (uintptr_t)exp_rl);
    } else {
        printf("  [INFO] htinst=0 (allowed); funct5/aq/rl retention not "
               "observable this run\n");
    }
}

TEST_REGISTER(test_hzlasr_20_htinst_funct5_aqrl);
bool test_hzlasr_20_htinst_funct5_aqrl(void)
{
    TEST_BEGIN("HZLASR-20: htinst retains funct5 (load/store) + aq/rl bits");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    hzlasr_aqrl_case(hz_vs_lw_aq,   va, ZALASR_F5_LOAD,  1, 0);
    hzlasr_aqrl_case(hz_vs_lw_aqrl, va, ZALASR_F5_LOAD,  1, 1);
    hzlasr_aqrl_case(hz_vs_sw_rl,   va, ZALASR_F5_STORE, 0, 1);
    hzlasr_aqrl_case(hz_vs_sw_aqrl, va, ZALASR_F5_STORE, 1, 1);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-21: load-acquire/store-release htinst Addr. Offset (bits19:15)
 *            is always 0 (single memory operation, never page-split).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_21_addr_offset_zero);
bool test_hzlasr_21_addr_offset_zero(void)
{
    TEST_BEGIN("HZLASR-21: Zalasr htinst Addr. Offset == 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    /* store-release explicit G-stage fault. */
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hz_st_val = 0x00009abcu;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va);
    bool fired_s = trap_was_triggered();
    uintptr_t htinst_s = fired_s ? trap_get_htinst() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("store-release guest fault fired", fired_s);
    if (htinst_s != 0)
        TEST_ASSERT_EQ("store-release htinst Addr. Offset (bits19:15) == 0",
                       (htinst_s >> 15) & 0x1FUL, (uintptr_t)0);
    else
        printf("  [INFO] store-release htinst=0; Addr. Offset not observable\n");

    /* load-acquire explicit G-stage fault. */
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, va);
    bool fired_l = trap_was_triggered();
    uintptr_t htinst_l = fired_l ? trap_get_htinst() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("load-acquire guest fault fired", fired_l);
    if (htinst_l != 0)
        TEST_ASSERT_EQ("load-acquire htinst Addr. Offset (bits19:15) == 0",
                       (htinst_l >> 15) & 0x1FUL, (uintptr_t)0);
    else
        printf("  [INFO] load-acquire htinst=0; Addr. Offset not observable\n");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-22: implicit VS-stage walk fault reported for the original type:
 *            load-acquire -> 21 (record), store-release -> 23 (forced),
 *            + read pseudoinstruction when htval != 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_22_implicit_walk_cause);
bool test_hzlasr_22_implicit_walk_cause(void)
{
    TEST_BEGIN("HZLASR-22: implicit VS-walk fault -> load 21 (rec) / store 23");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;

    /* (a) load-acquire implicit walk -> expect cause 21 (record-type). */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t pt_gpa_a = ts2_invalidate_vs_pt_in_g(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("(a) VS leaf PT GPA resolvable", pt_gpa_a != 0);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, va);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    trap_expect_end();
    uintptr_t exp_a = hzlasr_pte_gpa_in_pt(pt_gpa_a, va, 0) >> 2;
    if (fired_a)
        CHECK_IMPLICIT_FAULT_REPORT(exp_a, HTINST_PSEUDO_READ_RV64);
    ts2_finish(&ctx);

    TEST_ASSERT("(a) implicit-walk load-acquire fault fired", fired_a);
    hzlasr_record_load_cause("(a) implicit-walk load-acquire (expect gpf 21)",
                             cause_a);

    /* (b) store-release implicit walk -> cause 23 (forced). */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hz_st_val = 0x00009abcu;
    uintptr_t pt_gpa_b = ts2_invalidate_vs_pt_in_g(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("(b) VS leaf PT GPA resolvable", pt_gpa_b != 0);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va);
    bool fired_b = trap_was_triggered();
    uintptr_t cause_b = fired_b ? trap_get_cause() : 0;
    trap_expect_end();
    uintptr_t exp_b = hzlasr_pte_gpa_in_pt(pt_gpa_b, va, 0) >> 2;
    if (fired_b)
        CHECK_IMPLICIT_FAULT_REPORT(exp_b, HTINST_PSEUDO_READ_RV64);
    ts2_finish(&ctx);

    TEST_ASSERT("(b) implicit-walk store-release fault fired", fired_b);
    TEST_ASSERT_EQ("(b) cause == store/AMO guest-page-fault (23), NOT 21",
                   cause_b, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-23: ADUE=1, VS PTE A/D=0 + PT page G-stage read-only -> implicit
 *            A/D-update store faults: write pseudoinst.
 *            (a) load-acquire updates only A -> cause 21 (record).
 *            (b) store-release updates A+D -> cause 23 (forced).
 * ------------------------------------------------------------------ */
static void hzlasr_adue_write_pseudo(int is_store, uintptr_t va)
{
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hz_st_val = 0x00009abcu;

    uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PTE resolvable", pte != NULL);
    if (pte == NULL) { ts2_finish(&ctx); return; }
    uintptr_t ppn = (*pte) & ~((1UL << 10) - 1UL);
    if (is_store)
        *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A;   /* D=0 */
    else
        *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X | PTE_D;   /* A=0 */

    uintptr_t pt_gpa = two_stage_vs_pt_page_addr(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PT GPA resolvable", pt_gpa != 0);
    ts2_g_override_4k(&ctx, pt_gpa, PTE_V | PTE_R | PTE_U | PTE_A);  /* no W */

    ts2_enable_adue();   /* Svadu availability is checked once by the caller */

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, is_store ? hz_vs_sw_rl : hz_vs_lw_aq, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = fired ? trap_get_htval() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();

    uintptr_t exp_htval = hzlasr_pte_gpa_in_pt(pt_gpa, va, 0) >> 2;
    ts2_finish(&ctx);
    ts2_disable_adue();

    printf("  [INFO] ADUE %s A/D-update: fired=%d cause=%lu htval=0x%lx "
           "htinst=0x%lx\n", is_store ? "store-release" : "load-acquire",
           (int)fired, (unsigned long)cause, (unsigned long)htval,
           (unsigned long)htinst);
    TEST_ASSERT("A/D-update Zalasr fault fired", fired);
    if (is_store)
        TEST_ASSERT_EQ("store-release cause == store/AMO guest-pf (23)",
                       cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    else
        hzlasr_record_load_cause("load-acquire A-update (expect gpf 21)", cause);
    if (htval != 0) {
        TEST_ASSERT_EQ("htval == implicit-access PTE GPA>>2", htval, exp_htval);
        TEST_ASSERT_EQ("htinst == write pseudoinst (zero NOT allowed)",
                       htinst, (uintptr_t)HTINST_PSEUDO_WRITE_RV64);
    }
}

TEST_REGISTER(test_hzlasr_23_adue_write_pseudo);
bool test_hzlasr_23_adue_write_pseudo(void)
{
    TEST_BEGIN("HZLASR-23: ADUE=1 A/D-update fault -> write pseudoinst");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;

    /* Probe Svadu availability once (TEST_SKIP must run in the bool-returning
     * test body, not in the void helper). */
    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    ts2_disable_adue();
    if (!adue_on)
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented");

    hzlasr_adue_write_pseudo(0, va);   /* (a) load-acquire: A only  */
    hzlasr_adue_write_pseudo(1, va);   /* (b) store-release: A+D    */

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-24: explicit vs implicit fault disambiguation (store-release,
 *            same cause=23): htinst is the sole disambiguator.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_24_explicit_vs_implicit);
bool test_hzlasr_24_explicit_vs_implicit(void)
{
    TEST_BEGIN("HZLASR-24: htinst disambiguates explicit vs implicit (23)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    /* (a) explicit store-release data access fails in G-stage. */
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hz_st_val = 0x00009abcu;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    uintptr_t htinst_a = fired_a ? trap_get_htinst() : 0;
    uintptr_t htval_a = fired_a ? trap_get_htval() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("(a) explicit store-release fault fired", fired_a);
    TEST_ASSERT_EQ("(a) cause == 23", cause_a,
                   (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    TEST_ASSERT("(a) htinst == 0 or transformed (funct5=store-release)",
                htinst_a == 0 || ((htinst_a >> 27) & 0x1FUL) == ZALASR_F5_STORE);
    TEST_ASSERT("(a) htval == 0 or data GPA>>2",
                htval_a == 0 || htval_a == (va >> 2));

    /* (b) implicit VS-stage PTE read fails in G-stage. */
    uintptr_t va_b = (uintptr_t)test_data_area;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hz_st_val = 0x00009abcu;
    uintptr_t pt_gpa = ts2_invalidate_vs_pt_in_g(&ctx, va_b, PT_LEVEL_4K);
    TEST_ASSERT("(b) VS leaf PT GPA resolvable", pt_gpa != 0);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, va_b);
    bool fired_b = trap_was_triggered();
    uintptr_t cause_b = fired_b ? trap_get_cause() : 0;
    uintptr_t htinst_b = fired_b ? trap_get_htinst() : 0;
    uintptr_t htval_b = fired_b ? trap_get_htval() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("(b) implicit-walk store-release fault fired", fired_b);
    TEST_ASSERT_EQ("(b) cause == 23 (store-release original type)", cause_b,
                   (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    if (htval_b != 0) {
        uintptr_t exp = hzlasr_pte_gpa_in_pt(pt_gpa, va_b, 0) >> 2;
        TEST_ASSERT_EQ("(b) htval == PTE GPA>>2", htval_b, exp);
        TEST_ASSERT_EQ("(b) htinst == read pseudoinst (zero NOT allowed)",
                       htinst_b, (uintptr_t)HTINST_PSEUDO_READ_RV64);
    }
    printf("  [INFO] (a) htinst=0x%lx htval=0x%lx | (b) htinst=0x%lx htval=0x%lx\n",
           (unsigned long)htinst_a, (unsigned long)htval_a,
           (unsigned long)htinst_b, (unsigned long)htval_b);

    HYP_TEST_END();
}
