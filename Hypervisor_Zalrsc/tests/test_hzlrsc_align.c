/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.6 - Misaligned LR/SC exception path (HZLRSC-27 ~ HZLRSC-29)
 *
 * Spec basis:
 *   norm:lr_sc_alignment - LR/SC require natural alignment; a misaligned
 *     LR/SC raises an address-misaligned OR access-fault exception. Both
 *     flavours are compliant, so the actual cause is recorded and checked
 *     against the expected class set. Zalrsc has NO misaligned-atomicity
 *     -granule (MAG) relaxation (unlike Zalasr), so a misaligned LR/SC
 *     always faults and never splits: stval == the misaligned address
 *     itself and htinst Addr. Offset == 0.
 *   norm:hedeleg_op - hedeleg bit set -> delivered to VS-mode; clear ->
 *     stays at the hypervisor (HS/M) level.
 *
 * LR misaligned -> load class {4,5}; SC misaligned -> store/AMO {6,7}.
 *
 * NOTE: a misaligned SC can never hold a matching reservation (the LR at
 * the same misaligned address faults first), so it is always a FAILED SC.
 * QEMU short-circuits failed SCs and skips the alignment/permission check
 * (see bugs/qemu_zalrsc_failed_sc_semantics_bugs.md defect 3); Spike/Sail
 * raise the store/AMO misaligned fault as the SPEC requires. The SC cases
 * therefore stay FAIL on QEMU and are not relaxed.
 * =================================================================== */

static inline bool hz_cause_in2(uintptr_t c, uintptr_t a, uintptr_t b)
{
    return c == a || c == b;
}

