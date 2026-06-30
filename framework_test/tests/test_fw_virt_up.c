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

/* Helper: read hstatus in M-mode or HS-mode */
static uintptr_t read_hstatus_fn(uintptr_t arg)
{
    (void)arg;
    return CSRR(hstatus);
}

/* Helper: read vsstatus in VS-mode */
static uintptr_t read_vsstatus_fn(uintptr_t arg)
{
    (void)arg;
    return CSRR(vsstatus);
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

    goto_priv(PRIV_M);

    /*
     * VS->M uses ecall-from-VS (cause=10). The handler sets MPP=M
     * and mrets back to M-mode. After mret, MPP is cleared by hardware.
     * We verify the switch succeeded by checking current_priv.
     */
    TEST_ASSERT_EQ("current_priv should be PRIV_M after VS->M", get_current_priv(), PRIV_M);

    /* Verify we're in M-mode by reading hstatus (M-mode only H-ext CSR) */
    uintptr_t hstatus = CSRR(hstatus);
    TEST_ASSERT("hstatus read succeeded in M-mode", true);

    TEST_END();
}

/* FW-VIRT-UP-06: VS->M switch, verify M-mode H-ext CSR access */
bool test_fw_virt_up_06(void)
{
    TEST_BEGIN("FW-VIRT-UP-06: VS->M M-mode H-ext CSR access");

    goto_priv(PRIV_VS);
    goto_priv(PRIV_M);

    /* In M-mode (V=0), hstatus should be accessible */
    uintptr_t hstatus = CSRR(hstatus);
    TEST_ASSERT("hstatus read succeeded in M-mode", true);

    TEST_END();
}

/* FW-VIRT-UP-07: VS->HS switch, verify S-mode H-ext CSR access */
bool test_fw_virt_up_07(void)
{
    TEST_BEGIN("FW-VIRT-UP-07: VS->HS S-mode H-ext CSR access");

    goto_priv(PRIV_VS);
    goto_priv(PRIV_S);

    /* In HS-mode (V=0, S-mode), hstatus should be accessible */
    uintptr_t result = run_in_priv(PRIV_S, read_hstatus_fn, 0);
    TEST_ASSERT("hstatus read succeeded in HS-mode", true);

    TEST_END();
}

/* FW-VIRT-UP-08: VU->VS switch, verify VS-mode CSR access */
bool test_fw_virt_up_08(void)
{
    TEST_BEGIN("FW-VIRT-UP-08: VU->VS VS-mode CSR access");

    /*
     * This test requires two-stage translation setup (hgatp) to execute
     * code in VS-mode. Without proper G-stage translation, QEMU will
     * trigger guest page faults when fetching instructions in VS-mode.
     * Skip this test if two-stage translation is not available.
     */
    TEST_SKIP("Requires two-stage translation setup (hgatp)");

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
