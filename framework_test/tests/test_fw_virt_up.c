/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 7: Virtualization upward switching (VS/VU -> M/HS-mode)
 *
 * Tests FW-VIRT-UP-01 through FW-VIRT-UP-08
 * Requires ENABLE_HYP=1
 */

#include "test_framework.h"

#ifdef ENABLE_HYP

/* Returns 0 when hstatus could be read, otherwise the cause of the trap the
 * read took.
 *
 * hstatus is the discriminating access for this group: it succeeds in M/HS
 * and raises a virtual-instruction exception with V=1
 * (norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0), so a clean read proves the
 * switch under test really left virtualization. */
static uintptr_t read_hstatus_fn(uintptr_t arg)
{
    (void)arg;
    PRIV_DO(CSRR(hstatus));
    return trap_was_triggered() ? trap_get_cause() : 0u;
}

/* FW-VIRT-UP-01: VS->M switch, verify current_priv */
bool test_fw_virt_up_01(void)
{
    TEST_BEGIN("FW-VIRT-UP-01: VS->M current_priv verification");

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("current_priv should be PRIV_VS", get_current_priv(), PRIV_VS);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("current_priv should be PRIV_M", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-VIRT-UP-02: VU->M switch, verify current_priv */
bool test_fw_virt_up_02(void)
{
    TEST_BEGIN("FW-VIRT-UP-02: VU->M current_priv verification");

    goto_priv(PRIV_VU);
    TEST_ASSERT_EQ("current_priv should be PRIV_VU", get_current_priv(), PRIV_VU);

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("current_priv should be PRIV_M", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-VIRT-UP-03: VS->HS (PRIV_S) switch, verify current_priv */
bool test_fw_virt_up_03(void)
{
    TEST_BEGIN("FW-VIRT-UP-03: VS->HS current_priv verification");

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("current_priv should be PRIV_VS", get_current_priv(), PRIV_VS);

    /* VS->HS exits virtualization (V=1 -> V=0) */
    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should be PRIV_S", get_current_priv(), PRIV_S);

    TEST_END();
}

/* FW-VIRT-UP-04: VU->VS switch, verify current_priv */
bool test_fw_virt_up_04(void)
{
    TEST_BEGIN("FW-VIRT-UP-04: VU->VS current_priv verification");

    goto_priv(PRIV_VU);
    TEST_ASSERT_EQ("current_priv should be PRIV_VU", get_current_priv(), PRIV_VU);

    /* VU->VS keeps V=1, raises nominal priv to S */
    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("current_priv should be PRIV_VS", get_current_priv(), PRIV_VS);

    TEST_END();
}

/* FW-VIRT-UP-05: VS->M ecall path verification */
bool test_fw_virt_up_05(void)
{
    TEST_BEGIN("FW-VIRT-UP-05: VS->M ecall path verification");

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("current_priv should be PRIV_VS before ecall", get_current_priv(), PRIV_VS);

    /*
     * VS->M is an ecall-from-VS (cause=10).  hedeleg[10] is read-only-zero,
     * so the request stops at HS and is forwarded from there to M; the
     * handler raises MPP to M and mrets, which per norm:mstatus_mpv_op keeps
     * V at 0 because MPP=M.  Reading hstatus afterwards is the observable
     * half of that: it needs M/HS authority and no V=1.
     */
    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("current_priv should be PRIV_M after VS->M", get_current_priv(), PRIV_M);

    PRIV_DO(CSRR(hstatus));
    CHECK_NO_TRAP("hstatus read succeeded in M-mode");

    TEST_END();
}

/* FW-VIRT-UP-06: VS->M switch, verify M-mode H-ext CSR access */
bool test_fw_virt_up_06(void)
{
    TEST_BEGIN("FW-VIRT-UP-06: VS->M M-mode H-ext CSR access");

    goto_priv(PRIV_VS);
    goto_priv(PRIV_M);

    /* In M-mode (V=0), hstatus should be accessible */
    PRIV_DO(CSRR(hstatus));
    CHECK_NO_TRAP("hstatus read succeeded in M-mode");

    TEST_END();
}

/* FW-VIRT-UP-07: VS->HS switch, verify S-mode H-ext CSR access */
bool test_fw_virt_up_07(void)
{
    TEST_BEGIN("FW-VIRT-UP-07: VS->HS S-mode H-ext CSR access");

    goto_priv(PRIV_VS);
    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should be PRIV_S", get_current_priv(), PRIV_S);

    /* In HS-mode (V=0, S-mode), hstatus should be accessible.  The helper
     * reports the trap cause instead of the value so the assertion states
     * what was observed rather than assuming it. */
    uintptr_t fault = run_in_priv(PRIV_S, read_hstatus_fn, 0);
    TEST_ASSERT_EQ("hstatus read succeeded in HS-mode", fault, 0u);

    TEST_END();
}

/* FW-VIRT-UP-08: VU->VS switch, verify VS-mode CSR access */
bool test_fw_virt_up_08(void)
{
    TEST_BEGIN("FW-VIRT-UP-08: VU->VS VS-mode CSR access");

    goto_priv(PRIV_M);

    /* Clear the marker first, so what is observed below cannot be a leftover
     * from an earlier case. */
    uintptr_t vs = CSRR(vsstatus);
    CSRW(vsstatus, vs & ~MSTATUS_SUM_BIT);

    goto_priv(PRIV_VU);
    TEST_ASSERT_EQ("current_priv should be PRIV_VU", get_current_priv(), PRIV_VU);

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("current_priv should be PRIV_VS", get_current_priv(), PRIV_VS);

    /* With V=1 the sstatus address reaches vsstatus (norm:H_vscsrs_sub), so
     * setting SUM from VS-mode has to show up as vsstatus.SUM when read back
     * from M-mode.  That round trip is the evidence the VS-mode CSR access is
     * the virtualized one, and not a stray write to the real sstatus.
     *
     * CSRS is a statement, not an expression, so it cannot go through
     * PRIV_DO's (void)(stmt); arm the window by hand instead. */
    trap_expect_begin();
    CSRS(sstatus, MSTATUS_SUM_BIT);
    trap_expect_end();
    bool write_ok = !trap_was_triggered();

    goto_priv(PRIV_M);
    TEST_ASSERT("VS-mode write through the sstatus address did not trap", write_ok);
    TEST_ASSERT("the write landed in vsstatus", CSRR(vsstatus) & MSTATUS_SUM_BIT);

    /* Leave the marker cleared for whatever runs next. */
    vs = CSRR(vsstatus);
    CSRW(vsstatus, vs & ~MSTATUS_SUM_BIT);

    TEST_END();
}

TEST_REGISTER(test_fw_virt_up_01);
TEST_REGISTER(test_fw_virt_up_02);
TEST_REGISTER(test_fw_virt_up_03);
TEST_REGISTER(test_fw_virt_up_04);
TEST_REGISTER(test_fw_virt_up_05);
TEST_REGISTER(test_fw_virt_up_06);
TEST_REGISTER(test_fw_virt_up_07);
TEST_REGISTER(test_fw_virt_up_08);

#else /* !ENABLE_HYP */

bool test_fw_virt_up_skip(void)
{
    TEST_BEGIN("FW-VIRT-UP: Virtualization tests");
    TEST_SKIP("ENABLE_HYP not set");
}

TEST_REGISTER(test_fw_virt_up_skip);

#endif /* ENABLE_HYP */
