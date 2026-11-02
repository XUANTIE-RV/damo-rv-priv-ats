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

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should be PRIV_S", get_current_priv(), PRIV_S);

    /* Control: hstatus is an HS-level CSR, so HS-mode reads it cleanly. */
    PRIV_DO(CSRR(hstatus));
    bool hs_ok = !trap_was_triggered();

    /*
     * S->VS requires the indirect path: S->M (ecall), then M->VS (mret with
     * MPV=1, MPP=S).  The framework handles this automatically.
     */
    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("current_priv should be PRIV_VS", get_current_priv(), PRIV_VS);

    /* Nothing about the hart changed between the two reads except V, so the
     * access being rejected now - with a virtual-instruction rather than an
     * illegal-instruction exception - is the V=1 evidence
     * (norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0). */
    PRIV_DO(CSRR(hstatus));
    bool vs_trapped = trap_was_triggered();
    uintptr_t vs_cause = trap_get_cause();

    goto_priv(PRIV_M);
    TEST_ASSERT("HS-mode read hstatus", hs_ok);
    TEST_ASSERT("VS-mode read of hstatus trapped", vs_trapped);
    TEST_ASSERT_EQ("cause is virtual-instruction", vs_cause,
                   (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

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

    /* main() runs in S-mode; establish the M start this case documents. */
    goto_priv(PRIV_M);
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

    goto_priv(PRIV_M);
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
