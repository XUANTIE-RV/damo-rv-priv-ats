/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 4: Round-trip privilege switching
 *
 * Tests FW-RT-01 through FW-RT-06
 */

#include "test_framework.h"

/* FW-RT-01: M->S->M round-trip */
bool test_fw_rt_01(void)
{
    TEST_BEGIN("FW-RT-01: M->S->M round-trip");

    TEST_ASSERT_EQ("start in M-mode", get_current_priv(), PRIV_M);

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("in S-mode", get_current_priv(), PRIV_S);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("back in M-mode", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-RT-02: M->U->M round-trip */
bool test_fw_rt_02(void)
{
    TEST_BEGIN("FW-RT-02: M->U->M round-trip");

    TEST_ASSERT_EQ("start in M-mode", get_current_priv(), PRIV_M);

    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("in U-mode", get_current_priv(), PRIV_U);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("back in M-mode", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-RT-03: M->S->U->M three-level round-trip */
bool test_fw_rt_03(void)
{
    TEST_BEGIN("FW-RT-03: M->S->U->M three-level round-trip");

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("in S-mode", get_current_priv(), PRIV_S);

    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("in U-mode", get_current_priv(), PRIV_U);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("back in M-mode", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-RT-04: M->U->S->M cross-level round-trip */
bool test_fw_rt_04(void)
{
    TEST_BEGIN("FW-RT-04: M->U->S->M cross-level round-trip");

    /*
     * M->U: mret (downward)
     * U->S: ecall to M-mode handler, which sets MPP=S and mrets to S
     * S->M: ecall to M-mode handler, which sets MPP=M and mrets to M
     */
    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("in U-mode", get_current_priv(), PRIV_U);

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("in S-mode", get_current_priv(), PRIV_S);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("back in M-mode", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-RT-05: M->S->U->S->M multiple round-trip */
bool test_fw_rt_05(void)
{
    TEST_BEGIN("FW-RT-05: M->S->U->S->M multiple round-trip");

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("in S-mode", get_current_priv(), PRIV_S);

    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("in U-mode", get_current_priv(), PRIV_U);

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("back in S-mode", get_current_priv(), PRIV_S);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("final in M-mode", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-RT-06: Round-trip mstatus integrity */
bool test_fw_rt_06(void)
{
    TEST_BEGIN("FW-RT-06: Round-trip mstatus integrity");

    /*
     * Per RISC-V Privileged Spec:
     * - mret sets MIE = MPIE, MPIE = 1
     * - Trap entry sets MPIE = MIE, MIE = 0
     *
     * So MIE is NOT preserved by hardware across mret. To test that
     * the framework preserves explicitly-set interrupt enable bits,
     * we set MIE=1 before the round-trip (matching FW-CSR-01 approach).
     */

    /* Set MIE=1 and SIE=1 explicitly */
    uintptr_t mstatus = CSRR(mstatus);
    mstatus |= MSTATUS_MIE_BIT | MSTATUS_SIE_BIT;
    CSRW(mstatus, mstatus);

    /* Save initial mstatus bits */
    uintptr_t mstatus_before = CSRR(mstatus);
    uintptr_t mie_before = mstatus_before & MSTATUS_MIE_BIT;
    uintptr_t sie_before = mstatus_before & MSTATUS_SIE_BIT;
    uintptr_t mprv_before = mstatus_before & MSTATUS_MPRV_BIT;

    /* Perform round-trip */
    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

    /* Check mstatus bits are preserved */
    uintptr_t mstatus_after = CSRR(mstatus);
    uintptr_t mie_after = mstatus_after & MSTATUS_MIE_BIT;
    uintptr_t sie_after = mstatus_after & MSTATUS_SIE_BIT;
    uintptr_t mprv_after = mstatus_after & MSTATUS_MPRV_BIT;

    TEST_ASSERT_EQ("MIE preserved", mie_after, mie_before);
    TEST_ASSERT_EQ("SIE preserved", sie_after, sie_before);
    TEST_ASSERT_EQ("MPRV preserved", mprv_after, mprv_before);

    /* Clean up: clear MIE and SIE */
    mstatus_after &= ~(MSTATUS_MIE_BIT | MSTATUS_SIE_BIT);
    CSRW(mstatus, mstatus_after);

    TEST_END();
}

/* Register all tests */
TEST_REGISTER(test_fw_rt_01);
TEST_REGISTER(test_fw_rt_02);
TEST_REGISTER(test_fw_rt_03);
TEST_REGISTER(test_fw_rt_04);
TEST_REGISTER(test_fw_rt_05);
TEST_REGISTER(test_fw_rt_06);
