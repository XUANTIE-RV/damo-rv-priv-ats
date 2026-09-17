/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.5 - misaligned load-acquire/store-release and MAG relaxation
 *             (HZLASR-25 ~ HZLASR-28)
 *
 * Spec basis:
 *   norm:zalasr_natural_align / norm:zalasr_misaligned_exception - a
 *     Zalasr access requires natural alignment; if misaligned and not
 *     MAG-covered it raises an address-misaligned OR access-fault
 *     exception (both compliant). store-release -> store/AMO class (6/7,
 *     FORCED); load-acquire -> load class (4/5, RECORD-AND-COMPARE because
 *     zalasr.adoc does not pin its class).
 *   norm:zalasr_misaligned_pma_relax / norm:zalasr_misaligned_single_op -
 *     when every accessed byte lies in one misaligned atomicity granule
 *     (Zama16b pins it at 16 bytes) there is NO alignment exception and
 *     the access is a single atomic memory operation.
 *
 * NOTE (known platform defect): QEMU/Spike reject an INTRA-granule
 *   misaligned load-acquire/store-release with cause 4/6, violating
 *   norm:zalasr_misaligned_single_op + Zama16b. HZLASR-27 asserts the
 *   SPEC behaviour (no fault) and is expected to FAIL there; the defect is
 *   archived in
 *   bugs/qemu_spike_zalasr_misaligned_intra_granule_rejected_bug.md.
 * =================================================================== */

static inline bool hzlasr_cause_in2(uintptr_t c, uintptr_t a, uintptr_t b)
{
    return c == a || c == b;
}

/* Preset the granule holding test_data_area so a misaligned sub-word
 * access has valid backing bytes. */
static inline void hzlasr_preset_granule(void)
{
    hzlasr_store_le64((uintptr_t)test_data_area & ~15UL,
                      0x0011223344556677ULL);
    hzlasr_store_le64(((uintptr_t)test_data_area & ~15UL) + 8,
                      0x8899aabbccddeeffULL);
}

