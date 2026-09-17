/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.2 - exception-class split and permission requirements
 *             (HZLASR-05 ~ HZLASR-11)
 *
 * Spec basis:
 *   norm:ldaq_atomic_load_op - a load-acquire is a PURE read: it only
 *     needs READ permission, so an R=1/W=0 page MUST execute normally
 *     (HZLASR-06/10, forced positive) and it never sets a data-page D bit.
 *   norm:sdrl_atomic_store_op / norm:store_page_fault_no_w - a
 *     store-release is a PURE write: it needs WRITE permission, so an
 *     R=1/W=0 or R=0 page raises a store/AMO page-fault (15). The store
 *     class is UNAMBIGUOUS -> hard assert.
 *   norm:mcause_exccode_ld_ldrsv / norm:mcause_exccode_st_sc_amo -
 *     load-acquire -> load class (4/5/13/21), store-release -> store/AMO
 *     class (6/7/15/23). zalasr.adoc does NOT pin the load-acquire class
 *     (AMO opcode space), so the load-acquire cause is RECORD-AND-COMPARE.
 *   norm:hedeleg_acc / norm:hedeleg_op - hedeleg[4/5/6/7/13/15] Writable,
 *     hedeleg[21/23] read-only-0 (VS-stage delegable, G-stage forced HS).
 * =================================================================== */

/* Run a store-release against a VS-stage victim and report the trap.
 * store-release is unambiguous (store/AMO class), so this hard-checks
 * the expected cause. delegate_to_vs routes cause 15 to VS-mode via
 * hedeleg; otherwise the fault is captured at HS/M level. */
static int hzlasr_store_vs_fault(uintptr_t victim_va, uintptr_t vs_flags,
                                 uintptr_t (*probe)(uintptr_t),
                                 int delegate_to_vs, uintptr_t exp_cause,
                                 const char *tag)
{
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, victim_va, vs_flags);
    hz_st_val = 0x00009abcu;

    int fired;
    uintptr_t cause = 0;
    if (delegate_to_vs) {
        hyp_delegate_to_vs((1UL << CAUSE_STORE_PAGE_FAULT), 0);
        hz_vs_handler_install();
        (void)two_stage_run_in_vs(&ctx, probe, victim_va);
        fired = g_hz_vs_triggered;
        cause = g_hz_vs_cause;
    } else {
        hedeleg_write(hedeleg_read() & ~(1UL << CAUSE_STORE_PAGE_FAULT));
        trap_expect_begin();
        (void)two_stage_run_in_vs(&ctx, probe, victim_va);
        fired = trap_was_triggered();
        cause = fired ? trap_get_cause() : 0;
        trap_expect_end();
    }
    ts2_finish(&ctx);

    printf("  [INFO] %s: fired=%d cause=%lu (expected %lu)%s\n",
           tag, fired, (unsigned long)cause, (unsigned long)exp_cause,
           delegate_to_vs ? " [deleg->VS]" : "");
    return fired && cause == exp_cause;
}

/* Run a load-acquire against a VS-stage victim and report the trap.
 * load-acquire cause is RECORD-AND-COMPARE (SPEC-ambiguous), so this
 * captures the fault on BOTH the VS-delivery path (hedeleg[13]=1) and the
 * HS/M path (any non-delegated cause) and reports the observed cause via
 * out-params without hard-asserting the class. */
static int hzlasr_load_vs_fault(uintptr_t victim_va, uintptr_t vs_flags,
                                uintptr_t (*probe)(uintptr_t),
                                uintptr_t *obs_cause, uintptr_t *obs_rd)
{
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, victim_va, vs_flags);

    hyp_delegate_to_vs((1UL << CAUSE_LOAD_PAGE_FAULT), 0);
    hz_vs_handler_install();
    trap_expect_begin();
    uintptr_t rd = two_stage_run_in_vs(&ctx, probe, victim_va);
    bool fired_m = trap_was_triggered();
    uintptr_t cause_m = fired_m ? trap_get_cause() : 0;
    trap_expect_end();
    bool fired_vs = g_hz_vs_triggered;
    uintptr_t cause_vs = g_hz_vs_cause;
    ts2_finish(&ctx);

    if (obs_rd) *obs_rd = rd;
    if (fired_vs)      { if (obs_cause) *obs_cause = cause_vs; return 1; }
    else if (fired_m)  { if (obs_cause) *obs_cause = cause_m; return 1; }
    if (obs_cause) *obs_cause = 0;
    return 0;
}

