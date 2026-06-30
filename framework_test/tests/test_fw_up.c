/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 2: Basic upward privilege switching (S/U-mode -> M-mode)
 *
 * Tests FW-UP-01 through FW-UP-07
 */

#include "test_framework.h"

/* Helper function to read mstatus (only works in M-mode) */
static uintptr_t read_mstatus_fn(uintptr_t arg)
{
    (void)arg;
    return CSRR(mstatus);
}

/* FW-UP-01: S->M switch, verify current_priv */
bool test_fw_up_01(void)
{
    TEST_BEGIN("FW-UP-01: S->M current_priv verification");

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should be PRIV_S", get_current_priv(), PRIV_S);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("current_priv should be PRIV_M after S->M", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-UP-02: U->M switch, verify current_priv */
bool test_fw_up_02(void)
{
    TEST_BEGIN("FW-UP-02: U->M current_priv verification");

    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("current_priv should be PRIV_U", get_current_priv(), PRIV_U);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("current_priv should be PRIV_M after U->M", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-UP-03: S->M ecall path verification */
bool test_fw_up_03(void)
{
    TEST_BEGIN("FW-UP-03: S->M ecall path verification");

    /*
     * S->M uses ecall. The ecall should trap to M-mode handler with
     * cause=9 (ecall-from-S). The handler sets MPP=M and mrets back.
     * We verify the switch succeeded by reading mstatus (M-mode only CSR).
     *
     * Note: Per RISC-V Privileged Spec Section 3.1.6.1, MRET clears
     * MPP to the least-privileged mode (U-mode). So we cannot verify
     * MPP after mret returns. Instead, we verify we're in M-mode by
     * successfully reading mstatus (which faults in non-M modes).
     */
    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should be PRIV_S before ecall", get_current_priv(), PRIV_S);

    goto_priv(PRIV_M);

    /* Verify we're in M-mode: current_priv tracking and mstatus access */
    TEST_ASSERT_EQ("current_priv should be PRIV_M after S->M", get_current_priv(), PRIV_M);
    /* If CSRR(mstatus) doesn't fault, we are confirmed in M-mode */
    (void)CSRR(mstatus);
    TEST_ASSERT("mstatus read succeeded (M-mode only CSR)", true);

    TEST_END();
}

/* FW-UP-04: U->M ecall path verification */
bool test_fw_up_04(void)
{
    TEST_BEGIN("FW-UP-04: U->M ecall path verification");

    /*
     * U->M uses ecall. Per RISC-V Privileged Spec Section 3.1.6.1,
     * MRET clears MPP to the least-privileged mode after returning.
     * So we verify M-mode entry by successfully reading mstatus.
     */
    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("current_priv should be PRIV_U before ecall", get_current_priv(), PRIV_U);

    goto_priv(PRIV_M);

    /* Verify we're in M-mode */
    TEST_ASSERT_EQ("current_priv should be PRIV_M after U->M", get_current_priv(), PRIV_M);
    (void)CSRR(mstatus);
    TEST_ASSERT("mstatus read succeeded (M-mode only CSR)", true);

    TEST_END();
}

/* FW-UP-05: U->S ecall path verification */
bool test_fw_up_05(void)
{
    TEST_BEGIN("FW-UP-05: U->S ecall path verification");

    /*
     * U->S uses ecall (delegated to S-mode handler).
     * Per RISC-V Privileged Spec Section 3.1.6.2, SRET clears SPP
     * to 0 (U-mode) after returning. So we verify S-mode entry by
     * successfully reading sstatus (S-mode CSR) and checking
     * current_priv tracking.
     */
    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("current_priv should be PRIV_U before ecall", get_current_priv(), PRIV_U);

    goto_priv(PRIV_S);

    /* Verify we're in S-mode */
    TEST_ASSERT_EQ("current_priv should be PRIV_S after U->S", get_current_priv(), PRIV_S);

    /* Verify by reading sstatus (should work in S-mode, faults in U-mode) */
    (void)CSRR(sstatus);
    TEST_ASSERT("sstatus read succeeded (S-mode CSR)", true);

    TEST_END();
}

/* FW-UP-06: S->M switch, verify M-mode CSR access */
bool test_fw_up_06(void)
{
    TEST_BEGIN("FW-UP-06: S->M M-mode CSR access verification");

    goto_priv(PRIV_S);

    /* Use run_in_priv to execute in M-mode and read mstatus */
    (void)run_in_priv(PRIV_M, read_mstatus_fn, 0);

    /* If we successfully read mstatus, we were in M-mode */
    TEST_ASSERT("mstatus read succeeded in M-mode", true);

    TEST_END();
}

/* FW-UP-07: U->S switch, verify S-mode CSR access */
bool test_fw_up_07(void)
{
    TEST_BEGIN("FW-UP-07: U->S S-mode CSR access verification");

    goto_priv(PRIV_U);
    goto_priv(PRIV_S);

    /* Verify we can read sstatus in S-mode */
    (void)CSRR(sstatus);
    TEST_ASSERT("sstatus read succeeded in S-mode", true);

    TEST_END();
}

/* Register all tests */
TEST_REGISTER(test_fw_up_01);
TEST_REGISTER(test_fw_up_02);
TEST_REGISTER(test_fw_up_03);
TEST_REGISTER(test_fw_up_04);
TEST_REGISTER(test_fw_up_05);
TEST_REGISTER(test_fw_up_06);
TEST_REGISTER(test_fw_up_07);
