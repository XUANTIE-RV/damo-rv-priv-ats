/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 1: Basic downward privilege switching (M-mode -> S/U-mode)
 *
 * Tests FW-DOWN-01 through FW-DOWN-08
 */

#include "test_framework.h"

/* Probe the privilege the trampoline actually landed in.
 *
 * Returns a bitmask: bit0 = sstatus readable, bit1 = mstatus denied.  Both
 * halves are needed: sstatus alone would also read fine in M-mode, and
 * mstatus being denied alone would also hold in U-mode.  Only the pair pins
 * the execution to S.  Each read is trap-protected so a denial is an
 * observation rather than an UNEXPECTED TRAP halt. */
#define FW_S_PROBE_SSTATUS_OK    0x1u
#define FW_S_PROBE_MSTATUS_DENY  0x2u

static uintptr_t s_mode_probe_fn(uintptr_t arg)
{
    (void)arg;
    PRIV_DO(CSRR(sstatus));
    unsigned r = trap_was_triggered() ? 0u : FW_S_PROBE_SSTATUS_OK;
    PRIV_DO(CSRR(mstatus));
    if (trap_was_triggered())
        r |= FW_S_PROBE_MSTATUS_DENY;
    return (uintptr_t)r;
}

/* FW-DOWN-01: M->S switch, verify current_priv */
bool test_fw_down_01(void)
{
    TEST_BEGIN("FW-DOWN-01: M->S current_priv verification");

    /* main() runs in S-mode, so every case in this file establishes the
     * M start its name declares.  Without it goto_priv(PRIV_S) below is a
     * no-op and the M->S descent under test never happens. */
    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("raised to M before descending", get_current_priv(), PRIV_M);

    goto_priv(PRIV_S);
    unsigned priv = get_current_priv();
    TEST_ASSERT_EQ("current_priv should be PRIV_S", priv, PRIV_S);

    TEST_END();
}

/* FW-DOWN-02: M->U switch, verify current_priv */
bool test_fw_down_02(void)
{
    TEST_BEGIN("FW-DOWN-02: M->U current_priv verification");

    goto_priv(PRIV_M);

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
    goto_priv(PRIV_M);

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

    goto_priv(PRIV_M);

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

    /* The body has to hold M-mode authority for real, not borrow it through
     * the T-SBI CSR proxy: MPRV is exactly what an mret with MPP != M clears,
     * so a proxied write would set the bit and the proxy's own return leg
     * would clear it again before we could observe it. */
    goto_priv(PRIV_M);

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

    /* Use run_in_priv to execute in S-mode and return result.  Raise first:
     * from an S-mode caller this would be a same-privilege direct call, not
     * the M->S descent the case is named for. */
    goto_priv(PRIV_M);
    uintptr_t probe = run_in_priv(PRIV_S, s_mode_probe_fn, 0);

    /* The probe only reports 3 when it ran with exactly S authority. */
    TEST_ASSERT_EQ("fn ran in S-mode: sstatus readable, mstatus denied",
                   probe, FW_S_PROBE_SSTATUS_OK | FW_S_PROBE_MSTATUS_DENY);

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
    goto_priv(PRIV_M);

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

    /* Same reason as FW-DOWN-05: mret clears MPV, so this one cannot be
     * staged through the CSR proxy either. */
    goto_priv(PRIV_M);

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
