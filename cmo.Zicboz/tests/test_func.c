/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zicboz Functional Tests (Group 5)
 *
 * Tests cbo.zero basic functionality: zeroing cache blocks,
 * unaligned address behavior, and cache block boundary.
 *
 * Reference: CMO_test_plan.md Group 5
 * Spec: norm:cbo-zero_op, norm:cbo-zero_unaligned, norm:cbo-zero_specified_block
 */

#include "test_framework.h"
#include "cmo.h"
#include "mem_ops.h"

/* Linker-provided test data region (cache block aligned) */
extern char __cmo_test_data_start[];

/* ===================================================================
 * CBZ-FUNC-01: cbo.zero basic zeroing functionality
 * =================================================================== */
TEST_REGISTER(test_cbz_func_01);
bool test_cbz_func_01(void)
{
    TEST_BEGIN("CBZ-FUNC-01: cbo.zero basic zeroing functionality");

    menvcfg_set_cbze(1);

    volatile uint64_t *block = (volatile uint64_t *)__cmo_test_data_start;

    /* Write non-zero pattern to first cache block */
    for (unsigned i = 0; i < CMO_CACHE_BLOCK_SIZE / 8; i++)
    {
        block[i] = 0xDEADBEEFCAFE0000ULL + i;
    }

    /* Execute cbo.zero */
    CBO_ZERO(__cmo_test_data_start);

    /* Verify all bytes are zero */
    bool all_zero = true;
    for (unsigned i = 0; i < CMO_CACHE_BLOCK_SIZE / 8; i++)
    {
        if (block[i] != 0)
        {
            all_zero = false;
            break;
        }
    }
    TEST_ASSERT("cache block zeroed", all_zero);

    TEST_END();
}

/* ===================================================================
 * CBZ-FUNC-03: cbo.zero rs1 unaligned - no misaligned exception
 * =================================================================== */
TEST_REGISTER(test_cbz_func_03);
bool test_cbz_func_03(void)
{
    TEST_BEGIN("CBZ-FUNC-03: cbo.zero rs1 unaligned");

    menvcfg_set_cbze(1);

    volatile uint64_t *block = (volatile uint64_t *)__cmo_test_data_start;

    /* Write non-zero pattern */
    for (unsigned i = 0; i < CMO_CACHE_BLOCK_SIZE / 8; i++)
    {
        block[i] = 0xAAAAAAAAAAAAAAAAULL;
    }

    /* Use unaligned address: base + 5 */
    uintptr_t unaligned = (uintptr_t)__cmo_test_data_start + 5;

    /* Should not trigger misaligned exception */
    EXPECT_NO_TRAP(CBO_ZERO(unaligned));

    /* Entire cache block should be zeroed */
    bool all_zero = true;
    for (unsigned i = 0; i < CMO_CACHE_BLOCK_SIZE / 8; i++)
    {
        if (block[i] != 0)
        {
            all_zero = false;
            break;
        }
    }
    TEST_ASSERT("cache block zeroed with unaligned rs1", all_zero);

    TEST_END();
}

/* ===================================================================
 * CBZ-FUNC-04: cbo.zero only operates on one cache block
 * =================================================================== */
TEST_REGISTER(test_cbz_func_04);
bool test_cbz_func_04(void)
{
    TEST_BEGIN("CBZ-FUNC-04: cbo.zero only operates on one cache block");

    menvcfg_set_cbze(1);

    volatile uint64_t *block0 = (volatile uint64_t *)__cmo_test_data_start;
    volatile uint64_t *block1 = (volatile uint64_t *)(
        (uintptr_t)__cmo_test_data_start + CMO_CACHE_BLOCK_SIZE);

    /* Write non-zero pattern to both blocks */
    for (unsigned i = 0; i < CMO_CACHE_BLOCK_SIZE / 8; i++)
    {
        block0[i] = 0x1111111111111111ULL;
        block1[i] = 0x2222222222222222ULL;
    }

    /* Zero only first block */
    CBO_ZERO(__cmo_test_data_start);

    /* First block should be zero */
    bool block0_zero = true;
    for (unsigned i = 0; i < CMO_CACHE_BLOCK_SIZE / 8; i++)
    {
        if (block0[i] != 0)
        {
            block0_zero = false;
            break;
        }
    }
    TEST_ASSERT("target block zeroed", block0_zero);

    /* Second block should be unchanged */
    bool block1_intact = true;
    for (unsigned i = 0; i < CMO_CACHE_BLOCK_SIZE / 8; i++)
    {
        if (block1[i] != 0x2222222222222222ULL)
        {
            block1_intact = false;
            break;
        }
    }
    TEST_ASSERT("adjacent block unchanged", block1_intact);

    TEST_END();
}

/* ===================================================================
 * CBZ-FUNC-06: cbo.zero on already-zero block
 * =================================================================== */
TEST_REGISTER(test_cbz_func_06);
bool test_cbz_func_06(void)
{
    TEST_BEGIN("CBZ-FUNC-06: cbo.zero on already-zero block");

    menvcfg_set_cbze(1);

    /* First zero the block */
    CBO_ZERO(__cmo_test_data_start);

    /* Zero again - should not cause any issue */
    EXPECT_NO_TRAP(CBO_ZERO(__cmo_test_data_start));

    TEST_END();
}

/* ===================================================================
 * CBZ-FUNC-07: cbo.zero then load returns zero
 * =================================================================== */
TEST_REGISTER(test_cbz_func_07);
bool test_cbz_func_07(void)
{
    TEST_BEGIN("CBZ-FUNC-07: cbo.zero then load returns zero");

    menvcfg_set_cbze(1);

    volatile uint64_t *ptr = (volatile uint64_t *)__cmo_test_data_start;

    /* Write non-zero */
    *ptr = 0xFEDCBA9876543210ULL;

    /* Zero the block */
    CBO_ZERO(__cmo_test_data_start);

    /* Load should return 0 */
    uint64_t val = *ptr;
    TEST_ASSERT_EQ("load returns zero after cbo.zero", val, 0);

    TEST_END();
}
