/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 3.5 - Misaligned amocas and MAG relaxation (HZACAS-23 ~ HZACAS-26)
 *
 * Spec basis:
 *   norm:Zacas_amocas_rs1_addr_alignment - amocas requires natural
 *     alignment; "the same exception options apply" cross-references
 *     Zaamo (norm:amo_alignment / norm:misaligned_atomicity_granule_size),
 *     so amocas admits the address-misaligned/access-fault exceptions and
 *     the MAG relaxation. amocas is store/AMO class -> cause 6/7.
 *   Zama16b fixes the granule at 16 bytes for coherent cacheable memory.
 *
 * test_data_area is page-aligned (hence 16-byte aligned), so:
 *   va+2  -> bytes [va+2,va+6)  lie inside granule [va,va+16)  (relaxed)
 *   va+14 -> bytes [va+14,va+18) straddle the va+16 granule boundary (fault)
 * =================================================================== */

#ifdef ZAMA16B_SUPPORTED
#define HZACAS_MAG_DECLARED  1
#else
#define HZACAS_MAG_DECLARED  0
#endif

#define HZACAS_MIS_INTRA     2UL
#define HZACAS_MIS_STRADDLE  14UL

static inline bool hzacas_cause_in2(uintptr_t c, uintptr_t a, uintptr_t b)
{
    return c == a || c == b;
}

static void hzacas_setup_cas(uintptr_t va)
{
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;
}

/* ------------------------------------------------------------------
 * HZACAS-23: misaligned amocas with NO MAG coverage -> store/AMO address
 *            misaligned (6) or access fault (7); deleg per hedeleg[6]/[7].
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_23_misaligned_cas);
bool test_hzacas_23_misaligned_cas(void)
{
    TEST_BEGIN("HZACAS-23: misaligned amocas (no MAG) -> cause 6/7");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t off = HZACAS_MAG_DECLARED ? HZACAS_MIS_STRADDLE : HZACAS_MIS_INTRA;
    uintptr_t mis = (uintptr_t)test_data_area + off;

    /* Part A: hedeleg[6]/[7]=0 -> captured at HS/M level. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzacas_setup_cas((uintptr_t)test_data_area);
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, mis);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    printf("  [INFO] (A) misaligned amocas @+off=%lu cause=%lu fired=%d\n",
           (unsigned long)off, (unsigned long)cause_a, (int)fired_a);
    TEST_ASSERT("(A) misaligned amocas faulted (not MAG-covered)", fired_a);
    TEST_ASSERT("(A) cause in store/AMO class {6,7} (never load 4/5)",
                hzacas_cause_in2(cause_a, CAUSE_STORE_ADDR_MISALIGN,
                                 CAUSE_STORE_ACCESS_FAULT));

    /* Part B: hedeleg[6]/[7]=1 -> delivered to VS-mode. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzacas_setup_cas((uintptr_t)test_data_area);
    hyp_delegate_to_vs((1UL << 6) | (1UL << 7), 0);
    hz_vs_handler_install();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, mis);
    bool vs_b = g_hz_vs_triggered;
    uintptr_t cause_b = g_hz_vs_cause;
    ts2_finish(&ctx);

    TEST_ASSERT("(B) misaligned amocas delivered to VS-mode", vs_b);
    TEST_ASSERT("(B) vscause in store/AMO class {6,7}",
                hzacas_cause_in2(cause_b, CAUSE_STORE_ADDR_MISALIGN,
                                 CAUSE_STORE_ACCESS_FAULT));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-24: MAG intra-granule misaligned amocas -> no alignment
 *            exception, executes atomically. SKIP if no MAG declared.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_24_mag_intra_no_fault);
bool test_hzacas_24_mag_intra_no_fault(void)
{
    TEST_BEGIN("HZACAS-24: MAG intra-granule misaligned amocas -> no fault");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    if (!HZACAS_MAG_DECLARED)
        TEST_SKIP("platform declares no misaligned atomicity granule (Zama16b)");

    uintptr_t va = (uintptr_t)test_data_area;
    uintptr_t mis = va + HZACAS_MIS_INTRA;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzacas_setup_cas(va);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, mis);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  [INFO] MAG intra-granule amocas trapped cause=%lu\n",
               (unsigned long)cause);
    TEST_ASSERT("MAG intra-granule misaligned amocas raised no alignment "
                "exception (norm:misaligned_atomicity_granule_size)", !fired);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-25: MAG intra-granule misaligned amocas with G-stage W=0 ->
 *            cause 23, htinst Addr. Offset == 0. SKIP if no MAG.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_25_mag_intra_gstage_fault);
bool test_hzacas_25_mag_intra_gstage_fault(void)
{
    TEST_BEGIN("HZACAS-25: MAG intra-granule amocas G-stage W=0 -> cause 23");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    if (!HZACAS_MAG_DECLARED)
        TEST_SKIP("platform declares no misaligned atomicity granule (Zama16b)");

    uintptr_t va = (uintptr_t)test_fault_page;
    uintptr_t mis = va + HZACAS_MIS_INTRA;

    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    hzacas_setup_cas(va);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, mis);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = fired ? trap_get_htval() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    printf("  [INFO] MAG intra G-stage amocas: fired=%d cause=%lu htval=0x%lx\n",
           (int)fired, (unsigned long)cause, (unsigned long)htval);
    TEST_ASSERT("MAG intra-granule amocas G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    TEST_ASSERT("htval == 0 or GPA>>2", htval == 0 || htval == (va >> 2));
    if (htinst != 0)
        TEST_ASSERT_EQ("htinst Addr. Offset == 0 (single memory op)",
                       (htinst >> 15) & 0x1FUL, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-26: misaligned amocas trapped at HS/M level: cause 6/7, stval ==
 *            the misaligned address, GVA=1, SPV=1, htval=0, Addr.Offset=0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_26_misaligned_trap_context);
bool test_hzacas_26_misaligned_trap_context(void)
{
    TEST_BEGIN("HZACAS-26: misaligned amocas HS-mode trap context");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t off = HZACAS_MAG_DECLARED ? HZACAS_MIS_STRADDLE : HZACAS_MIS_INTRA;
    uintptr_t mis = (uintptr_t)test_data_area + off;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hzacas_setup_cas((uintptr_t)test_data_area);
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    hz_clear_gva_spv();
    hz_route_to_hs((1UL << 6) | (1UL << 7));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, mis);
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

    printf("  [INFO] misaligned amocas: cause=%lu tval=0x%lx gva=%d spv=%d\n",
           (unsigned long)cause, (unsigned long)tval, (int)gva, (int)spv);
    TEST_ASSERT("misaligned amocas faulted", fired);
    TEST_ASSERT("cause in store/AMO class {6,7}",
                hzacas_cause_in2(cause, CAUSE_STORE_ADDR_MISALIGN,
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
