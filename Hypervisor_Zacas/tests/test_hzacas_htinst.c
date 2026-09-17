/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 3.4 - htinst: transformed atomic instruction vs pseudoinstruction
 *             (HZACAS-17 ~ HZACAS-22)
 *
 * Spec basis:
 *   norm:H_trap_xtinst_val / htinst_transformed_atomic - for an explicit
 *     amocas access fault, htinst is 0 or the transformed atomic
 *     instruction (all fields preserved except bits 19:15 = Addr.Offset).
 *     The transformed value must retain funct5=00101 (amocas).
 *   norm:H_vm_gpapriv - an implicit VS-stage page-table walk fault is
 *     reported for the amocas original access type -> cause 23.
 *   norm:H_trap_xtinst_guestpage / _rw - implicit-walk fault with nonzero
 *     htval requires a pseudoinstruction (read 0x00003000 / A-D write
 *     0x00003020); zero is NOT allowed.
 *   amocas atomicity => a single memory operation => Addr. Offset == 0.
 * =================================================================== */

static inline uintptr_t hzacas_pte_gpa_in_pt(uintptr_t pt_page_base,
                                             uintptr_t vaddr, int level)
{
    return pt_page_base + (((vaddr >> (12 + level * 9)) & 0x1FFUL) * 8UL);
}

/* Set up a success-path amocas against a G-stage victim and report. */
static void hzacas_setup_cas_gstage(uintptr_t va)
{
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;
}

/* ------------------------------------------------------------------
 * HZACAS-17: guest amocas explicit G-stage fault -> htinst = 0 or
 *            transformed atomic retaining funct5=00101.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_17_cas_htinst_transformed);
bool test_hzacas_17_cas_htinst_transformed(void)
{
    TEST_BEGIN("HZACAS-17: amocas explicit G-stage fault htinst transformed");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hzacas_setup_cas_gstage(va);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_d_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();

    TEST_ASSERT("amocas guest fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    if (fired)
        check_xtinst_zero_or_golden("amocas htinst transformed-atomic",
                                    0x2FUL, va);
    if (htinst != 0)
        TEST_ASSERT_EQ("nonzero htinst funct5 == amocas (00101)",
                       (htinst >> 27) & 0x1FUL, (uintptr_t)0x05UL);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-18: htinst preserves the aq (bit26) / rl (bit25) annotations.
 * ------------------------------------------------------------------ */
static void hzacas_aqrl_case(uintptr_t (*probe)(uintptr_t), uintptr_t va,
                             unsigned exp_aq, unsigned exp_rl)
{
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hzacas_setup_cas_gstage(va);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, probe, va);
    bool fired = trap_was_triggered();
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();
    if (fired)
        check_xtinst_zero_or_golden("aq/rl amocas htinst", 0x2FUL, va);
    ts2_finish(&ctx);

    TEST_ASSERT("aq/rl amocas variant faulted", fired);
    if (htinst != 0) {
        TEST_ASSERT_EQ("htinst aq bit (26)",
                       (htinst >> 26) & 0x1UL, (uintptr_t)exp_aq);
        TEST_ASSERT_EQ("htinst rl bit (25)",
                       (htinst >> 25) & 0x1UL, (uintptr_t)exp_rl);
    } else {
        printf("  [INFO] htinst=0 (allowed); aq/rl retention not observable\n");
    }
}

TEST_REGISTER(test_hzacas_18_htinst_aqrl);
bool test_hzacas_18_htinst_aqrl(void)
{
    TEST_BEGIN("HZACAS-18: htinst preserves amocas aq/rl bits");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    hzacas_aqrl_case(hz_vs_amocas_w_aq, va, 1, 0);
    hzacas_aqrl_case(hz_vs_amocas_w_rl, va, 0, 1);
    hzacas_aqrl_case(hz_vs_amocas_w_aqrl, va, 1, 1);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-19: amocas htinst Addr. Offset (bits 19:15) is always 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_19_addr_offset_zero);