/* ------------------------------------------------------------------
 * HZLASR-05: hedeleg bit-property probe for the Zalasr causes.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_05_hedeleg_bits);
bool test_hzlasr_05_hedeleg_bits(void)
{
    TEST_BEGIN("HZLASR-05: hedeleg 4/5/13/6/7/15 writable, 21/23 RO-0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");

    uintptr_t load_bits  = (1UL << 4) | (1UL << 5) | (1UL << 13);
    uintptr_t store_bits = (1UL << 6) | (1UL << 7) | (1UL << 15);
    uintptr_t g_bits     = (1UL << 21) | (1UL << 23);

    uintptr_t saved = hedeleg_read();
    hedeleg_write(load_bits | store_bits | g_bits);
    uintptr_t rb = hedeleg_read();

    TEST_ASSERT_BITS("hedeleg load-side VS bits (4/5/13) writable",
                     rb, load_bits, load_bits);
    TEST_ASSERT_BITS("hedeleg store-side VS bits (6/7/15) writable",
                     rb, store_bits, store_bits);
    TEST_ASSERT_BITS("hedeleg G-stage bits (21/23) read-only-0",
                     rb, g_bits, (uintptr_t)0);

    hedeleg_write(saved);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-06: load-acquire to an R=1/W=0 VS-stage page executes normally
 *            (only needs read permission). FORCED POSITIVE assertion: a
 *            standalone atomic load MUST be able to read read-only memory.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_06_load_acq_ro_page_executes);
bool test_hzlasr_06_load_acq_ro_page_executes(void)
{
    TEST_BEGIN("HZLASR-06: load-acquire to R=1/W=0 VS page executes (read ok)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    /* The test region is identity-mapped (VA==GPA==SPA), so preset the
     * target from M-mode before the helper rebuilds the VS-stage victim. */
    hzlasr_store_le64(va, 0x0000000000005678ULL);

    /* hedeleg[13]=1 + a VS handler so a wrongly-faulting load-acquire is
     * captured (at VS for cause 13, at M for any other cause) rather than
     * crashing. A correct platform takes no fault at all. */
    uintptr_t obs = 0, rd = 0;
    int fired = hzlasr_load_vs_fault(va, HZ_VS_R, hz_vs_lw_aq, &obs, &rd);

    if (fired)
        printf("  [DEVIATION] lw.aq to R=1/W=0 trapped cause=%lu - a pure "
               "atomic load MUST read a read-only page; record to bugs/ "
               "(violates norm:ldaq_atomic_load_op)\n", (unsigned long)obs);
    TEST_ASSERT("lw.aq to R=1/W=0 executed with no fault (read-only ok)",
                !fired);
    if (!fired)
        TEST_ASSERT_EQ("lw.aq loaded the read-only page value",
                       rd, ZALASR_W_SIGNEXT(0x00005678u));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-07: load-acquire to an R=0 VS-stage page faults; the exact cause
 *            is RECORD-AND-COMPARE (functional expectation: load pf 13).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_07_load_acq_r0_record);
