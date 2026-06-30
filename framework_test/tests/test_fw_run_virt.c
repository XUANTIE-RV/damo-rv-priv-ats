/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 9: run_in_priv() with virtualization
 *
 * Tests FW-RUN-VIRT-01 through FW-RUN-VIRT-06
 * Requires ENABLE_HYP=1
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

    /*
     * This test requires two-stage translation setup (hgatp) to execute
     * code in VS-mode. Without proper G-stage translation, QEMU will
     * trigger guest page faults when fetching instructions in VS-mode.
     */
    TEST_SKIP("Requires two-stage translation setup (hgatp)");

    TEST_END();
}

/* FW-RUN-VIRT-02: run_in_priv VU-mode execution */
bool test_fw_run_virt_02(void)
{
    TEST_BEGIN("FW-RUN-VIRT-02: run_in_priv VU-mode execution");

    /*
     * This test requires two-stage translation setup (hgatp) to execute
     * code in VU-mode.
     */
    TEST_SKIP("Requires two-stage translation setup (hgatp)");

    TEST_END();
}

/* FW-RUN-VIRT-03: run_in_priv VS-mode privilege verification */
bool test_fw_run_virt_03(void)
{
    TEST_BEGIN("FW-RUN-VIRT-03: run_in_priv VS-mode privilege verification");

    /*
     * This test requires two-stage translation setup (hgatp) to execute
     * code in VS-mode.
     */
    TEST_SKIP("Requires two-stage translation setup (hgatp)");

    TEST_END();
}

/* FW-RUN-VIRT-04: run_in_priv VU-mode privilege verification */
bool test_fw_run_virt_04(void)
{
    TEST_BEGIN("FW-RUN-VIRT-04: run_in_priv VU-mode privilege verification");

    /*
     * This test requires two-stage translation setup (hgatp) to execute
     * code in VU-mode.
     */
    TEST_SKIP("Requires two-stage translation setup (hgatp)");

    TEST_END();
}

/* FW-RUN-VIRT-05: run_in_priv VS returns to M-mode */
bool test_fw_run_virt_05(void)
{
    TEST_BEGIN("FW-RUN-VIRT-05: run_in_priv VS returns to M-mode");

    /*
     * This test requires two-stage translation setup (hgatp) to execute
     * code in VS-mode.
     */
    TEST_SKIP("Requires two-stage translation setup (hgatp)");

    TEST_END();
}

/* FW-RUN-VIRT-06: run_in_priv VU returns to M-mode */
bool test_fw_run_virt_06(void)
{
    TEST_BEGIN("FW-RUN-VIRT-06: run_in_priv VU returns to M-mode");

    /*
     * This test requires two-stage translation setup (hgatp) to execute
     * code in VU-mode.
     */
    TEST_SKIP("Requires two-stage translation setup (hgatp)");

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
