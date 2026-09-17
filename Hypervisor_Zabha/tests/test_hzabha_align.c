/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4.5 - byte AMO never misaligned; half AMO misaligned/MAG
 *             (HZABHA-22 ~ HZABHA-26)
 *
 * Spec basis:
 *   norm:Zabha_rs1_align_addr - byte/half AMOs require natural alignment,
 *     cross-referencing Zaamo (norm:amo_alignment /
 *     norm:misaligned_atomicity_granule_size). BUT a byte AMO's 1-byte
 *     alignment ALWAYS holds, so a byte AMO is NEVER misaligned (no
 *     misaligned/MAG branch); only a halfword AMO (odd address) can be
 *     misaligned.
 *   Zama16b fixes the granule at 16 bytes. For a halfword (2 bytes):
 *     offset 1  -> bytes [1,3)  inside granule [0,16)  (relaxed)
 *     offset 15 -> bytes [15,17) straddle the 16-byte boundary (fault)
 * =================================================================== */

#ifdef ZAMA16B_SUPPORTED
#define HZABHA_MAG_DECLARED  1
#else
#define HZABHA_MAG_DECLARED  0
#endif

/* Halfword offsets: intra-granule (relaxed) vs granule-straddling (fault). */
#define HZABHA_H_MIS_INTRA     1UL
#define HZABHA_H_MIS_STRADDLE  15UL

static inline bool hzabha_cause_in2(uintptr_t c, uintptr_t a, uintptr_t b)
{
    return c == a || c == b;
}