/* ------------------------------------------------------------------
 * HZLRSC-27: misaligned LR -> load address-misaligned (4) or load access
 *            fault (5); delivery follows hedeleg[4]/[5].
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_27_misaligned_lr);
bool test_hzlrsc_27_misaligned_lr(void)
{
    TEST_BEGIN("HZLRSC-27: misaligned LR -> cause 4/5, deleg per hedeleg");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t mis = (uintptr_t)test_data_area + 2;   /* 2-byte aligned */

    /* Part A: hedeleg[4]/[5] = 0 -> captured at HS/M level. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hedeleg_write(hedeleg_read() & ~((1UL << 4) | (1UL << 5)));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, mis);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("(A) misaligned LR faulted", fired_a);
    TEST_ASSERT("(A) cause in load class {4,5}",
                hz_cause_in2(cause_a, CAUSE_LOAD_ADDR_MISALIGN,
                             CAUSE_LOAD_ACCESS_FAULT));
    printf("  [INFO] (A) misaligned LR cause=%lu\n", (unsigned long)cause_a);

    /* Part B: hedeleg[4]/[5] = 1 -> delivered to VS-mode. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hyp_delegate_to_vs((1UL << 4) | (1UL << 5), 0);
    hz_vs_handler_install();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, mis);
    bool vs_b = g_hz_vs_triggered;
    uintptr_t cause_b = g_hz_vs_cause;
    ts2_finish(&ctx);

    TEST_ASSERT("(B) misaligned LR delivered to VS-mode", vs_b);
    TEST_ASSERT("(B) vscause in load class {4,5}",
                hz_cause_in2(cause_b, CAUSE_LOAD_ADDR_MISALIGN,
                             CAUSE_LOAD_ACCESS_FAULT));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-28: misaligned SC -> store/AMO address-misaligned (6) or access
 *            fault (7); delivery follows hedeleg[6]/[7].
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_28_misaligned_sc);
bool test_hzlrsc_28_misaligned_sc(void)
{
    TEST_BEGIN("HZLRSC-28: misaligned SC -> cause 6/7, deleg per hedeleg");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t mis = (uintptr_t)test_data_area + 2;

    /* Part A: hedeleg[6]/[7] = 0 -> captured at HS/M level. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sc_w, mis);
    bool fired_a = trap_was_triggered();
    uintptr_t cause_a = fired_a ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("(A) misaligned SC faulted", fired_a);
    TEST_ASSERT("(A) cause in store/AMO class {6,7}",
                hz_cause_in2(cause_a, CAUSE_STORE_ADDR_MISALIGN,
                             CAUSE_STORE_ACCESS_FAULT));
    printf("  [INFO] (A) misaligned SC cause=%lu\n", (unsigned long)cause_a);

    /* Part B: hedeleg[6]/[7] = 1 -> delivered to VS-mode. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hyp_delegate_to_vs((1UL << 6) | (1UL << 7), 0);
    hz_vs_handler_install();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sc_w, mis);
    bool vs_b = g_hz_vs_triggered;
    uintptr_t cause_b = g_hz_vs_cause;
    ts2_finish(&ctx);

    TEST_ASSERT("(B) misaligned SC delivered to VS-mode", vs_b);
    TEST_ASSERT("(B) vscause in store/AMO class {6,7}",
                hz_cause_in2(cause_b, CAUSE_STORE_ADDR_MISALIGN,
                             CAUSE_STORE_ACCESS_FAULT));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-29: misaligned LR/SC trap context: stval == the misaligned
 *            address (== original VA, no split), GVA=1, SPV=1, htval=0
 *            (not a guest-page fault), htinst Addr.Offset=0.
 *            Routed into HS-mode so hstatus.GVA/SPV are written.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_29_misaligned_trap_context);
bool test_hzlrsc_29_misaligned_trap_context(void)
{
    TEST_BEGIN("HZLRSC-29: misaligned LR/SC HS-mode trap context");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t mis = (uintptr_t)test_data_area + 2;

    /* LR misaligned: route causes 4/5 into HS-mode for GVA/SPV. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hedeleg_write(hedeleg_read() & ~((1UL << 4) | (1UL << 5)));
    hz_clear_gva_spv();
    hz_route_to_hs((1UL << 4) | (1UL << 5));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lr_w, mis);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t tval = fired ? trap_get_tval() : 0;
    uintptr_t htval = fired ? trap_get_htval() : 0;
    uintptr_t htinst = fired ? trap_get_htinst() : 0;
    bool gva = trap_get_gva();
    bool spv = trap_get_spv();
    trap_expect_end();
    hz_unroute_from_hs((1UL << 4) | (1UL << 5));
    ts2_finish(&ctx);

    printf("  [INFO] misaligned LR: cause=%lu tval=0x%lx gva=%d spv=%d\n",
           (unsigned long)cause, (unsigned long)tval, (int)gva, (int)spv);
    TEST_ASSERT("misaligned LR faulted", fired);
    TEST_ASSERT("LR cause in load class {4,5}",
                hz_cause_in2(cause, CAUSE_LOAD_ADDR_MISALIGN,
                             CAUSE_LOAD_ACCESS_FAULT));
    TEST_ASSERT_EQ("stval == misaligned address itself (no split)", tval, mis);
    TEST_ASSERT_EQ("misaligned LR: GVA=1", (uintptr_t)gva, (uintptr_t)1);
    TEST_ASSERT("misaligned LR: SPV=1", spv);
    TEST_ASSERT_EQ("htval == 0 (not a guest-page fault)", htval, (uintptr_t)0);
    if (htinst != 0)
        TEST_ASSERT_EQ("htinst Addr. Offset (bits 19:15) == 0",
                       (htinst >> 15) & 0x1FUL, (uintptr_t)0);

    /* SC misaligned: same context, store/AMO class. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    hedeleg_write(hedeleg_read() & ~((1UL << 6) | (1UL << 7)));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_sc_w, mis);
    bool fired2 = trap_was_triggered();
    uintptr_t cause2 = fired2 ? trap_get_cause() : 0;
    uintptr_t tval2 = fired2 ? trap_get_tval() : 0;
    uintptr_t htval2 = fired2 ? trap_get_htval() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("misaligned SC faulted", fired2);
    TEST_ASSERT("SC cause in store/AMO class {6,7}",
                hz_cause_in2(cause2, CAUSE_STORE_ADDR_MISALIGN,
                             CAUSE_STORE_ACCESS_FAULT));
    TEST_ASSERT_EQ("SC stval == misaligned address itself", tval2, mis);
    TEST_ASSERT_EQ("SC htval == 0 (not a guest-page fault)",
                   htval2, (uintptr_t)0);

    HYP_TEST_END();
}