bool test_hzacas_19_addr_offset_zero(void)
{
    TEST_BEGIN("HZACAS-19: amocas htinst Addr. Offset == 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hzacas_setup_cas_gstage(va);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("amocas guest fault fired", fired);
    if (htinst != 0)
        TEST_ASSERT_EQ("htinst Addr. Offset (bits 19:15) == 0",
                       (htinst >> 15) & 0x1FUL, (uintptr_t)0);
    else
        printf("  [INFO] htinst=0; Addr. Offset not observable this run\n");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-20: implicit VS-stage walk fault -> cause 23 (amocas original
 *            type) + read pseudoinstruction when htval != 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_20_implicit_walk_cause23);
bool test_hzacas_20_implicit_walk_cause23(void)
{
    TEST_BEGIN("HZACAS-20: implicit VS-walk amocas fault -> cause 23 + pseudo");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    hzacas_setup_cas_gstage(va);

    uintptr_t pt_gpa = ts2_invalidate_vs_pt_in_g(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PT GPA resolvable", pt_gpa != 0);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    TEST_ASSERT("implicit-walk amocas fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23), NOT 21",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    uintptr_t exp_htval = hzacas_pte_gpa_in_pt(pt_gpa, va, 0) >> 2;
    CHECK_IMPLICIT_FAULT_REPORT(exp_htval, HTINST_PSEUDO_READ_RV64);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-21: ADUE=1, VS PTE D=0 (success CAS writes) + PT page G-stage
 *            read-only -> implicit D-update store faults: write
 *            pseudoinst, cause 23.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_21_adue_write_pseudo);
bool test_hzacas_21_adue_write_pseudo(void)
{
    TEST_BEGIN("HZACAS-21: ADUE=1 amocas D-update fault -> write pseudoinst");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzacas_setup_cas_gstage(va);

    uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PTE resolvable", pte != NULL);
    if (pte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    uintptr_t ppn = (*pte) & ~((1UL << 10) - 1UL);
    *pte = ppn | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A;   /* D=0 */

    uintptr_t pt_gpa = two_stage_vs_pt_page_addr(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PT GPA resolvable", pt_gpa != 0);
    ts2_g_override_4k(&ctx, pt_gpa, PTE_V | PTE_R | PTE_U | PTE_A);  /* no W */

    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    if (!adue_on) {
        ts2_disable_adue();
        ts2_finish(&ctx);
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented");
    }

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_d_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = fired ? trap_get_htval() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();

    uintptr_t exp_htval = hzacas_pte_gpa_in_pt(pt_gpa, va, 0) >> 2;
    ts2_finish(&ctx);
    ts2_disable_adue();

    printf("  [INFO] ADUE amocas D-update: fired=%d cause=%lu htval=0x%lx "
           "htinst=0x%lx\n", (int)fired, (unsigned long)cause,
           (unsigned long)htval, (unsigned long)htinst);
    TEST_ASSERT("A/D-update amocas fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    if (htval != 0) {
        TEST_ASSERT_EQ("htval == implicit-access PTE GPA>>2", htval, exp_htval);
        TEST_ASSERT_EQ("htinst == write pseudoinst (zero NOT allowed)",
                       htinst, (uintptr_t)HTINST_PSEUDO_WRITE_RV64);
    }

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-22: explicit vs implicit fault disambiguation (same cause=23).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_22_explicit_vs_implicit);
bool test_hzacas_22_explicit_vs_implicit(void)
{
    TEST_BEGIN("HZACAS-22: htinst disambiguates explicit vs implicit (23)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    /* (a) explicit amocas data access fails in G-stage. */
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hzacas_setup_cas_gstage(va);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    uintptr_t htinst_a = fired_a ? trap_get_htinst() : 0;
    uintptr_t htval_a = fired_a ? trap_get_htval() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("(a) explicit amocas fault fired", fired_a);
    TEST_ASSERT_EQ("(a) cause == 23", cause_a,
                   (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    TEST_ASSERT("(a) htinst == 0 or transformed (funct5=amocas)",
                htinst_a == 0 || ((htinst_a >> 27) & 0x1FUL) == 0x05UL);
    TEST_ASSERT("(a) htval == 0 or data GPA>>2",
                htval_a == 0 || htval_a == (va >> 2));

    /* (b) implicit VS-stage PTE read fails in G-stage. */
    uintptr_t va_b = (uintptr_t)test_data_area;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzacas_setup_cas_gstage(va_b);
    uintptr_t pt_gpa = ts2_invalidate_vs_pt_in_g(&ctx, va_b, PT_LEVEL_4K);
    TEST_ASSERT("(b) VS leaf PT GPA resolvable", pt_gpa != 0);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va_b);
    bool fired_b = trap_was_triggered();
    uintptr_t cause_b = fired_b ? trap_get_cause() : 0;
    uintptr_t htinst_b = fired_b ? trap_get_htinst() : 0;
    uintptr_t htval_b = fired_b ? trap_get_htval() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("(b) implicit-walk amocas fault fired", fired_b);
    TEST_ASSERT_EQ("(b) cause == 23 (amocas original type)", cause_b,
                   (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    if (htval_b != 0) {
        uintptr_t exp = hzacas_pte_gpa_in_pt(pt_gpa, va_b, 0) >> 2;
        TEST_ASSERT_EQ("(b) htval == PTE GPA>>2", htval_b, exp);
        TEST_ASSERT_EQ("(b) htinst == read pseudoinst (zero NOT allowed)",
                       htinst_b, (uintptr_t)HTINST_PSEUDO_READ_RV64);
    }
    printf("  [INFO] (a) htinst=0x%lx htval=0x%lx | (b) htinst=0x%lx htval=0x%lx\n",
           (unsigned long)htinst_a, (unsigned long)htval_a,
           (unsigned long)htinst_b, (unsigned long)htval_b);

    HYP_TEST_END();
}
