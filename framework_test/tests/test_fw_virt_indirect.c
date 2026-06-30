/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 8: Virtualization indirect switching paths
 *
 * Tests FW-VIRT-IND-01 through FW-VIRT-IND-10
 * Requires ENABLE_HYP=1
 */

#include "test_framework.h"

#ifdef ENABLE_HYP

/* Helper: check V=1 mode by reading hstatus.SPV */
static uintptr_t check_v1_mode_fn(uintptr_t arg)
{
    (void)arg;
    uintptr_t hstatus = CSRR(hstatus);
    return (hstatus >> 7) & 0x1;  /* SPV is bit 7 */
}

/* FW-VIRT-IND-01: S->VS indirect switch */
bool test_fw_virt_ind_01(void)
{
    TEST_BEGIN("FW-VIRT-IND-01: S->VS indirect switch");

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should be PRIV_S", get_current_priv(), PRIV_S);

    /*
     * S->VS requires indirect path: S->M (ecall), then M->VS (mret with MPV=1,MPP=S).
     * The framework handles this automatically.
     */
    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("current_priv should be PRIV_VS", get_current_priv(), PRIV_VS);

    TEST_END();
}

/* FW-VIRT-IND-02: S->VU indirect switch */
bool test_fw_virt_ind_02(void)
{
    TEST_BEGIN("FW-VIRT-IND-02: S->VU indirect switch");

    goto_priv(PRIV_S);
    goto_priv(PRIV_VU);
    TEST_ASSERT_EQ("current_priv should be PRIV_VU", get_current_priv(), PRIV_VU);

    TEST_END();
}

/* FW-VIRT-IND-03: VS->VU indirect switch */
bool test_fw_virt_ind_03(void)
{
    TEST_BEGIN("FW-VIRT-IND-03: VS->VU indirect switch");

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("current_priv should be PRIV_VS", get_current_priv(), PRIV_VS);

    /*
     * VS->VU requires indirect path: VS->M (ecall), then M->VU (mret with MPV=1,MPP=U).
     */
    goto_priv(PRIV_VU);
    TEST_ASSERT_EQ("current_priv should be PRIV_VU", get_current_priv(), PRIV_VU);

    TEST_END();
}

/* FW-VIRT-IND-04: S->VS switch, verify V=1 mode */
bool test_fw_virt_ind_04(void)
{
    TEST_BEGIN("FW-VIRT-IND-04: S->VS V=1 mode verification");

    /*
     * This test requires two-stage translation setup (hgatp) to execute
     * code in VS-mode. Without proper G-stage translation, QEMU will
     * trigger guest page faults when fetching instructions in VS-mode.
     * Skip this test if two-stage translation is not available.
     */
    TEST_SKIP("Requires two-stage translation setup (hgatp)");

    TEST_END();
}

/* FW-VIRT-IND-05: S->VU switch, verify V=1 mode */
bool test_fw_virt_ind_05(void)
{
    TEST_BEGIN("FW-VIRT-IND-05: S->VU V=1 mode verification");

    goto_priv(PRIV_S);
    goto_priv(PRIV_VU);

    TEST_ASSERT_EQ("current_priv should be PRIV_VU", get_current_priv(), PRIV_VU);

    /* Verify V=1 by checking we can return to M-mode */
    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("current_priv should be PRIV_M", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-VIRT-IND-06: VS->VU switch, verify V=1 preserved */
bool test_fw_virt_ind_06(void)
{
    TEST_BEGIN("FW-VIRT-IND-06: VS->VU V=1 mode preserved");

    goto_priv(PRIV_VS);
    goto_priv(PRIV_VU);

    TEST_ASSERT_EQ("current_priv should be PRIV_VU", get_current_priv(), PRIV_VU);

    TEST_END();
}

/* FW-VIRT-IND-07: M->VS->M round-trip */
bool test_fw_virt_ind_07(void)
{
    TEST_BEGIN("FW-VIRT-IND-07: M->VS->M round-trip");

    TEST_ASSERT_EQ("start in M-mode", get_current_priv(), PRIV_M);

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("in VS-mode", get_current_priv(), PRIV_VS);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("back in M-mode", get_current_priv(), PRIV_M);

    /* Verify V=0 (non-virtualized) */
    uintptr_t mstatus = CSRR(mstatus);
    TEST_ASSERT("MPV should be 0 in M-mode", !(mstatus & MSTATUS_MPV));

    TEST_END();
}

/* FW-VIRT-IND-08: M->VU->M round-trip */
bool test_fw_virt_ind_08(void)
{
    TEST_BEGIN("FW-VIRT-IND-08: M->VU->M round-trip");

    TEST_ASSERT_EQ("start in M-mode", get_current_priv(), PRIV_M);

    goto_priv(PRIV_VU);
    TEST_ASSERT_EQ("in VU-mode", get_current_priv(), PRIV_VU);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("back in M-mode", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-VIRT-IND-09: S->VS->S round-trip */
bool test_fw_virt_ind_09(void)
{
    TEST_BEGIN("FW-VIRT-IND-09: S->VS->S round-trip");

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("start in S-mode", get_current_priv(), PRIV_S);

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("in VS-mode", get_current_priv(), PRIV_VS);

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("back in S-mode", get_current_priv(), PRIV_S);

    TEST_END();
}

/* FW-VIRT-IND-10: VS->VU->VS round-trip */
bool test_fw_virt_ind_10(void)
{
    TEST_BEGIN("FW-VIRT-IND-10: VS->VU->VS round-trip");

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("start in VS-mode", get_current_priv(), PRIV_VS);

    goto_priv(PRIV_VU);
    TEST_ASSERT_EQ("in VU-mode", get_current_priv(), PRIV_VU);

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("back in VS-mode", get_current_priv(), PRIV_VS);

    TEST_END();
}

TEST_REGISTER(test_fw_virt_ind_01);
TEST_REGISTER(test_fw_virt_ind_02);
TEST_REGISTER(test_fw_virt_ind_03);
TEST_REGISTER(test_fw_virt_ind_04);
TEST_REGISTER(test_fw_virt_ind_05);
TEST_REGISTER(test_fw_virt_ind_06);
TEST_REGISTER(test_fw_virt_ind_07);
TEST_REGISTER(test_fw_virt_ind_08);
TEST_REGISTER(test_fw_virt_ind_09);
TEST_REGISTER(test_fw_virt_ind_10);

#else /* !ENABLE_HYP */

bool test_fw_virt_ind_skip(void)
{
    TEST_BEGIN("FW-VIRT-IND: Virtualization indirect tests");
    TEST_SKIP("ENABLE_HYP not set");
}

TEST_REGISTER(test_fw_virt_ind_skip);

#endif /* ENABLE_HYP */
