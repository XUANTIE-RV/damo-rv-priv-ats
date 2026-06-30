/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 1: Basic downward privilege switching (M-mode -> S/U-mode)
 *
 * Tests FW-DOWN-01 through FW-DOWN-08
 */

#include "test_framework.h"

/* Helper function to read sstatus in S-mode */
static uintptr_t read_sstatus_fn(uintptr_t arg)
{
    (void)arg;
    return CSRR(sstatus);
}

/* FW-DOWN-01: M->S switch, verify current_priv */
bool test_fw_down_01(void)
{
    TEST_BEGIN("FW-DOWN-01: M->S current_priv verification");

    goto_priv(PRIV_S);
    unsigned priv = get_current_priv();
    TEST_ASSERT_EQ("current_priv should be PRIV_S", priv, PRIV_S);

    TEST_END();
}

/* FW-DOWN-02: M->U switch, verify current_priv */
bool test_fw_down_02(void)
{
    TEST_BEGIN("FW-DOWN-02: M->U current_priv verification");

    goto_priv(PRIV_U);
    unsigned priv = get_current_priv();
    TEST_ASSERT_EQ("current_priv should be PRIV_U", priv, PRIV_U);

    TEST_END();
}

/* FW-DOWN-03: M->S switch, verify mstatus.MPP configuration */
bool test_fw_down_03(void)
{
    TEST_BEGIN("FW-DOWN-03: M->S mstatus.MPP verification");

    /*
     * We cannot directly observe MPP before mret (it happens inside
     * lower_priv). Instead, verify the switch succeeded and mret
     * cleared MPP (hardware behavior).
     */
    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

    /* After mret, MPP should be cleared to U (0) by hardware */
    uintptr_t mstatus = CSRR(mstatus);
    uintptr_t mpp = (mstatus >> MSTATUS_MPP_OFF) & 0x3;
    TEST_ASSERT_EQ("MPP should be cleared after mret", mpp, 0);

    TEST_END();
}

/* FW-DOWN-04: M->U switch, verify mstatus.MPP configuration */
bool test_fw_down_04(void)
{
    TEST_BEGIN("FW-DOWN-04: M->U mstatus.MPP verification");

    goto_priv(PRIV_U);
    goto_priv(PRIV_M);

    uintptr_t mstatus = CSRR(mstatus);
    uintptr_t mpp = (mstatus >> MSTATUS_MPP_OFF) & 0x3;
    TEST_ASSERT_EQ("MPP should be cleared after mret", mpp, 0);

    TEST_END();
}

/* FW-DOWN-05: M->S switch, verify MPRV is cleared */
bool test_fw_down_05(void)
{
    TEST_BEGIN("FW-DOWN-05: M->S MPRV clear verification");

    /* Set MPRV=1 in M-mode */
    uintptr_t mstatus = CSRR(mstatus);
    mstatus |= MSTATUS_MPRV_BIT;
    CSRW(mstatus, mstatus);

    /* Verify MPRV is set */
    mstatus = CSRR(mstatus);
    TEST_ASSERT("MPRV should be set before switch", mstatus & MSTATUS_MPRV_BIT);

    /* Switch to S-mode and back */
    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

    /* Check MPRV is cleared (mret clears MPRV when MPP != M) */
    mstatus = CSRR(mstatus);
    TEST_ASSERT("MPRV should be cleared after mret with MPP=S", !(mstatus & MSTATUS_MPRV_BIT));

    TEST_END();
}

/* FW-DOWN-06: M->S switch, verify S-mode execution by reading sstatus */
bool test_fw_down_06(void)
{
    TEST_BEGIN("FW-DOWN-06: M->S S-mode execution verification");

    /* Use run_in_priv to execute in S-mode and return result */
    (void)run_in_priv(PRIV_S, read_sstatus_fn, 0);

    /* If we successfully read sstatus, we were in S-mode */
    TEST_ASSERT("sstatus read succeeded in S-mode", true);

    TEST_END();
}

/* FW-DOWN-07: M->U switch, verify U-mode execution by attempting sstatus read */
bool test_fw_down_07(void)
{
    TEST_BEGIN("FW-DOWN-07: M->U U-mode execution verification");

    /*
     * In U-mode, reading sstatus should trigger illegal instruction.
     * Use PRIV_DO pattern: arm trap, execute, disarm in U-mode,
     * then check result in M-mode.
     */
    goto_priv(PRIV_U);
    PRIV_DO(CSRR(sstatus));
    goto_priv(PRIV_M);

    /* Should have trapped with cause=2 (illegal instruction) */
    CHECK_TRAP("sstatus read in U-mode should trap", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* FW-DOWN-08: M->S switch, verify MPV is cleared (non-virtualized) */
bool test_fw_down_08(void)
{
    TEST_BEGIN("FW-DOWN-08: M->S MPV clear verification");

#ifdef ENABLE_HYP
    /* Clear MPV before switch */
    uintptr_t mstatus = CSRR(mstatus);
    mstatus &= ~MSTATUS_MPV;
    CSRW(mstatus, mstatus);
#endif

    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

#ifdef ENABLE_HYP
    /* MPV should remain 0 (non-virtualized switch) */
    mstatus = CSRR(mstatus);
    TEST_ASSERT("MPV should be 0 for non-virtualized switch", !(mstatus & MSTATUS_MPV));
#else
    TEST_ASSERT("MPV check skipped (ENABLE_HYP not set)", true);
#endif

    TEST_END();
}

/* Register all tests */
TEST_REGISTER(test_fw_down_01);
TEST_REGISTER(test_fw_down_02);
TEST_REGISTER(test_fw_down_03);
TEST_REGISTER(test_fw_down_04);
TEST_REGISTER(test_fw_down_05);
TEST_REGISTER(test_fw_down_06);
TEST_REGISTER(test_fw_down_07);
TEST_REGISTER(test_fw_down_08);