/* ------------------------------------------------------------------
 * HZABHA-22: byte AMO at ANY address (even odd) is naturally aligned ->
 *            no alignment exception. FORCED POSITIVE (1-byte alignment
 *            always holds).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_22_byte_amo_never_misaligned);
bool test_hzabha_22_byte_amo_never_misaligned(void)
{
    TEST_BEGIN("HZABHA-22: byte AMO at odd address -> no alignment fault");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    uintptr_t odd = va + 1;   /* odd address: always valid for a byte AMO */

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_b, odd);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("byte AMO at odd address took no alignment exception "
                "(1-byte alignment always holds)", !fired);
    TEST_ASSERT("byte AMO did not report cause=6/7",
                !hzabha_cause_in2(cause, CAUSE_STORE_ADDR_MISALIGN,
                                  CAUSE_STORE_ACCESS_FAULT));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-23: misaligned half AMO with NO MAG coverage -> store/AMO address
 *            misaligned (6) or access fault (7); deleg per hedeleg[6]/[7].
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_23_misaligned_half_amo);
bool test_hzabha_23_misaligned_half_amo(void)
{
    TEST_BEGIN("HZABHA-23: misaligned half AMO (no MAG) -> cause 6/7");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t off = HZABHA_MAG_DECLARED ? HZABHA_H_MIS_STRADDLE
                                        : HZABHA_H_MIS_INTRA;
    uintptr_t mis = (uintptr_t)test_data_area + off;

    /* Part A: hedeleg[6]/[7]=0 -> captured at HS/M level. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint64_t *)((uintptr_t)test_data_area & ~15UL) =
        0x0011223344556677ULL;
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_h, mis);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    printf("  [INFO] (A) misaligned half AMO @+off=%lu cause=%lu fired=%d\n",
           (unsigned long)off, (unsigned long)cause_a, (int)fired_a);
    TEST_ASSERT("(A) misaligned half AMO faulted (not MAG-covered)", fired_a);
    TEST_ASSERT("(A) cause in store/AMO class {6,7} (never load 4/5)",
                hzabha_cause_in2(cause_a, CAUSE_STORE_ADDR_MISALIGN,
                                 CAUSE_STORE_ACCESS_FAULT));

    /* Part B: hedeleg[6]/[7]=1 -> delivered to VS-mode. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint64_t *)((uintptr_t)test_data_area & ~15UL) =
        0x0011223344556677ULL;
    hyp_delegate_to_vs((1UL << 6) | (1UL << 7), 0);
    hz_vs_handler_install();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_h, mis);
    bool vs_b = g_hz_vs_triggered;
    uintptr_t cause_b = g_hz_vs_cause;
    ts2_finish(&ctx);

    TEST_ASSERT("(B) misaligned half AMO delivered to VS-mode", vs_b);
    TEST_ASSERT("(B) vscause in store/AMO class {6,7}",
                hzabha_cause_in2(cause_b, CAUSE_STORE_ADDR_MISALIGN,
                                 CAUSE_STORE_ACCESS_FAULT));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-24: MAG intra-granule misaligned half AMO -> no alignment
 *            exception, executes atomically. SKIP if no MAG declared.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_24_mag_intra_no_fault);
bool test_hzabha_24_mag_intra_no_fault(void)
{
    TEST_BEGIN("HZABHA-24: MAG intra-granule misaligned half AMO -> no fault");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    if (!HZABHA_MAG_DECLARED)
        TEST_SKIP("platform declares no misaligned atomicity granule (Zama16b)");

    uintptr_t va = (uintptr_t)test_data_area;
    uintptr_t mis = va + HZABHA_H_MIS_INTRA;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint64_t *)va = 0x0011223344556677ULL;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_h, mis);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  [INFO] MAG intra-granule half AMO trapped cause=%lu\n",
               (unsigned long)cause);
    TEST_ASSERT("MAG intra-granule misaligned half AMO raised no alignment "
                "exception (norm:misaligned_atomicity_granule_size)", !fired);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-25: MAG intra-granule misaligned half AMO with G-stage W=0 ->
 *            cause 23, htinst Addr. Offset == 0. SKIP if no MAG.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_25_mag_intra_gstage_fault);
bool test_hzabha_25_mag_intra_gstage_fault(void)
{
    TEST_BEGIN("HZABHA-25: MAG intra-granule half AMO G-stage W=0 -> cause 23");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    if (!HZABHA_MAG_DECLARED)
        TEST_SKIP("platform declares no misaligned atomicity granule (Zama16b)");

    uintptr_t va = (uintptr_t)test_fault_page;
    uintptr_t mis = va + HZABHA_H_MIS_INTRA;

    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint64_t *)va = 0x0011223344556677ULL;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_h, mis);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = fired ? trap_get_htval() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    printf("  [INFO] MAG intra G-stage half AMO: fired=%d cause=%lu htval=0x%lx\n",
           (int)fired, (unsigned long)cause, (unsigned long)htval);
    TEST_ASSERT("MAG intra-granule half AMO G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    TEST_ASSERT("htval == 0 or GPA>>2", htval == 0 || htval == (va >> 2));
    if (htinst != 0)
        TEST_ASSERT_EQ("htinst Addr. Offset == 0 (single memory op)",
                       (htinst >> 15) & 0x1FUL, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-26: misaligned half AMO trapped at HS/M level: cause 6/7, stval
 *            == the misaligned address, GVA=1, SPV=1, htval=0,
 *            Addr.Offset=0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_26_misaligned_trap_context);
bool test_hzabha_26_misaligned_trap_context(void)
{
    TEST_BEGIN("HZABHA-26: misaligned half AMO HS-mode trap context");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t off = HZABHA_MAG_DECLARED ? HZABHA_H_MIS_STRADDLE
                                        : HZABHA_H_MIS_INTRA;
    uintptr_t mis = (uintptr_t)test_data_area + off;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint64_t *)((uintptr_t)test_data_area & ~15UL) =
        0x0011223344556677ULL;
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    hz_clear_gva_spv();
    hz_route_to_hs((1UL << 6) | (1UL << 7));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_h, mis);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t tval = fired ? trap_get_tval() : 0;
    uintptr_t htval = fired ? trap_get_htval() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    trap_expect_end();
    hz_unroute_from_hs((1UL << 6) | (1UL << 7));
    ts2_finish(&ctx);

    printf("  [INFO] misaligned half AMO: cause=%lu tval=0x%lx gva=%d spv=%d\n",
           (unsigned long)cause, (unsigned long)tval, (int)gva, (int)spv);
    TEST_ASSERT("misaligned half AMO faulted", fired);
    TEST_ASSERT("cause in store/AMO class {6,7}",
                hzabha_cause_in2(cause, CAUSE_STORE_ADDR_MISALIGN,
                                 CAUSE_STORE_ACCESS_FAULT));
    TEST_ASSERT_EQ("stval == misaligned address itself", tval, mis);
    TEST_ASSERT_EQ("GVA=1", (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("SPV=1", spv);
    TEST_ASSERT_EQ("htval == 0 (not a guest-page fault)", htval, (uintptr_t)0);
    if (htinst != 0)
        TEST_ASSERT_EQ("htinst Addr. Offset == 0", (htinst >> 15) & 0x1FUL,
                       (uintptr_t)0);

    HYP_TEST_END();
}
