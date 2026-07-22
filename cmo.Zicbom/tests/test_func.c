/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zicbom Functional Tests (Group 4)
 *
 * Tests cbo.inval / cbo.clean / cbo.flush basic functionality,
 * unaligned address behavior, and cache block operations.
 *
 * Reference: CMO_test_plan.md Group 4
 * Spec: norm:cbo-inval_op, norm:cbo-clean_op, norm:cbo-flush_op,
 *       norm:cbo-inval_unaligned, norm:cbo-flush_unaligned
 */

#include "test_framework.h"
#include "cmo.h"
#include "mem_ops.h"

/* Linker-provided test data region (cache block aligned) */
extern char __cmo_test_data_start[];

/* ===================================================================
 * CBM-FUNC-05: cbo.clean rs1 unaligned - no misaligned exception
 * =================================================================== */
TEST_REGISTER(test_cbm_func_05);
bool test_cbm_func_05(void)
{
    TEST_BEGIN("CBM-FUNC-05: cbo.clean rs1 unaligned");

    /* Enable CBO for S-mode */
    menvcfg_set_cbcfe(1);

    /* Use unaligned address: base + 1 */
    uintptr_t unaligned = (uintptr_t)__cmo_test_data_start + 1;

    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(CBO_CLEAN(unaligned));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.clean with unaligned rs1");

    TEST_END();
}

/* ===================================================================
 * CBM-FUNC-06: cbo.flush rs1 unaligned - no misaligned exception
 * =================================================================== */
TEST_REGISTER(test_cbm_func_06);
bool test_cbm_func_06(void)
{
    TEST_BEGIN("CBM-FUNC-06: cbo.flush rs1 unaligned");

    menvcfg_set_cbcfe(1);

    /* Use unaligned address: base + 3 */
    uintptr_t unaligned = (uintptr_t)__cmo_test_data_start + 3;

    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(CBO_FLUSH(unaligned));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.flush with unaligned rs1");

    TEST_END();
}

/* ===================================================================
 * CBM-FUNC-07: cbo.inval rs1 unaligned - no misaligned exception
 * =================================================================== */
TEST_REGISTER(test_cbm_func_07);
bool test_cbm_func_07(void)
{
    TEST_BEGIN("CBM-FUNC-07: cbo.inval rs1 unaligned");

    menvcfg_set_cbie(CBIE_INVAL);

    /* Use unaligned address: base + 7 */
    uintptr_t unaligned = (uintptr_t)__cmo_test_data_start + 7;

    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(CBO_INVAL(unaligned));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.inval with unaligned rs1");

    TEST_END();
}

/* ===================================================================
 * CBM-FUNC-11: cbo.clean on clean block has no side effects
 * =================================================================== */
TEST_REGISTER(test_cbm_func_11);
bool test_cbm_func_11(void)
{
    TEST_BEGIN("CBM-FUNC-11: cbo.clean on clean block no side effects");

    menvcfg_set_cbcfe(1);

    /* First flush to ensure block is clean */
    CBO_FLUSH(__cmo_test_data_start);

    /* Now clean again - should not cause any issue */
    EXPECT_NO_TRAP(CBO_CLEAN(__cmo_test_data_start));

    TEST_END();
}

/* ===================================================================
 * CBM-FUNC-14: cache block size discovery
 * =================================================================== */
TEST_REGISTER(test_cbm_func_14);
bool test_cbm_func_14(void)
{
    TEST_BEGIN("CBM-FUNC-14: cache block size discovery");

    /* Verify configured cache block size is a power of 2 */
    unsigned int cbs = CMO_CACHE_BLOCK_SIZE;
    TEST_ASSERT("cache block size is power of 2",
                (cbs & (cbs - 1)) == 0);
    TEST_ASSERT("cache block size >= 16", cbs >= 16);
    TEST_ASSERT("cache block size <= 256", cbs <= 256);

    TEST_END();
}
