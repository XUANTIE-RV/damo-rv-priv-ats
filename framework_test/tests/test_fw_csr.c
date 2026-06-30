/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 10: CSR state integrity during privilege switching
 *
 * Tests FW-CSR-01 through FW-CSR-08
 */

#include "test_framework.h"

/* FW-CSR-01: MIE preserved across M->S->M */
bool test_fw_csr_01(void)
{
    TEST_BEGIN("FW-CSR-01: MIE preserved across M->S->M");

    /* Set MIE=1 */
    uintptr_t mstatus = CSRR(mstatus);
    mstatus |= MSTATUS_MIE_BIT;
    CSRW(mstatus, mstatus);

    /* Round-trip */
    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

    /* Verify MIE still set */
    mstatus = CSRR(mstatus);
    TEST_ASSERT("MIE should be preserved", mstatus & MSTATUS_MIE_BIT);

    /* Clean up: clear MIE */
    mstatus &= ~MSTATUS_MIE_BIT;
    CSRW(mstatus, mstatus);

    TEST_END();
}

/* FW-CSR-02: SIE preserved across M->S->M */
bool test_fw_csr_02(void)
{
    TEST_BEGIN("FW-CSR-02: SIE preserved across M->S->M");

    /* Set SIE=1 */
    uintptr_t mstatus = CSRR(mstatus);
    mstatus |= MSTATUS_SIE_BIT;
    CSRW(mstatus, mstatus);

    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

    mstatus = CSRR(mstatus);
    TEST_ASSERT("SIE should be preserved", mstatus & MSTATUS_SIE_BIT);

    /* Clean up */
    mstatus &= ~MSTATUS_SIE_BIT;
    CSRW(mstatus, mstatus);

    TEST_END();
}

/* FW-CSR-03: ecall handler only modifies MPP/MPV/MPRV */
bool test_fw_csr_03(void)
{
    TEST_BEGIN("FW-CSR-03: ecall handler only modifies MPP/MPV/MPRV");

    /*
     * Set some bits in mstatus that are NOT MPP/MPV/MPRV.
     * The ecall handler should preserve them.
     * MPP = bits [12:11], MPRV = bit 17, MPV = bit 39
     */
    uintptr_t mstatus = CSRR(mstatus);

    /* Set MPIE (bit 7) and SPIE (bit 5) as test bits */
    mstatus |= (1UL << 7) | (1UL << 5);
    CSRW(mstatus, mstatus);

    uintptr_t before = CSRR(mstatus);

    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

    uintptr_t after = CSRR(mstatus);

    /* Check MPIE and SPIE preserved */
    TEST_ASSERT_BITS("MPIE preserved", after, (1UL << 7), before);
    TEST_ASSERT_BITS("SPIE preserved", after, (1UL << 5), before);

    /* Clean up */
    after &= ~((1UL << 7) | (1UL << 5));
    CSRW(mstatus, after);

    TEST_END();
}

/* FW-CSR-04: MPP cleared to U after M->S mret */
bool test_fw_csr_04(void)
{
    TEST_BEGIN("FW-CSR-04: MPP cleared after M->S mret");

    /*
     * After mret with MPP=S, hardware sets MPP to the lowest
     * implemented privilege (U=0).
     */
    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

    uintptr_t mstatus = CSRR(mstatus);
    uintptr_t mpp = (mstatus >> MSTATUS_MPP_OFF) & 0x3;
    TEST_ASSERT_EQ("MPP should be U(0) after mret", mpp, PRIV_U);

    TEST_END();
}

/* FW-CSR-05: MPP cleared to U after M->U mret */
bool test_fw_csr_05(void)
{
    TEST_BEGIN("FW-CSR-05: MPP cleared after M->U mret");

    goto_priv(PRIV_U);
    goto_priv(PRIV_M);

    uintptr_t mstatus = CSRR(mstatus);
    uintptr_t mpp = (mstatus >> MSTATUS_MPP_OFF) & 0x3;
    TEST_ASSERT_EQ("MPP should be U(0) after mret", mpp, PRIV_U);

    TEST_END();
}

/* FW-CSR-06: SPP cleared to U after S->U sret */
bool test_fw_csr_06(void)
{
    TEST_BEGIN("FW-CSR-06: SPP cleared after S->U sret");

    /*
     * After sret with SPP=U, hardware sets SPP to U(0).
     * We need to check sstatus.SPP after returning to M-mode.
     */
    goto_priv(PRIV_S);
    goto_priv(PRIV_U);
    goto_priv(PRIV_M);

    /* Read sstatus (which is a view of mstatus.SD and some fields) */
    uintptr_t sstatus = CSRR(sstatus);
    uintptr_t spp = (sstatus >> 8) & 0x1;
    TEST_ASSERT_EQ("SPP should be U(0) after sret", spp, PRIV_U);

    TEST_END();
}

/* FW-CSR-07: mtvec preserved across M->S->M */
bool test_fw_csr_07(void)
{
    TEST_BEGIN("FW-CSR-07: mtvec preserved across M->S->M");

    uintptr_t mtvec_before = CSRR(mtvec);

    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

    uintptr_t mtvec_after = CSRR(mtvec);
    TEST_ASSERT_EQ("mtvec should be preserved", mtvec_after, mtvec_before);

    TEST_END();
}

/* FW-CSR-08: stvec preserved across M->S->M */
bool test_fw_csr_08(void)
{
    TEST_BEGIN("FW-CSR-08: stvec preserved across M->S->M");

    /* Write a test value to stvec (must be 4-byte aligned) */
    uintptr_t test_stvec = 0x80001000UL;  /* arbitrary aligned address */
    uintptr_t stvec_before = CSRR(stvec);
    CSRW(stvec, test_stvec);

    goto_priv(PRIV_S);
    goto_priv(PRIV_M);

    uintptr_t stvec_after = CSRR(stvec);
    TEST_ASSERT_EQ("stvec should be preserved", stvec_after, test_stvec);

    /* Restore original stvec */
    CSRW(stvec, stvec_before);

    TEST_END();
}

/* Register all tests */
TEST_REGISTER(test_fw_csr_01);
TEST_REGISTER(test_fw_csr_02);
TEST_REGISTER(test_fw_csr_03);
TEST_REGISTER(test_fw_csr_04);
TEST_REGISTER(test_fw_csr_05);
TEST_REGISTER(test_fw_csr_06);
TEST_REGISTER(test_fw_csr_07);
TEST_REGISTER(test_fw_csr_08);
