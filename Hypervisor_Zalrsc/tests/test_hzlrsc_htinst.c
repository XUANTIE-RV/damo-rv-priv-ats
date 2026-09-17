/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.4 - htinst: transformed atomic instruction vs pseudoinstruction
 *             (HZLRSC-15 ~ HZLRSC-21)
 *
 * Spec basis:
 *   norm:H_trap_xtinst_val / htinst_transformed_atomic - for an explicit
 *     LR/SC access fault, htinst is 0 or the transformed atomic
 *     instruction (all fields preserved except bits 19:15 = Addr.Offset).
 *   norm:H_trap_xtinst_guestpage - for an implicit VS-stage page-table
 *     walk fault with nonzero htval, htinst MUST be a pseudoinstruction
 *     (zero is NOT allowed).
 *   norm:H_trap_xtinst_guestpage_rw - an A/D-update implicit store uses
 *     the WRITE pseudoinstruction (RV64 = 0x00003020).
 *   norm:lr_sc_alignment - LR/SC never split a misaligned access, so the
 *     Addr. Offset field (bits 19:15) is always 0.
 * =================================================================== */

/* GPA of the PTE that maps @vaddr at @level inside the page-table page
 * based at @pt_page_base (htval reports this >> 2 on implicit-walk
 * faults). A function (not a macro) so it composes cleanly as an actual
 * argument inside the CHECK_IMPLICIT_FAULT_REPORT / TEST_ASSERT_EQ macros. */
static inline uintptr_t hz_pte_gpa_in_pt(uintptr_t pt_page_base,
                                         uintptr_t vaddr, int level)
{
    return pt_page_base +
           (((vaddr >> (12 + level * 9)) & 0x1FFUL) * 8UL);
}

/* ------------------------------------------------------------------
 * HZLRSC-15: guest LR explicit G-stage fault -> htinst = 0 or the
 *            transformed atomic instruction.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_15_lr_htinst_transformed);
bool test_hzlrsc_15_lr_htinst_transformed(void)
{
    TEST_BEGIN("HZLRSC-15: LR explicit G-stage fault htinst = 0/transformed");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();

    TEST_ASSERT("LR guest fault fired", fired);
    TEST_ASSERT_EQ("cause == load guest-page-fault (21)",
                   cause, (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    /* golden check: htinst == 0 or transformed atomic (funct5=00010). */
    check_xtinst_zero_or_golden("LR htinst transformed-atomic", 0x2FUL, va);
    if (htinst != 0)
        TEST_ASSERT_EQ("nonzero htinst funct5 == LR (00010)",
                       (htinst >> 27) & 0x1FUL, (uintptr_t)0x02UL);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-16: guest SC explicit G-stage fault -> htinst = 0 or the
 *            transformed atomic instruction retaining funct5=00011.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_16_sc_htinst_transformed);
