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
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    hz_clear_vtw();
    hz_clear_tw();
    (void)hz_reserve();

    goto_priv(PRIV_S);
    PRIV_DO(EXEC_WRS_NTO());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("HS-mode wrs.nto");

    goto_priv(PRIV_S);
    PRIV_DO(EXEC_WRS_STO());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("HS-mode wrs.sto");

    HYP_TEST_END();
}

/* ---- HZWRS-02: VS-mode wrs.nto/wrs.sto normal ---- */

TEST_REGISTER(test_hzwrs_02);
bool test_hzwrs_02(void)
{
    TEST_BEGIN("HZWRS-02: VS-mode wrs.nto/wrs.sto normal (VTW=0)");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    hz_clear_vtw();
    hz_clear_tw();
    (void)hz_reserve();

    hz_vs_expect(_vs_wrs_nto, false, 0);
    hz_vs_expect(_vs_wrs_sto, false, 0);

    HYP_TEST_END();
}

/* ---- HZWRS-03: VU-mode wrs.nto/wrs.sto normal ---- */

TEST_REGISTER(test_hzwrs_03);
bool test_hzwrs_03(void)
{
    TEST_BEGIN("HZWRS-03: VU-mode wrs.nto/wrs.sto normal (VTW=0)");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    hz_clear_vtw();
    hz_clear_tw();
    (void)hz_reserve();

    hz_vu_expect(_vu_wrs_nto, false, 0);
    hz_vu_expect(_vu_wrs_sto, false, 0);

    HYP_TEST_END();
}
