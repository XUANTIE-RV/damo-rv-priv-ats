/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 5: run_in_priv() function execution
 *
 * Tests FW-RUN-01 through FW-RUN-08
 */

#include "test_framework.h"

/* Helper: return a fixed magic value */
static uintptr_t return_magic_fn(uintptr_t arg)
{
    (void)arg;
    return 0xDEADBEEF;
}

/* Helper: return the argument passed */
static uintptr_t return_arg_fn(uintptr_t arg)
{
    return arg;
}

/* Helper: return current privilege level */
static uintptr_t get_priv_fn(uintptr_t arg)
{
    (void)arg;
    return (uintptr_t)get_current_priv();
}

/* FW-RUN-01: run_in_priv S-mode execution */
bool test_fw_run_01(void)
{
    TEST_BEGIN("FW-RUN-01: run_in_priv S-mode execution");

    uintptr_t result = run_in_priv(PRIV_S, return_magic_fn, 0);
    TEST_ASSERT_EQ("function should return magic value", result, 0xDEADBEEF);

    TEST_END();
}

/* FW-RUN-02: run_in_priv U-mode execution */
bool test_fw_run_02(void)
{
    TEST_BEGIN("FW-RUN-02: run_in_priv U-mode execution");

    uintptr_t result = run_in_priv(PRIV_U, return_magic_fn, 0);
    TEST_ASSERT_EQ("function should return magic value", result, 0xDEADBEEF);

    TEST_END();
}

/* FW-RUN-03: run_in_priv S-mode privilege verification */
bool test_fw_run_03(void)
{
    TEST_BEGIN("FW-RUN-03: run_in_priv S-mode privilege verification");

    printf("[DEBUG] Before run_in_priv(PRIV_S)\n");
    uintptr_t priv = run_in_priv(PRIV_S, get_priv_fn, 0);
    printf("[DEBUG] After run_in_priv(PRIV_S), priv=%lu\n", (unsigned long)priv);
    TEST_ASSERT_EQ("should be in S-mode during execution", priv, PRIV_S);

    TEST_END();
}

/* FW-RUN-04: run_in_priv U-mode privilege verification */
bool test_fw_run_04(void)
{
    TEST_BEGIN("FW-RUN-04: run_in_priv U-mode privilege verification");

    uintptr_t priv = run_in_priv(PRIV_U, get_priv_fn, 0);
    TEST_ASSERT_EQ("should be in U-mode during execution", priv, PRIV_U);

    TEST_END();
}

/* FW-RUN-05: run_in_priv returns to M-mode */
bool test_fw_run_05(void)
{
    TEST_BEGIN("FW-RUN-05: run_in_priv returns to M-mode");

    run_in_priv(PRIV_S, return_magic_fn, 0);
    TEST_ASSERT_EQ("should be back in M-mode", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-RUN-06: run_in_priv parameter passing */
bool test_fw_run_06(void)
{
    TEST_BEGIN("FW-RUN-06: run_in_priv parameter passing");

    uintptr_t test_arg = 0x12345678;
    uintptr_t result = run_in_priv(PRIV_S, return_arg_fn, test_arg);
    TEST_ASSERT_EQ("argument should be passed correctly", result, test_arg);

    TEST_END();
}

/* FW-RUN-07: run_in_priv M-mode direct call */
bool test_fw_run_07(void)
{
    TEST_BEGIN("FW-RUN-07: run_in_priv M-mode direct call");

    /*
     * When target priv >= current priv, run_in_priv calls fn directly
     * without mret/sret.
     */
    uintptr_t result = run_in_priv(PRIV_M, return_magic_fn, 0);
    TEST_ASSERT_EQ("direct call should work", result, 0xDEADBEEF);

    uintptr_t priv = run_in_priv(PRIV_M, get_priv_fn, 0);
    TEST_ASSERT_EQ("should execute in M-mode", priv, PRIV_M);

    TEST_END();
}

/* FW-RUN-08: run_in_priv same-privilege direct call */
bool test_fw_run_08(void)
{
    TEST_BEGIN("FW-RUN-08: run_in_priv same-privilege direct call");

    /*
     * From S-mode, calling run_in_priv(PRIV_S, ...) should execute
     * directly without switching.
     */
    goto_priv(PRIV_S);

    uintptr_t result = run_in_priv(PRIV_S, return_magic_fn, 0);
    TEST_ASSERT_EQ("same-priv call should work", result, 0xDEADBEEF);

    uintptr_t priv = run_in_priv(PRIV_S, get_priv_fn, 0);
    TEST_ASSERT_EQ("should execute in S-mode", priv, PRIV_S);

    TEST_END();
}

/* Register all tests */
TEST_REGISTER(test_fw_run_01);
TEST_REGISTER(test_fw_run_02);
TEST_REGISTER(test_fw_run_03);
TEST_REGISTER(test_fw_run_04);
TEST_REGISTER(test_fw_run_05);
TEST_REGISTER(test_fw_run_06);
TEST_REGISTER(test_fw_run_07);
TEST_REGISTER(test_fw_run_08);