bool test_hzlrsc_16_sc_htinst_transformed(void)
{
    TEST_BEGIN("HZLRSC-16: SC explicit G-stage fault htinst keeps funct5=SC");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lrsc_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();

    TEST_ASSERT("SC guest fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    check_xtinst_zero_or_golden("SC htinst transformed-atomic", 0x2FUL, va);
    if (htinst != 0)
        TEST_ASSERT_EQ("nonzero htinst funct5 == SC (00011)",
                       (htinst >> 27) & 0x1FUL, (uintptr_t)0x03UL);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-17: htinst preserves the aq (bit26) / rl (bit25) annotations.
 * ------------------------------------------------------------------ */
static void hzlrsc_aqrl_case(uintptr_t (*probe)(uintptr_t), uintptr_t va,
                             uintptr_t g_flags, uintptr_t exp_cause,
                             unsigned exp_aq, unsigned exp_rl)
{
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, g_flags);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();
    /* golden comparison uses the trapping instruction word; only valid
     * when a fault actually fired (epc is stale otherwise). */
    if (fired)
        check_xtinst_zero_or_golden("aq/rl htinst transformed-atomic",
                                    0x2FUL, va);
    ts2_finish(&ctx);

    TEST_ASSERT("aq/rl variant faulted", fired);
    TEST_ASSERT_EQ("aq/rl variant cause", cause, exp_cause);
    if (htinst != 0) {
        TEST_ASSERT_EQ("htinst aq bit (26)",
                       (htinst >> 26) & 0x1UL, (uintptr_t)exp_aq);
        TEST_ASSERT_EQ("htinst rl bit (25)",
                       (htinst >> 25) & 0x1UL, (uintptr_t)exp_rl);
    } else {
        printf("  [INFO] htinst=0 (allowed for explicit access); "
               "aq/rl retention not observable this run\n");
    }
}

TEST_REGISTER(test_hzlrsc_17_htinst_aqrl);
bool test_hzlrsc_17_htinst_aqrl(void)
{
    TEST_BEGIN("HZLRSC-17: htinst preserves aq/rl bits");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    /* lr.w.aq  -> load guest-page-fault (21), aq=1 rl=0 */
    hzlrsc_aqrl_case(hz_vs_lr_w_aq, va, HZ_G_INV,
                     CAUSE_LOAD_GUEST_PAGE_FAULT, 1, 0);
    /* lr.w.aqrl -> (21), aq=1 rl=1 */
    hzlrsc_aqrl_case(hz_vs_lr_w_aqrl, va, HZ_G_INV,
                     CAUSE_LOAD_GUEST_PAGE_FAULT, 1, 1);
    /* sc.w.rl  -> store guest-page-fault (23), aq=0 rl=1 */
    hzlrsc_aqrl_case(hz_vs_lr_sc_w_rl, va, HZ_G_RU,
                     CAUSE_STORE_GUEST_PAGE_FAULT, 0, 1);
    /* sc.w.aqrl -> (23), aq=1 rl=1 */
    hzlrsc_aqrl_case(hz_vs_lr_sc_w_aqrl, va, HZ_G_RU,
                     CAUSE_STORE_GUEST_PAGE_FAULT, 1, 1);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-18: LR/SC htinst Addr. Offset (bits 19:15) is always 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_18_addr_offset_zero);
bool test_hzlrsc_18_addr_offset_zero(void)
{
    TEST_BEGIN("HZLRSC-18: LR/SC htinst Addr. Offset == 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired = trap_was_triggered();
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("LR guest fault fired", fired);
    if (htinst != 0)
        TEST_ASSERT_EQ("htinst Addr. Offset (bits 19:15) == 0",
                       (htinst >> 15) & 0x1FUL, (uintptr_t)0);
    else
        printf("  [INFO] htinst=0; Addr. Offset not observable this run\n");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-19: implicit VS-stage walk fault -> htinst MUST be the read
 *            pseudoinstruction (0x00003000) when htval != 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_19_implicit_walk_pseudo);
bool test_hzlrsc_19_implicit_walk_pseudo(void)
{
    TEST_BEGIN("HZLRSC-19: implicit VS-walk LR fault -> read pseudoinst");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;

    /* Invalidate the VS-stage leaf PT page in G-stage so the implicit
     * PTE read for @va faults. */
    uintptr_t pt_gpa = ts2_invalidate_vs_pt_in_g(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PT GPA resolvable", pt_gpa != 0);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    TEST_ASSERT("implicit-walk LR fault fired", fired);
    TEST_ASSERT_EQ("cause == load guest-page-fault (21)",
                   cause, (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    /* htval nonzero => htinst MUST be the read pseudoinstruction. */
    uintptr_t exp_htval19 = hz_pte_gpa_in_pt(pt_gpa, va, 0) >> 2;
    CHECK_IMPLICIT_FAULT_REPORT(exp_htval19, HTINST_PSEUDO_READ_RV64);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-20: A/D auto-update (ADUE=1) implicit store fault -> WRITE
 *            pseudoinstruction (0x00003020), cause = 23.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_20_adue_write_pseudo);
bool test_hzlrsc_20_adue_write_pseudo(void)
{
    TEST_BEGIN("HZLRSC-20: ADUE=1 SC D-update fault -> write pseudoinst");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    /* Patch the VS leaf PTE: W=1 (store passes VS perm), A=1, D=0 (so a
     * successful SC store requires a hardware D-bit update). */
    uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PTE resolvable", pte != NULL);
    if (pte == NULL) { ts2_finish(&ctx); HYP_TEST_END(); }
    uintptr_t ppn_bits = (*pte) & ~((1UL << 10) - 1UL);
    *pte = ppn_bits | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A;  /* no D */

    /* Make the VS-stage leaf PT page read-only in G-stage so the
     * implicit D-update store faults there. */
    uintptr_t pt_gpa = two_stage_vs_pt_page_addr(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PT GPA resolvable", pt_gpa != 0);
    ts2_g_override_4k(&ctx, pt_gpa, PTE_V | PTE_R | PTE_U | PTE_A); /* no W */

    /* Enable hardware A/D update and probe whether Svadu is present. */
    ts2_enable_adue();
    bool adue_on = ((menvcfg_read() & MENVCFG_ADUE) != 0) &&
                   ((henvcfg_read() & HENVCFG_ADUE) != 0);
    if (!adue_on) {
        ts2_disable_adue();
        ts2_finish(&ctx);
        TEST_SKIP("Svadu (henvcfg.ADUE) not implemented; write "
                  "pseudoinstruction path unreachable");
    }

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lrsc_w, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    ts2_finish(&ctx);
    ts2_disable_adue();

    TEST_ASSERT("A/D-update SC fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    uintptr_t exp_htval20 = hz_pte_gpa_in_pt(pt_gpa, va, 0) >> 2;
    CHECK_IMPLICIT_FAULT_REPORT(exp_htval20, HTINST_PSEUDO_WRITE_RV64);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-21: explicit vs implicit fault disambiguation (same cause=21).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_21_explicit_vs_implicit);
bool test_hzlrsc_21_explicit_vs_implicit(void)
{
    TEST_BEGIN("HZLRSC-21: htinst disambiguates explicit vs implicit (21)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    /* (a) explicit LR data access fails in G-stage. */
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_INV);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    uintptr_t htinst_a = fired_a ? trap_get_htinst() : 0;
    uintptr_t htval_a = fired_a ? trap_get_htval() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("(a) explicit LR fault fired", fired_a);
    TEST_ASSERT_EQ("(a) cause == 21", cause_a,
                   (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    /* explicit: htinst 0 or transformed atomic; htval = data GPA>>2. */
    TEST_ASSERT("(a) htinst == 0 or transformed (funct5=LR)",
                htinst_a == 0 || ((htinst_a >> 27) & 0x1FUL) == 0x02UL);
    TEST_ASSERT("(a) htval == 0 or data GPA>>2",
                htval_a == 0 || htval_a == (va >> 2));

    /* (b) implicit VS-stage PTE read fails in G-stage. */
    uintptr_t va_b = (uintptr_t)test_data_area;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t pt_gpa = ts2_invalidate_vs_pt_in_g(&ctx, va_b, PT_LEVEL_4K);
    TEST_ASSERT("(b) VS leaf PT GPA resolvable", pt_gpa != 0);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, va_b);
    bool fired_b = trap_was_triggered();
    uintptr_t cause_b = fired_b ? trap_get_cause() : 0;
    uintptr_t htinst_b = fired_b ? trap_get_htinst() : 0;
    uintptr_t htval_b = fired_b ? trap_get_htval() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("(b) implicit-walk LR fault fired", fired_b);
    TEST_ASSERT_EQ("(b) cause == 21 (same as explicit)", cause_b,
                   (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    /* implicit + htval nonzero => htinst MUST be the read pseudoinst. */
    if (htval_b != 0) {
        uintptr_t exp_htval21 = hz_pte_gpa_in_pt(pt_gpa, va_b, 0) >> 2;
        TEST_ASSERT_EQ("(b) htval == PTE GPA>>2", htval_b, exp_htval21);
        TEST_ASSERT_EQ("(b) htinst == read pseudoinst (zero NOT allowed)",
                       htinst_b, (uintptr_t)HTINST_PSEUDO_READ_RV64);
    }
    printf("  [INFO] (a) htinst=0x%lx htval=0x%lx | (b) htinst=0x%lx htval=0x%lx\n",
           (unsigned long)htinst_a, (unsigned long)htval_a,
           (unsigned long)htinst_b, (unsigned long)htval_b);

    HYP_TEST_END();
}
