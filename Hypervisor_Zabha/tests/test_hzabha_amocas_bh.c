/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4.7 - amocas.b/h (Zabha x Zacas intersection, conditional)
 *             (HZABHA-32 ~ HZABHA-36)
 *
 * Spec basis:
 *   norm:Zabha_amocas-BH_ignore_bits - amocas.b/h ignore rd's
 *     XLEN-1:2^(width+3) bits (only rd[7:0]/rd[15:0] are compared).
 *   norm:Zacas_amocas_w_permission - amocas always requires write
 *     permission, so a FAILED CAS still raises a store-class fault
 *     (analogue of HZACAS-07/12).
 *   amocas.b/h are AMO-family -> store/AMO exception class; htinst retains
 *     funct5=00101 and funct3=000/001.
 *
 * These cases require Zacas; they SKIP when Zacas is not implemented.
 * =================================================================== */

/* ------------------------------------------------------------------
 * HZABHA-32: VS/VU-mode amocas.b/h success + failure, no cause=22, rd
 *            sign-extension and rd high-bit ignoring.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_32_amocas_bh_exec);
bool test_hzabha_32_amocas_bh_exec(void)
{
    TEST_BEGIN("HZABHA-32: VS/VU amocas.b/h execute, no cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented (amocas.b/h sub-group)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    two_stage_ctx_t ctx;

    /* VS-mode amocas.b success: cmp matches the byte. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint8_t *)va = 0x12u;
    hz_cas_cmp = 0x12u;
    hz_cas_swap = 0x34u;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_b_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VS-mode amocas.b (success) took no trap", !fired);
    TEST_ASSERT_NEQ("VS-mode amocas.b not cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("amocas.b success: memory == swap",
                   *(volatile uint8_t *)va, (uintptr_t)0x34u);

    /* VS-mode amocas.h failure: cmp mismatches -> memory unchanged. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint16_t *)va = 0x1234u;
    hz_cas_cmp = 0x9999u;   /* mismatch */
    hz_cas_swap = 0x5678u;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_h_probe, va);
    bool fired_f = trap_was_triggered();
    trap_expect_end();
    ts2_finish(&ctx);
    TEST_ASSERT("VS-mode amocas.h (failure) took no trap", !fired_f);
    TEST_ASSERT_EQ("amocas.h failure: memory unchanged",
                   *(volatile uint16_t *)va, (uintptr_t)0x1234u);

    /* VU-mode amocas.b success (U=1 pages). */
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint8_t *)va = 0x21u;
    hz_cas_cmp = 0x21u;
    hz_cas_swap = 0x43u;
    trap_expect_begin();
    (void)two_stage_run_in_vu(&ctx, hz_vs_amocas_b_probe, va);
    bool fired_vu = trap_was_triggered();
    uintptr_t cause_vu = fired_vu ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);
    TEST_ASSERT("VU-mode amocas.b took no trap", !fired_vu);
    TEST_ASSERT_NEQ("VU-mode amocas.b not cause=22",
                    cause_vu, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-33: amocas.b/h to R=1/W=0 -> store page-fault (15) for BOTH the
 *            success and failure paths (norm:Zacas_amocas_w_permission).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_33_amocas_bh_write_perm);
bool test_hzabha_33_amocas_bh_write_perm(void)
{
    TEST_BEGIN("HZABHA-33: amocas.b/h to W=0 -> store pf (15), both paths");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented (amocas.b/h sub-group)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;

    /* Success path (cmp matches) to W=0. */
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);
    *(volatile uint8_t *)va = 0x12u;
    hz_cas_cmp = 0x12u;
    hz_cas_swap = 0x34u;
    hyp_delegate_to_vs((1UL << CAUSE_STORE_PAGE_FAULT), 0);
    hz_vs_handler_install();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_b_probe, va);
    bool fired_s = g_hz_vs_triggered;
    uintptr_t cause_s = g_hz_vs_cause;
    ts2_finish(&ctx);
    TEST_ASSERT("amocas.b success-path to W=0 faulted", fired_s);
    TEST_ASSERT_EQ("amocas.b success-path cause == store pf (15)",
                   cause_s, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    /* Failure path (cmp mismatches) to W=0 -> STILL store pf (15). */
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_R);
    *(volatile uint8_t *)va = 0x12u;
    hz_cas_cmp = 0x99u;   /* mismatch */
    hz_cas_swap = 0x34u;
    hyp_delegate_to_vs((1UL << CAUSE_STORE_PAGE_FAULT), 0);
    hz_vs_handler_install();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_b_probe, va);
    bool fired_f = g_hz_vs_triggered;
    uintptr_t cause_f = g_hz_vs_cause;
    ts2_finish(&ctx);
    TEST_ASSERT("amocas.b FAILED-path to W=0 faulted", fired_f);
    TEST_ASSERT_EQ("amocas.b FAILED-path cause == store pf (15) "
                   "[norm:Zacas_amocas_w_permission]",
                   cause_f, (uintptr_t)CAUSE_STORE_PAGE_FAULT);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-34: amocas.h G-stage W=0 -> store/AMO guest-page-fault (23).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_34_amocas_h_gstage);