/* ------------------------------------------------------------------
 * HZLASR-25: misaligned load-acquire with NO MAG coverage -> fault; the
 *            cause is RECORD-AND-COMPARE (functional expectation 4/5);
 *            delegation per hedeleg[4]/[5].
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_25_misaligned_load_acq);
bool test_hzlasr_25_misaligned_load_acq(void)
{
    TEST_BEGIN("HZLASR-25: misaligned load-acquire (no MAG) -> fault (record)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t off = HZLASR_MAG_DECLARED ? HZLASR_W_MIS_STRADDLE
                                        : HZLASR_W_MIS_INTRA;
    uintptr_t mis = (uintptr_t)test_data_area + off;

    /* Part A: hedeleg[4]/[5]=0 -> captured at HS/M level. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzlasr_preset_granule();
    hedeleg_write(hedeleg_read() & ~((1UL << 4) | (1UL << 5)));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, mis);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    printf("  [INFO] (A) misaligned load-acquire @+off=%lu cause=%lu fired=%d\n",
           (unsigned long)off, (unsigned long)cause_a, (int)fired_a);
    TEST_ASSERT("(A) misaligned load-acquire faulted (not MAG-covered)", fired_a);
    hzlasr_record_load_cause("(A) misaligned load-acquire (expect 4/5)", cause_a);

    /* Part B: hedeleg[4]/[5]=1 -> a load-align cause is delivered to VS. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzlasr_preset_granule();
    hyp_delegate_to_vs((1UL << 4) | (1UL << 5), 0);
    hz_vs_handler_install();
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, mis);
    bool fired_mB = trap_was_triggered();
    uintptr_t cause_mB = fired_mB ? trap_get_cause() : 0;
    trap_expect_end();
    bool vs_b = g_hz_vs_triggered;
    uintptr_t cause_b = vs_b ? g_hz_vs_cause : cause_mB;
    ts2_finish(&ctx);

    printf("  [INFO] (B) misaligned load-acquire hedeleg[4/5]=1: vs=%d cause=%lu\n",
           (int)vs_b, (unsigned long)cause_b);
    if (hzlasr_load_align_cause(cause_a)) {
        TEST_ASSERT("(B) load-align cause (4/5) delegated to VS-mode", vs_b);
        TEST_ASSERT("(B) vscause in load-align class {4,5}",
                    hzlasr_load_align_cause(cause_b));
    } else {
        printf("  [RECORD] (B) Part-A cause=%lu not load-align; delegation of "
               "4/5 not exercised (SPEC-ambiguous class)\n",
               (unsigned long)cause_a);
    }

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-26: misaligned store-release with NO MAG coverage -> store/AMO
 *            address misaligned (6) or access fault (7), FORCED; deleg per
 *            hedeleg[6]/[7].
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_26_misaligned_store_rel);
bool test_hzlasr_26_misaligned_store_rel(void)
{
    TEST_BEGIN("HZLASR-26: misaligned store-release (no MAG) -> cause 6/7");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t off = HZLASR_MAG_DECLARED ? HZLASR_W_MIS_STRADDLE
                                        : HZLASR_W_MIS_INTRA;
    uintptr_t mis = (uintptr_t)test_data_area + off;
    hz_st_val = 0x00009abcu;

    /* Part A: hedeleg[6]/[7]=0 -> captured at HS/M level. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzlasr_preset_granule();
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, mis);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    printf("  [INFO] (A) misaligned store-release @+off=%lu cause=%lu fired=%d\n",
           (unsigned long)off, (unsigned long)cause_a, (int)fired_a);
    TEST_ASSERT("(A) misaligned store-release faulted (not MAG-covered)", fired_a);
    TEST_ASSERT("(A) cause in store/AMO class {6,7} (never load 4/5)",
                hzlasr_cause_in2(cause_a, CAUSE_STORE_ADDR_MISALIGN,
                                 CAUSE_STORE_ACCESS_FAULT));

    /* Part B: hedeleg[6]/[7]=1 -> delivered to VS-mode. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzlasr_preset_granule();
    hyp_delegate_to_vs((1UL << 6) | (1UL << 7), 0);
    hz_vs_handler_install();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, mis);
    bool vs_b = g_hz_vs_triggered;
    uintptr_t cause_b = g_hz_vs_cause;
    ts2_finish(&ctx);

    TEST_ASSERT("(B) misaligned store-release delivered to VS-mode", vs_b);
    TEST_ASSERT("(B) vscause in store/AMO class {6,7}",
                hzlasr_cause_in2(cause_b, CAUSE_STORE_ADDR_MISALIGN,
                                 CAUSE_STORE_ACCESS_FAULT));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-27: MAG intra-granule misaligned load-acquire/store-release ->
 *            NO alignment exception, executes atomically. SKIP if no MAG.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_27_mag_intra_no_fault);
bool test_hzlasr_27_mag_intra_no_fault(void)
{
    TEST_BEGIN("HZLASR-27: MAG intra-granule misaligned Zalasr -> no fault");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    if (!HZLASR_MAG_DECLARED)
        TEST_SKIP("platform declares no misaligned atomicity granule (Zama16b)");

    uintptr_t mis = (uintptr_t)test_data_area + HZLASR_W_MIS_INTRA;
    hz_st_val = 0x00009abcu;

    /* load-acquire intra-granule. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzlasr_preset_granule();
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, mis);
    bool fired_l = trap_was_triggered();
    uintptr_t cause_l = fired_l ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);
    if (fired_l)
        printf("  [INFO] MAG intra load-acquire trapped cause=%lu\n",
               (unsigned long)cause_l);
    TEST_ASSERT("MAG intra-granule misaligned load-acquire raised no alignment "
                "exception (norm:zalasr_misaligned_single_op)", !fired_l);

    /* store-release intra-granule. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzlasr_preset_granule();
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, mis);
    bool fired_s = trap_was_triggered();
    uintptr_t cause_s = fired_s ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);
    if (fired_s)
        printf("  [INFO] MAG intra store-release trapped cause=%lu\n",
               (unsigned long)cause_s);
    TEST_ASSERT("MAG intra-granule misaligned store-release raised no alignment "
                "exception (norm:zalasr_misaligned_single_op)", !fired_s);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-28: misaligned Zalasr trapped at HS-mode: stval == the misaligned
 *            address, GVA=1, SPV=1, htval=0, htinst Addr. Offset == 0.
 *            load-acquire cause recorded, store-release cause 6/7 forced.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_28_misaligned_trap_context);
bool test_hzlasr_28_misaligned_trap_context(void)
{
    TEST_BEGIN("HZLASR-28: misaligned Zalasr HS-mode trap context");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t off = HZLASR_MAG_DECLARED ? HZLASR_W_MIS_STRADDLE
                                        : HZLASR_W_MIS_INTRA;
    uintptr_t mis = (uintptr_t)test_data_area + off;
    hz_st_val = 0x00009abcu;

    /* store-release misaligned trap context (forced store class). */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzlasr_preset_granule();
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    hz_clear_gva_spv();
    hz_route_to_hs((1UL << 6) | (1UL << 7));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sw_rl, mis);
    bool fired_s = trap_was_triggered();
    uintptr_t cause_s = fired_s ? trap_get_cause() : 0;
    uintptr_t tval_s = fired_s ? trap_get_tval() : 0;
    uintptr_t htval_s = fired_s ? trap_get_htval() : 0;
    uintptr_t htinst_s = fired_s ? trap_get_htinst() : 0;
    bool gva_s = trap_get_gva();
    bool spv_s = trap_get_spv();
    trap_expect_end();
    hz_unroute_from_hs((1UL << 6) | (1UL << 7));
    ts2_finish(&ctx);

    printf("  [INFO] store-release misaligned: cause=%lu tval=0x%lx gva=%d spv=%d\n",
           (unsigned long)cause_s, (unsigned long)tval_s, (int)gva_s, (int)spv_s);
    TEST_ASSERT("store-release misaligned faulted", fired_s);
    TEST_ASSERT("store-release cause in store/AMO class {6,7}",
                hzlasr_cause_in2(cause_s, CAUSE_STORE_ADDR_MISALIGN,
                                 CAUSE_STORE_ACCESS_FAULT));
    TEST_ASSERT_EQ("store-release stval == misaligned address itself", tval_s, mis);
    TEST_ASSERT_EQ("store-release GVA=1", (uintptr_t)gva_s, (uintptr_t)1);
    TEST_ASSERT("store-release SPV=1", spv_s);
    TEST_ASSERT_EQ("store-release htval == 0 (not a guest-page fault)",
                   htval_s, (uintptr_t)0);
    if (htinst_s != 0)
        TEST_ASSERT_EQ("store-release htinst Addr. Offset == 0",
                       (htinst_s >> 15) & 0x1FUL, (uintptr_t)0);

    /* load-acquire misaligned trap context (record cause class). */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzlasr_preset_granule();
    hedeleg_write(hedeleg_read() & ~((1UL << 4) | (1UL << 5)));
    hz_clear_gva_spv();
    hz_route_to_hs((1UL << 4) | (1UL << 5));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, mis);
    bool fired_l = trap_was_triggered();
    uintptr_t cause_l = fired_l ? trap_get_cause() : 0;
    uintptr_t tval_l = fired_l ? trap_get_tval() : 0;
    uintptr_t htval_l = fired_l ? trap_get_htval() : 0;
    bool gva_l = trap_get_gva();
    bool spv_l = trap_get_spv();
    trap_expect_end();
    hz_unroute_from_hs((1UL << 4) | (1UL << 5));
    ts2_finish(&ctx);

    printf("  [INFO] load-acquire misaligned: cause=%lu tval=0x%lx gva=%d spv=%d\n",
           (unsigned long)cause_l, (unsigned long)tval_l, (int)gva_l, (int)spv_l);
    TEST_ASSERT("load-acquire misaligned faulted", fired_l);
    hzlasr_record_load_cause("load-acquire misaligned (expect 4/5)", cause_l);
    TEST_ASSERT_EQ("load-acquire stval == misaligned address itself", tval_l, mis);
    TEST_ASSERT_EQ("load-acquire GVA=1", (uintptr_t)gva_l, (uintptr_t)1);
    TEST_ASSERT("load-acquire SPV=1", spv_l);
    TEST_ASSERT_EQ("load-acquire htval == 0 (not a guest-page fault)",
                   htval_l, (uintptr_t)0);

    HYP_TEST_END();
}
