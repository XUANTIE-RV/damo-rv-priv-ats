/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 6.1: HS/VS/VU-mode wrs normal execution (VTW=0)
 *
 * HZWRS-01 ~ HZWRS-03
 *
 * Spec anchors:
 *   zawrs.adoc - "These instructions are available in all privilege
 *   modes"; in virtualized modes wrs instructions are ordinary
 *   instructions and must NOT spuriously raise a virtual-instruction
 *   exception when hstatus.VTW=0.
 *
 * The VS/VU cases run with no interrupt pending at all: the reference
 * simulators (QEMU/Spike/Sail) complete wrs without stalling, and a
 * pending wake source would risk being taken as a real interrupt in
 * V=1 contexts. hyp_reset_state() leaves vsatp/hgatp Bare, so VS/VU
 * callbacks run on physical addresses.
 * =================================================================== */

/* ---- HZWRS-01: HS-mode wrs.nto/wrs.sto normal ---- */

TEST_REGISTER(test_hzwrs_01);
bool test_hzwrs_01(void)
{
    TEST_BEGIN("HZWRS-01: HS-mode wrs.nto/wrs.sto normal (VTW=0)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAWRS_AVAILABLE) TEST_SKIP("Zawrs not implemented");

    /* First case ONLY: the config declares Zawrs, so probe the DUT to
     * confirm it really implements the extension (aligned with the
     * ZAWRS_SUPPORTED declaration). Execute wrs.sto in M-mode
     * (trap-armed) with a locally enabled pending M-software wake so it
     * cannot stall (norm:Zawrs_exec_resume_rules); an
     * illegal-instruction trap would mean the DUT does NOT implement
     * Zawrs despite the config declaring it. No other case probes. */
    uintptr_t saved_mideleg = hz_set_m_soft_pending();
    M_TRAP_EXPECT_BEGIN();
    EXEC_WRS_STO();
    bool probe_trapped = trap_was_triggered();
    uintptr_t probe_cause = trap_get_cause();
    trap_expect_end();
    hz_clear_m_soft_pending(saved_mideleg);
    TEST_ASSERT("DUT really implements Zawrs (aligned with ZAWRS_SUPPORTED)",
                !(probe_trapped && probe_cause == CAUSE_ILLEGAL_INST));

    hz_clear_vtw();
    hz_clear_tw();
    (void)hz_reserve();

    /* A pending locally enabled SSIP, delegated to S-level with
     * sstatus.SIE=0, stays pending while the hart executes in
     * HS-mode, so the stall condition never holds and wrs.nto
     * completes deterministically. Without a wake source wrs.nto may
     * legally stall forever (zawrs.adoc: stall is permitted while no
     * locally enabled interrupt is pending). */
    hz_suppress_globals();
    saved_mideleg = hz_set_m_soft_pending();

    goto_priv(PRIV_S);
    PRIV_DO(EXEC_WRS_NTO());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("HS-mode wrs.nto");

    goto_priv(PRIV_S);
    PRIV_DO(EXEC_WRS_STO());
    goto_priv(PRIV_M);
    /* Clear before the assertion: the framework's TEST_ASSERT only
     * records a failure (execution continues), but ordering the
     * cleanup ahead of the check keeps the wake source teardown on
     * the unconditional path regardless of the record content. */
    hz_clear_m_soft_pending(saved_mideleg);
    CHECK_NO_TRAP("HS-mode wrs.sto");

    HYP_TEST_END();
}

/* ---- HZWRS-02: VS-mode wrs.nto/wrs.sto normal ---- */

TEST_REGISTER(test_hzwrs_02);
bool test_hzwrs_02(void)
{
    TEST_BEGIN("HZWRS-02: VS-mode wrs.nto/wrs.sto normal (VTW=0)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAWRS_AVAILABLE) TEST_SKIP("Zawrs not implemented");

    hz_clear_vtw();
    hz_clear_tw();
    (void)hz_reserve();

    /* A pending locally enabled VS-software interrupt (hvip.VSSIP
     * routed to VS-level with vsie.SSIE, vsstatus.SIE kept 0) stays
     * pending while the hart executes in VS-mode, so the stall
     * condition never holds and the wrs instructions complete
     * deterministically instead of legally stalling forever. */
    hz_suppress_globals();
    hz_set_vs_soft_pending();

    hz_vs_expect(_vs_wrs_nto, false, 0);
    hz_vs_expect(_vs_wrs_sto, false, 0);

    /* The clear runs on the unconditional path: hz_vs_expect()'s
     * TEST_ASSERT records failures without aborting the case, and
     * both wrs runs share one armed wake source, so the teardown must
     * stay after the second run and before HYP_TEST_END(). */
    hz_clear_vs_soft_pending();

    HYP_TEST_END();
}

/* ---- HZWRS-03: VU-mode wrs.nto/wrs.sto normal ---- */

TEST_REGISTER(test_hzwrs_03);
bool test_hzwrs_03(void)
{
    TEST_BEGIN("HZWRS-03: VU-mode wrs.nto/wrs.sto normal (VTW=0)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZAWRS_AVAILABLE) TEST_SKIP("Zawrs not implemented");

    hz_clear_vtw();
    hz_clear_tw();
    (void)hz_reserve();

    /* A pending locally enabled interrupt always targets VS or
     * above and is taken on VU-mode entry, so a pending-interrupt
     * wake source cannot be maintained in VU-mode. Verify normal
     * execution with the watchdog instead: wrs may legally stall; the
     * armed M-timer interrupt breaks the stall
     * (norm:Zawrs_exec_resume_rules) and execution resumes. */
    uintptr_t saved_mie = hz_arm_watchdog();
    trap_expect_begin();
    (void)run_in_vu_mode(_vu_wrs_nto, 0);
    trap_expect_end();
    hz_disarm_watchdog(saved_mie);
    TEST_ASSERT("VU-mode wrs.nto completed without exception",
                hz_trap_is_watchdog_only());

    saved_mie = hz_arm_watchdog();
    trap_expect_begin();
    (void)run_in_vu_mode(_vu_wrs_sto, 0);
    trap_expect_end();
    hz_disarm_watchdog(saved_mie);
    TEST_ASSERT("VU-mode wrs.sto completed without exception",
                hz_trap_is_watchdog_only());

    HYP_TEST_END();
}