bool test_hzlasr_07_load_acq_r0_record(void)
{
    TEST_BEGIN("HZLASR-07: load-acquire to R=0 VS page -> fault (record cause)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    uintptr_t obs = 0, rd = 0;
    int fired = hzlasr_load_vs_fault(va, HZ_VS_XONLY, hz_vs_lw_aq, &obs, &rd);
    (void)rd;

    TEST_ASSERT("load-acquire to R=0 faulted", fired);
    hzlasr_record_load_cause("load-acquire to R=0 (expect load pf 13)", obs);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-08: store-release to an R=1/W=0 VS-stage page -> store page-fault
 *            (15), delegated to VS-mode. FORCED (store class unambiguous).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_08_store_rel_w0_store_fault);
bool test_hzlasr_08_store_rel_w0_store_fault(void)
{
    TEST_BEGIN("HZLASR-08: store-release to R=1/W=0 -> store pf (15) to VS");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    int ok_w = hzlasr_store_vs_fault(va, HZ_VS_R, hz_vs_sw_rl,
                                     /*deleg*/1, CAUSE_STORE_PAGE_FAULT,
                                     "sw.rl to W=0");
    TEST_ASSERT("sw.rl to W=0 raises store pf (15) delivered to VS", ok_w);
#if __riscv_xlen == 64
    int ok_d = hzlasr_store_vs_fault(va, HZ_VS_R, hz_vs_sd_rl,
                                     /*deleg*/1, CAUSE_STORE_PAGE_FAULT,
                                     "sd.rl to W=0");
    TEST_ASSERT("sd.rl to W=0 raises store pf (15) delivered to VS", ok_d);
#endif

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-09: store-release to an unreadable (R=0) page -> store class
 *            (15), NEVER load class (13).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_09_store_rel_unreadable_store_class);
bool test_hzlasr_09_store_rel_unreadable_store_class(void)
{
    TEST_BEGIN("HZLASR-09: store-release to R=0 -> store pf (15), not load");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    int ok = hzlasr_store_vs_fault(va, HZ_VS_XONLY, hz_vs_sw_rl,
                                   /*deleg*/0, CAUSE_STORE_PAGE_FAULT,
                                   "sw.rl to R=0");
    TEST_ASSERT("sw.rl to R=0 raises store pf (15), not load (13)", ok);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-10: permission contrast on the same R=1/W=0 page: lw.aq executes
 *            (read only), sw.rl faults with store pf (15) (needs write).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_10_load_vs_store_permission);
bool test_hzlasr_10_load_vs_store_permission(void)
{
    TEST_BEGIN("HZLASR-10: same R=1/W=0 page: lw.aq ok, sw.rl -> store pf (15)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    hzlasr_store_le32(va, 0x00005678u);

    /* load-acquire: normal execution. */
    uintptr_t obs = 0, rd = 0;
    int ld_fired = hzlasr_load_vs_fault(va, HZ_VS_R, hz_vs_lw_aq, &obs, &rd);
    if (ld_fired)
        printf("  [DEVIATION] lw.aq to R=1/W=0 trapped cause=%lu (must read a "
               "read-only page); record to bugs/\n", (unsigned long)obs);
    TEST_ASSERT("lw.aq to R=1/W=0 executed with no fault (read ok)", !ld_fired);
    if (!ld_fired)
        TEST_ASSERT_EQ("lw.aq loaded the value", rd, ZALASR_W_SIGNEXT(0x00005678u));

    /* store-release: store page-fault (15). */
    int ok_st = hzlasr_store_vs_fault(va, HZ_VS_R, hz_vs_sw_rl,
                                      /*deleg*/0, CAUSE_STORE_PAGE_FAULT,
                                      "sw.rl to R=1/W=0");
    TEST_ASSERT("sw.rl to R=1/W=0 raises store pf (15) (needs write)", ok_st);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-11: hedeleg=0 -> both the load-acquire (R=0) and store-release
 *            (R=1/W=0) VS-stage faults stay at HS/M level. store-release
 *            cause is 15 (forced); load-acquire cause is recorded.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_11_hedeleg0_to_hs);
bool test_hzlasr_11_hedeleg0_to_hs(void)
{
    TEST_BEGIN("HZLASR-11: hedeleg=0 -> load/store-release VS fault at HS/M");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    /* load-acquire to R=0, hedeleg[13]=0 -> captured at HS/M; record cause. */
    two_stage_ctx_t ctx;
    ts2_setup_with_vs_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, HZ_VS_XONLY);
    hedeleg_write(hedeleg_read() & ~(1UL << CAUSE_LOAD_PAGE_FAULT));
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_lw_aq, va);
    bool ld_fired = trap_was_triggered();
    uintptr_t ld_cause = ld_fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("load-acquire VS-stage fault captured at HS/M (hedeleg=0)",
                ld_fired);
    hzlasr_record_load_cause("load-acquire to R=0 (hedeleg=0, expect 13)",
                             ld_cause);

    /* store-release to R=1/W=0, hedeleg[15]=0 -> captured at HS/M, cause 15. */
    int ok_st = hzlasr_store_vs_fault(va, HZ_VS_R, hz_vs_sw_rl,
                                      /*deleg*/0, CAUSE_STORE_PAGE_FAULT,
                                      "sw.rl to W=0, hedeleg=0");
    TEST_ASSERT("store-release fault captured at HS/M (cause 15)", ok_st);

    HYP_TEST_END();
}
