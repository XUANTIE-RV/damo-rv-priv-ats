/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zicbop Prefetch Functional Tests (Group 6)
 *
 * Tests prefetch.r / prefetch.w / prefetch.i HINT semantics:
 * - No exceptions on any address
 * - No effect on accessed/dirty bits
 * - Not controlled by CBIE/CBCFE/CBZE
 *
 * Reference: CMO_test_plan.md Group 6
 * Spec: norm:prefetch_operating_block, norm:cbp_access,
 *       norm:cbp_unperm_noexcep, norm:cbp_unperm_translate
 */

#include "test_framework.h"
#include "cmo.h"

/* Linker-provided test data region */
extern char __cmo_test_data_start[];

/* ===================================================================
 * CBP-FUNC-01: prefetch.r basic execution - no exception
 * =================================================================== */
TEST_REGISTER(test_cbp_func_01);
bool test_cbp_func_01(void)
{
    TEST_BEGIN("CBP-FUNC-01: prefetch.r basic execution - no exception");

    EXPECT_NO_TRAP(PREFETCH_R(__cmo_test_data_start));

    TEST_END();
}

/* ===================================================================
 * CBP-FUNC-02: prefetch.w basic execution - no exception
 * =================================================================== */
TEST_REGISTER(test_cbp_func_02);
bool test_cbp_func_02(void)
{
    TEST_BEGIN("CBP-FUNC-02: prefetch.w basic execution - no exception");

    EXPECT_NO_TRAP(PREFETCH_W(__cmo_test_data_start));

    TEST_END();
}

/* ===================================================================
 * CBP-FUNC-03: prefetch.i basic execution - no exception
 * =================================================================== */
TEST_REGISTER(test_cbp_func_03);
bool test_cbp_func_03(void)
{
    TEST_BEGIN("CBP-FUNC-03: prefetch.i basic execution - no exception");

    EXPECT_NO_TRAP(PREFETCH_I(__cmo_test_data_start));

    TEST_END();
}

/* ===================================================================
 * CBP-FUNC-06: prefetch on invalid address - no exception
 * =================================================================== */
TEST_REGISTER(test_cbp_func_06);
bool test_cbp_func_06(void)
{
    TEST_BEGIN("CBP-FUNC-06: prefetch on invalid address - no exception");

    /* Use an unmapped/invalid address - prefetch should not trap */
    uintptr_t invalid_addr = 0xDEAD0000UL;

    EXPECT_NO_TRAP(PREFETCH_R(invalid_addr));

    TEST_END();
}

/* ===================================================================
 * CBP-FUNC-10: prefetch does not raise illegal-instruction
 * =================================================================== */
TEST_REGISTER(test_cbp_func_10);
bool test_cbp_func_10(void)
{
    TEST_BEGIN("CBP-FUNC-10: prefetch does not raise illegal-instruction");

    /* Disable all CBO controls */
    menvcfg_set_cbie(CBIE_DISABLE);
    menvcfg_set_cbcfe(0);
    menvcfg_set_cbze(0);

    /* Prefetch should still work in S-mode */
    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(PREFETCH_R(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("prefetch.r in S-mode with all CBO disabled");

    /* Also test in U-mode */
    goto_priv(PRIV_U);
    PRIV_DO_NO_TRAP(PREFETCH_W(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("prefetch.w in U-mode with all CBO disabled");

    TEST_END();
}

/* ===================================================================
 * CBP-FUNC-14: prefetch not controlled by CBIE/CBCFE/CBZE
 * =================================================================== */
TEST_REGISTER(test_cbp_func_14);
bool test_cbp_func_14(void)
{
    TEST_BEGIN("CBP-FUNC-14: prefetch not controlled by CBIE/CBCFE/CBZE");

    /* Set all envcfg CMO fields to disable */
    menvcfg_set_cbie(CBIE_DISABLE);
    menvcfg_set_cbcfe(0);
    menvcfg_set_cbze(0);
    senvcfg_set_cbie(CBIE_DISABLE);
    senvcfg_set_cbcfe(0);
    senvcfg_set_cbze(0);

    /* All three prefetch types should still execute without trap */
    EXPECT_NO_TRAP(PREFETCH_R(__cmo_test_data_start));
    EXPECT_NO_TRAP(PREFETCH_W(__cmo_test_data_start));
    EXPECT_NO_TRAP(PREFETCH_I(__cmo_test_data_start));

    TEST_END();
}

/* ===================================================================
 * CBP-FUNC-S01: prefetch in S-mode - no exception
 * =================================================================== */
TEST_REGISTER(test_cbp_func_s01);
bool test_cbp_func_s01(void)
{
    TEST_BEGIN("CBP-FUNC-S01: prefetch in S-mode - no exception");

    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(PREFETCH_R(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("prefetch.r in S-mode");

    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(PREFETCH_W(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("prefetch.w in S-mode");

    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(PREFETCH_I(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("prefetch.i in S-mode");

    TEST_END();
}

/* ===================================================================
 * CBP-FUNC-U01: prefetch in U-mode - no exception
 * =================================================================== */
TEST_REGISTER(test_cbp_func_u01);
bool test_cbp_func_u01(void)
{
    TEST_BEGIN("CBP-FUNC-U01: prefetch in U-mode - no exception");

    goto_priv(PRIV_U);
    PRIV_DO_NO_TRAP(PREFETCH_R(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("prefetch.r in U-mode");

    goto_priv(PRIV_U);
    PRIV_DO_NO_TRAP(PREFETCH_W(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("prefetch.w in U-mode");

    goto_priv(PRIV_U);
    PRIV_DO_NO_TRAP(PREFETCH_I(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("prefetch.i in U-mode");

    TEST_END();
}
