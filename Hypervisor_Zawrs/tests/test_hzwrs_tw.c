/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 6.3: TW priority and instruction scope (VS/VU-mode)
 *
 * HZWRS-08 ~ HZWRS-12
 *
 * Spec anchors:
 *   norm:Zawrs_priv_illegal_instr_excp (zawrs.adoc)
 *     - mstatus.TW=1 + wrs.nto in a non-M mode not completing within
 *       the bounded time limit -> illegal-instruction (2). The TW
 *       clause takes precedence over VTW, so VS/VU-mode report
 *       illegal-instruction, not virtual-instruction (same semantics
 *       as the WFI case HSTAT-06 in Hypervisor_CSR_test_plan.md).
 *   norm:Zawrs_virtual_instr_excp
 *     - the VTW clause names wrs.nto only; wrs.sto is bounded by its
 *       own short timeout and is never VTW-gated.
 *     - VTW only applies when V=1, so HS-mode is unaffected.
 * =================================================================== */

/* ---- HZWRS-08: VTW=1 + TW=1 VS-mode -> illegal-instruction ---- */

TEST_REGISTER(test_hzwrs_08);
bool test_hzwrs_08(void)
{
    TEST_BEGIN("HZWRS-08: VTW=1 + TW=1 VS-mode wrs.nto -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    uintptr_t saved_mie = hz_quiet_interrupts();
    hz_set_vtw();
    hz_set_tw();
    (void)hz_reserve();

    hz_vs_expect(_vs_wrs_nto, true, CAUSE_ILLEGAL_INST);

    hz_clear_tw();
    hz_clear_vtw();
    hz_restore_interrupts(saved_mie);

    HYP_TEST_END();
}

/* ---- HZWRS-09: VTW=1 + TW=1 VU-mode -> illegal-instruction ---- */

TEST_REGISTER(test_hzwrs_09);
bool test_hzwrs_09(void)
{
    TEST_BEGIN("HZWRS-09: VTW=1 + TW=1 VU-mode wrs.nto -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    uintptr_t saved_mie = hz_quiet_interrupts();
    hz_set_vtw();
    hz_set_tw();
    (void)hz_reserve();

    hz_vu_expect(_vu_wrs_nto, true, CAUSE_ILLEGAL_INST);

    hz_clear_tw();
    hz_clear_vtw();
    hz_restore_interrupts(saved_mie);

    HYP_TEST_END();
}

/* ---- HZWRS-10: VTW=1 applies only to wrs.nto (wrs.sto normal) ---- */

TEST_REGISTER(test_hzwrs_10);
bool test_hzwrs_10(void)
{
    TEST_BEGIN("HZWRS-10: VTW=1 VS-mode wrs.sto completes normally");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    /* The VTW clause names wrs.nto only. wrs.sto is bounded by its
     * short timeout and must not raise virtual-instruction. No wake
     * source: the reference simulators complete wrs.sto without
     * stalling, and a pending interrupt risks being taken mid-test. */
    uintptr_t saved_mie = hz_quiet_interrupts();
    hz_clear_tw();
    hz_set_vtw();
    (void)hz_reserve();

    hz_vs_expect(_vs_wrs_sto, false, 0);

    hz_clear_vtw();
    hz_restore_interrupts(saved_mie);

    HYP_TEST_END();
}

/* ---- HZWRS-11: VTW does not affect HS-mode wrs.nto ---- */

TEST_REGISTER(test_hzwrs_11);
bool test_hzwrs_11(void)
{
    TEST_BEGIN("HZWRS-11: VTW=1 HS-mode wrs.nto completes normally");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    /* VTW gates only V=1 execution; HS-mode (V=0) is unaffected. */
    hz_clear_tw();
    hz_set_vtw();
    (void)hz_reserve();
    hz_suppress_globals();
    hz_set_m_soft_pending();

    goto_priv(PRIV_S);
    PRIV_DO(EXEC_WRS_NTO());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("HS-mode wrs.nto with VTW=1");

    hz_clear_m_soft_pending();
    hz_clear_vtw();

    HYP_TEST_END();
}

/* ---- HZWRS-12: VS-mode TW=1 alone -> illegal-instruction ---- */

TEST_REGISTER(test_hzwrs_12);
bool test_hzwrs_12(void)
{
    TEST_BEGIN("HZWRS-12: VS-mode TW=1/VTW=0 wrs.nto -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    /* Control case against HSTAT-06 (WFI semantics): mstatus.TW
     * alone intercepts wrs.nto in VS-mode with illegal-instruction. */
    uintptr_t saved_mie = hz_quiet_interrupts();
    hz_clear_vtw();
    hz_set_tw();
    (void)hz_reserve();

    hz_vs_expect(_vs_wrs_nto, true, CAUSE_ILLEGAL_INST);

    hz_clear_tw();
    hz_restore_interrupts(saved_mie);

    HYP_TEST_END();
}
