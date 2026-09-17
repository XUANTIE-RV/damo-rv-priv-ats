/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2.5 - Misaligned AMO and MAG relaxation (HZAMO-21 ~ HZAMO-24)
 *
 * Spec basis:
 *   norm:amo_alignment - an AMO requires natural alignment; if not aligned
 *     an address-misaligned OR access-fault exception is generated. AMO is
 *     store/AMO class, so the cause is 6/7 (NEVER load class 4/5).
 *   norm:misaligned_atomicity_granule_size - if all accessed bytes lie in
 *     the same misaligned atomicity granule (MAG), no alignment exception
 *     is raised and the AMO executes atomically as a SINGLE memory
 *     operation (Zaamo-specific; Zalrsc has no MAG). Single memory op =>
 *     htinst Addr. Offset == 0.
 *   Zama16b fixes the granule at 16 bytes for coherent cacheable memory.
 *
 * test_data_area is page-aligned (hence 16-byte aligned), so:
 *   va+2  -> bytes [va+2,va+6)  lie inside granule [va,va+16)  (relaxed)
 *   va+14 -> bytes [va+14,va+18) straddle the va+16 granule boundary (fault)
 * =================================================================== */

#ifdef ZAMA16B_SUPPORTED
#define HZAMO_MAG_DECLARED  1
#define HZAMO_GRANULE       16UL
#else
#define HZAMO_MAG_DECLARED  0
#define HZAMO_GRANULE       0UL
#endif

#define HZAMO_MIS_INTRA     2UL    /* inside one 16-byte granule */
#define HZAMO_MIS_STRADDLE  14UL   /* crosses the 16-byte granule boundary */

static inline bool hzamo_cause_in2(uintptr_t c, uintptr_t a, uintptr_t b)
{
    return c == a || c == b;
}

/* ------------------------------------------------------------------
 * HZAMO-21: misaligned AMO with NO MAG coverage -> store/AMO address
 *           misaligned (6) or access fault (7); deleg per hedeleg[6]/[7].
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_21_misaligned_amo);
bool test_hzamo_21_misaligned_amo(void)
{
    TEST_BEGIN("HZAMO-21: misaligned AMO (no MAG) -> cause 6/7");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* Granule-straddling offset when MAG is declared, else any misaligned. */
    uintptr_t off = HZAMO_MAG_DECLARED ? HZAMO_MIS_STRADDLE : HZAMO_MIS_INTRA;
    uintptr_t mis = (uintptr_t)test_data_area + off;

    /* Part A: hedeleg[6]/[7]=0 -> captured at HS/M level. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, mis);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    printf("  [INFO] (A) misaligned AMO @+off=%lu cause=%lu fired=%d\n",
           (unsigned long)off, (unsigned long)cause_a, (int)fired_a);
    TEST_ASSERT("(A) misaligned AMO faulted (not MAG-covered)", fired_a);
    TEST_ASSERT("(A) cause in store/AMO class {6,7} (never load 4/5)",
                hzamo_cause_in2(cause_a, CAUSE_STORE_ADDR_MISALIGN,
                                CAUSE_STORE_ACCESS_FAULT));

    /* Part B: hedeleg[6]/[7]=1 -> delivered to VS-mode. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hyp_delegate_to_vs((1UL << 6) | (1UL << 7), 0);
    hz_vs_handler_install();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, mis);
    bool vs_b = g_hz_vs_triggered;
    uintptr_t cause_b = g_hz_vs_cause;
    ts2_finish(&ctx);

    TEST_ASSERT("(B) misaligned AMO delivered to VS-mode", vs_b);
    TEST_ASSERT("(B) vscause in store/AMO class {6,7}",
                hzamo_cause_in2(cause_b, CAUSE_STORE_ADDR_MISALIGN,
                                CAUSE_STORE_ACCESS_FAULT));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-22: MAG intra-granule misaligned AMO -> no alignment exception,
 *           executes atomically. SKIP if the platform declares no MAG.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_22_mag_intra_no_fault);
bool test_hzamo_22_mag_intra_no_fault(void)
{
    TEST_BEGIN("HZAMO-22: MAG intra-granule misaligned AMO -> no fault");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    if (!HZAMO_MAG_DECLARED)
        TEST_SKIP("platform declares no misaligned atomicity granule (Zama16b)");

    uintptr_t mis = (uintptr_t)test_data_area + HZAMO_MIS_INTRA;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, mis);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  [INFO] MAG intra-granule AMO trapped cause=%lu\n",
               (unsigned long)cause);
    TEST_ASSERT("MAG intra-granule misaligned AMO raised no alignment "
                "exception (norm:misaligned_atomicity_granule_size)", !fired);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-23: MAG intra-granule misaligned AMO with G-stage W=0 -> cause 23,
 *           htinst Addr. Offset == 0 (single memory op). SKIP if no MAG.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_23_mag_intra_gstage_fault);
bool test_hzamo_23_mag_intra_gstage_fault(void)
{
    TEST_BEGIN("HZAMO-23: MAG intra-granule AMO G-stage W=0 -> cause 23");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    if (!HZAMO_MAG_DECLARED)
        TEST_SKIP("platform declares no misaligned atomicity granule (Zama16b)");

    uintptr_t va = (uintptr_t)test_fault_page;
    uintptr_t mis = va + HZAMO_MIS_INTRA;

    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, mis);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = fired ? trap_get_htval() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    printf("  [INFO] MAG intra G-stage AMO: fired=%d cause=%lu htval=0x%lx\n",
           (int)fired, (unsigned long)cause, (unsigned long)htval);
    TEST_ASSERT("MAG intra-granule AMO G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    TEST_ASSERT("htval == 0 or GPA>>2", htval == 0 || htval == (va >> 2));
    if (htinst != 0)
        TEST_ASSERT_EQ("htinst Addr. Offset == 0 (single memory op)",
                       (htinst >> 15) & 0x1FUL, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-24: misaligned AMO trapped at HS/M level: cause 6/7, stval == the
 *           misaligned address, GVA=1, SPV=1, htval=0, Addr. Offset=0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_24_misaligned_trap_context);
bool test_hzamo_24_misaligned_trap_context(void)
{
    TEST_BEGIN("HZAMO-24: misaligned AMO HS-mode trap context");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t off = HZAMO_MAG_DECLARED ? HZAMO_MIS_STRADDLE : HZAMO_MIS_INTRA;
    uintptr_t mis = (uintptr_t)test_data_area + off;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    hz_clear_gva_spv();
    hz_route_to_hs((1UL << 6) | (1UL << 7));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amo_add_w, mis);
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

    printf("  [INFO] misaligned AMO: cause=%lu tval=0x%lx gva=%d spv=%d\n",
           (unsigned long)cause, (unsigned long)tval, (int)gva, (int)spv);
    TEST_ASSERT("misaligned AMO faulted", fired);
    TEST_ASSERT("cause in store/AMO class {6,7}",
                hzamo_cause_in2(cause, CAUSE_STORE_ADDR_MISALIGN,
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
