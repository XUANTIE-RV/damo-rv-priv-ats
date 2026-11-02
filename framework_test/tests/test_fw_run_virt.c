/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 9: run_in_priv() with virtualization
 *
 * Tests FW-RUN-VIRT-01 through FW-RUN-VIRT-06
 * Requires ENABLE_HYP=1
 *
 * None of these needs a G-stage page table.  With hgatp.MODE=Bare and
 * vsatp.MODE=Bare a V=1 fetch translates to the physical address as-is, so
 * the trampoline runs in VS/VU exactly like it does in S/U.  Group 6/7/8
 * already switch into VS and VU and execute code there, which is the proof;
 * what this group adds is that run_in_priv() drives the same descent and
 * brings the caller back where it started.
 */

#include "test_framework.h"

#ifdef ENABLE_HYP

/* Helper: return magic value */
static uintptr_t return_magic_fn(uintptr_t arg)
{
    (void)arg;
    return 0xCAFEBABE;
}

/* Helper: return current privilege */
static uintptr_t get_priv_fn(uintptr_t arg)
{
    (void)arg;
    return (uintptr_t)get_current_priv();
}

/* FW-RUN-VIRT-01: run_in_priv VS-mode execution */
bool test_fw_run_virt_01(void)
{
    TEST_BEGIN("FW-RUN-VIRT-01: run_in_priv VS-mode execution");

    /* The plan puts the caller in M-mode: run_in_priv(PRIV_VS, ...) from M
     * takes the direct mret path (MPV=1, MPP=S). */
    goto_priv(PRIV_M);

    uintptr_t result = run_in_priv(PRIV_VS, return_magic_fn, 0);
    TEST_ASSERT_EQ("function should return magic value", result, 0xCAFEBABE);

    TEST_END();
}

/* FW-RUN-VIRT-02: run_in_priv VU-mode execution */
bool test_fw_run_virt_02(void)
{
    TEST_BEGIN("FW-RUN-VIRT-02: run_in_priv VU-mode execution");

    goto_priv(PRIV_M);

    uintptr_t result = run_in_priv(PRIV_VU, return_magic_fn, 0);
    TEST_ASSERT_EQ("function should return magic value", result, 0xCAFEBABE);

    TEST_END();
}

/* FW-RUN-VIRT-03: run_in_priv VS-mode privilege verification */
bool test_fw_run_virt_03(void)
{
    TEST_BEGIN("FW-RUN-VIRT-03: run_in_priv VS-mode privilege verification");

    goto_priv(PRIV_M);

    uintptr_t priv = run_in_priv(PRIV_VS, get_priv_fn, 0);
    TEST_ASSERT_EQ("should be in VS-mode during execution", priv, PRIV_VS);

    TEST_END();
}

/* FW-RUN-VIRT-04: run_in_priv VU-mode privilege verification */
bool test_fw_run_virt_04(void)
{
    TEST_BEGIN("FW-RUN-VIRT-04: run_in_priv VU-mode privilege verification");

    goto_priv(PRIV_M);

    uintptr_t priv = run_in_priv(PRIV_VU, get_priv_fn, 0);
    TEST_ASSERT_EQ("should be in VU-mode during execution", priv, PRIV_VU);

    TEST_END();
}

/* FW-RUN-VIRT-05: run_in_priv VS returns to M-mode */
bool test_fw_run_virt_05(void)
{
    TEST_BEGIN("FW-RUN-VIRT-05: run_in_priv VS returns to M-mode");

    goto_priv(PRIV_M);

    run_in_priv(PRIV_VS, return_magic_fn, 0);
    TEST_ASSERT_EQ("should be back in M-mode", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-RUN-VIRT-06: run_in_priv VU returns to M-mode */
bool test_fw_run_virt_06(void)
{
    TEST_BEGIN("FW-RUN-VIRT-06: run_in_priv VU returns to M-mode");

    goto_priv(PRIV_M);

    run_in_priv(PRIV_VU, return_magic_fn, 0);
    TEST_ASSERT_EQ("should be back in M-mode", get_current_priv(), PRIV_M);

    TEST_END();
}

TEST_REGISTER(test_fw_run_virt_01);
TEST_REGISTER(test_fw_run_virt_02);
TEST_REGISTER(test_fw_run_virt_03);
TEST_REGISTER(test_fw_run_virt_04);
TEST_REGISTER(test_fw_run_virt_05);
TEST_REGISTER(test_fw_run_virt_06);

#else /* !ENABLE_HYP */

bool test_fw_run_virt_skip(void)
{
    TEST_BEGIN("FW-RUN-VIRT: Virtualization run_in_priv tests");
    TEST_SKIP("ENABLE_HYP not set");
}

TEST_REGISTER(test_fw_run_virt_skip);

#endif /* ENABLE_HYP */