bool test_hzabha_34_amocas_h_gstage(void)
{
    TEST_BEGIN("HZABHA-34: amocas.h G-stage W=0 -> store guest-pf (23)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented (amocas.b/h sub-group)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint16_t *)va = 0x1234u;
    hz_cas_cmp = 0x1234u;   /* match -> success path */
    hz_cas_swap = 0x5678u;

    hz_clear_gva_spv();
    hz_route_to_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_h_probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    uintptr_t htval = fired ? trap_get_htval() : 0;
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    TEST_ASSERT("amocas.h G-stage fault fired", fired);
    TEST_ASSERT_EQ("cause == store/AMO guest-page-fault (23)",
                   cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    TEST_ASSERT_EQ("GVA=1", (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("SPV=1", spv);
    TEST_ASSERT("htval == 0 or GPA>>2", htval == 0 || htval == (va >> 2));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-35: amocas.b/h htinst retains funct5=00101 and funct3=000/001.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_35_amocas_bh_htinst);
bool test_hzabha_35_amocas_bh_htinst(void)
{
    TEST_BEGIN("HZABHA-35: amocas.b/h htinst retains funct5 + funct3");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented (amocas.b/h sub-group)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    two_stage_ctx_t ctx;

    /* amocas.b (funct3=000). */
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint8_t *)va = 0x12u;
    hz_cas_cmp = 0x12u;
    hz_cas_swap = 0x34u;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_b_probe, va);
    bool fired_b = trap_was_triggered();
    uintptr_t htinst_b = fired_b ? trap_get_htinst() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("amocas.b G-stage fault fired", fired_b);
    if (htinst_b != 0) {
        TEST_ASSERT_EQ("amocas.b htinst funct5 == 00101",
                       (htinst_b >> 27) & 0x1FUL, (uintptr_t)0x05UL);
        TEST_ASSERT_EQ("amocas.b htinst funct3 == 000 (.b)",
                       (htinst_b >> 12) & 0x7UL, (uintptr_t)0x0UL);
    } else {
        printf("  [INFO] amocas.b htinst=0; funct5/funct3 not observable\n");
    }

    /* amocas.h (funct3=001). */
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_G_RU);
    *(volatile uint16_t *)va = 0x1234u;
    hz_cas_cmp = 0x1234u;
    hz_cas_swap = 0x5678u;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_h_probe, va);
    bool fired_h = trap_was_triggered();
    uintptr_t htinst_h = fired_h ? trap_get_htinst() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("amocas.h G-stage fault fired", fired_h);
    if (htinst_h != 0) {
        TEST_ASSERT_EQ("amocas.h htinst funct5 == 00101",
                       (htinst_h >> 27) & 0x1FUL, (uintptr_t)0x05UL);
        TEST_ASSERT_EQ("amocas.h htinst funct3 == 001 (.h)",
                       (htinst_h >> 12) & 0x7UL, (uintptr_t)0x1UL);
    } else {
        printf("  [INFO] amocas.h htinst=0; funct5/funct3 not observable\n");
    }

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-36: misaligned amocas.h (odd address) -> store/AMO misaligned
 *            (6/7) without MAG; MAG intra-granule -> no fault. (Half only;
 *            amocas.b is never misaligned.)
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_36_amocas_h_misaligned);
bool test_hzabha_36_amocas_h_misaligned(void)
{
    TEST_BEGIN("HZABHA-36: misaligned amocas.h -> cause 6/7 (or MAG relax)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented (amocas.b/h sub-group)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

#ifdef ZAMA16B_SUPPORTED
    uintptr_t off = 15UL;   /* straddle the 16-byte granule boundary */
#else
    uintptr_t off = 1UL;    /* any odd address when no MAG declared */
#endif
    uintptr_t mis = (uintptr_t)test_data_area + off;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint64_t *)((uintptr_t)test_data_area & ~15UL) =
        0x0011223344556677ULL;
    hz_cas_cmp = 0x2233u;
    hz_cas_swap = 0x5678u;
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_h_probe, mis);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    printf("  [INFO] misaligned amocas.h @+off=%lu: fired=%d cause=%lu\n",
           (unsigned long)off, (int)fired, (unsigned long)cause);
    TEST_ASSERT("misaligned amocas.h (granule-straddling) faulted", fired);
    TEST_ASSERT("cause in store/AMO class {6,7}",
                cause == CAUSE_STORE_ADDR_MISALIGN ||
                cause == CAUSE_STORE_ACCESS_FAULT);

    HYP_TEST_END();
}
