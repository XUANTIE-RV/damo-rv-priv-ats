/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 2: Basic upward privilege switching (S/U-mode -> M-mode)
 *
 * Tests FW-UP-01 through FW-UP-07
 *
 * "The CSR read succeeded" is never asserted as a bare `true`: an ungated
 * read that faults would halt the suite as an UNEXPECTED TRAP instead of
 * failing the case, so every read here is trap-protected and the assertion
 * states the observation.  Where the case has to prove WHICH level was
 * reached, the probe reads one CSR that level may access and one it may not.
 */

#include "test_framework.h"

/* Returns 1 when mstatus could be read, 0 when the read trapped.
 * mstatus is M-only, so a clean read is sufficient evidence of M-mode. */
static uintptr_t m_mode_probe_fn(uintptr_t arg)
{
    (void)arg;
    PRIV_DO(CSRR(mstatus));
    return trap_was_triggered() ? 0u : 1u;
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
     * S->M is a T-SBI GOTO_MMODE request: the ecall traps with cause 9,
     * medeleg[9] is held at 0 so it reaches the M handler, which raises MPP
     * to M and mrets past the ecall.
     *
     * Per RISC-V Privileged Spec mret clears MPP to the least-privileged
     * mode, so MPP cannot be inspected afterwards; the observable half is
     * that an M-only CSR is now reachable.
     */
    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should be PRIV_S before ecall", get_current_priv(), PRIV_S);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("current_priv should be PRIV_M after S->M", get_current_priv(), PRIV_M);

    PRIV_DO(CSRR(mstatus));
    CHECK_NO_TRAP("mstatus read succeeded (M-mode only CSR)");

    TEST_END();
}

/* FW-UP-04: U->M ecall path verification */
bool test_fw_up_04(void)
{
    TEST_BEGIN("FW-UP-04: U->M ecall path verification");

    /*
     * U->M is a T-SBI GOTO_MMODE request too.  The ecall traps with cause 8,
     * which medeleg[8] hands to the S handler; S cannot raise privilege
     * itself, so it forwards the request up to M (see tsbi_design_brief.md).
     * MPP is already cleared again by the time the body runs, so the check is
     * the same observable one as FW-UP-03.
     */
    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("current_priv should be PRIV_U before ecall", get_current_priv(), PRIV_U);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("current_priv should be PRIV_M after U->M", get_current_priv(), PRIV_M);

    PRIV_DO(CSRR(mstatus));
    CHECK_NO_TRAP("mstatus read succeeded (M-mode only CSR)");

    TEST_END();
}

/* FW-UP-05: U->S ecall path verification */
bool test_fw_up_05(void)
{
    TEST_BEGIN("FW-UP-05: U->S ecall path verification");

    /*
     * U->S is a T-SBI GOTO_SMODE request.  Per the spec SRET clears SPP to 0
     * after returning, so the path is verified by where the body ends up
     * rather than by a status field: sstatus must be reachable and mstatus
     * must not be, which is exactly the S-mode authority split.
     */
    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("current_priv should be PRIV_U before ecall", get_current_priv(), PRIV_U);

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should be PRIV_S after U->S", get_current_priv(), PRIV_S);

    PRIV_DO(CSRR(sstatus));
    bool sstatus_ok = !trap_was_triggered();
    PRIV_DO(CSRR(mstatus));
    bool mstatus_denied = trap_was_triggered();

    TEST_ASSERT("sstatus read succeeded (S-mode CSR)", sstatus_ok);
    TEST_ASSERT("mstatus denied: the switch landed in S, not M", mstatus_denied);

    TEST_END();
}

/* FW-UP-06: S->M switch, verify M-mode CSR access */
bool test_fw_up_06(void)
{
    TEST_BEGIN("FW-UP-06: S->M M-mode CSR access verification");

    goto_priv(PRIV_S);

    /* Use run_in_priv to execute in M-mode and read mstatus */
    uintptr_t ok = run_in_priv(PRIV_M, m_mode_probe_fn, 0);
    TEST_ASSERT_EQ("mstatus read succeeded in M-mode", ok, 1u);

    TEST_END();
}

/* FW-UP-07: U->S switch, verify S-mode CSR access */
bool test_fw_up_07(void)
{
    TEST_BEGIN("FW-UP-07: U->S S-mode CSR access verification");

    /* Reference readings taken with M authority.  stvec is a plain S-level
     * CSR, so the value an S-mode read returns has to be this one - a read
     * that was emulated, redirected or fabricated would not match. */
    goto_priv(PRIV_M);
    uintptr_t ref_stvec = CSRR(stvec);

    goto_priv(PRIV_U);
    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should be PRIV_S after U->S", get_current_priv(), PRIV_S);

    PRIV_DO(CSRR(sstatus));
    bool sstatus_ok = !trap_was_triggered();
    PRIV_DO(CSRR(stvec));
    bool stvec_ok = !trap_was_triggered();
    uintptr_t got_stvec = CSRR(stvec);

    TEST_ASSERT("sstatus read succeeded in S-mode", sstatus_ok);
    TEST_ASSERT("stvec read succeeded in S-mode", stvec_ok);
    TEST_ASSERT_EQ("stvec read in S-mode matches the M-mode reference",
                   got_stvec, ref_stvec);

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
