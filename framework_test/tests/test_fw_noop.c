/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 3: Same-privilege switching (no-op)
 *
 * Tests FW-NOOP-01 through FW-NOOP-03
 */

#include "test_framework.h"

/* FW-NOOP-01: M->M same-privilege switch */
bool test_fw_noop_01(void)
{
    TEST_BEGIN("FW-NOOP-01: M->M same-privilege switch");

    /* Should be no-op, no ecall/mret executed */
    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("current_priv should remain PRIV_M", get_current_priv(), PRIV_M);

    TEST_END();
}

/* FW-NOOP-02: S->S same-privilege switch */
bool test_fw_noop_02(void)
{
    TEST_BEGIN("FW-NOOP-02: S->S same-privilege switch");

    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should be PRIV_S", get_current_priv(), PRIV_S);

    /* Should be no-op */
    goto_priv(PRIV_S);
    TEST_ASSERT_EQ("current_priv should remain PRIV_S", get_current_priv(), PRIV_S);

    TEST_END();
}

/* FW-NOOP-03: U->U same-privilege switch */
bool test_fw_noop_03(void)
{
    TEST_BEGIN("FW-NOOP-03: U->U same-privilege switch");

    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("current_priv should be PRIV_U", get_current_priv(), PRIV_U);

    /* Should be no-op */
    goto_priv(PRIV_U);
    TEST_ASSERT_EQ("current_priv should remain PRIV_U", get_current_priv(), PRIV_U);

    TEST_END();
}

/* Register all tests */
TEST_REGISTER(test_fw_noop_01);
TEST_REGISTER(test_fw_noop_02);
TEST_REGISTER(test_fw_noop_03);
