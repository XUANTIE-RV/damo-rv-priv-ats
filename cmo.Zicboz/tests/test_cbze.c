/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zicboz CBZE CSR Control Tests (Group 3)
 *
 * Tests menvcfg.CBZE / senvcfg.CBZE control of cbo.zero execution
 * across privilege levels.
 *
 * Reference: CMO_test_plan.md Group 3
 * Spec: norm:cbo-zero_basedon_xenvcfg-CBZE, norm:cbxe_unaffected
 */

#include "test_framework.h"
#include "cmo.h"

/* ===================================================================
 * CBZE-01: M-mode cbo.zero not restricted by CBZE
 * =================================================================== */
TEST_REGISTER(test_cbze_01);
bool test_cbze_01(void)
{
    TEST_BEGIN("CBZE-01: M-mode cbo.zero not restricted by CBZE");

    menvcfg_set_cbze(0);

    extern char __cmo_test_data_start[];
    EXPECT_NO_TRAP(CBO_ZERO(__cmo_test_data_start));

    TEST_END();
}

/* ===================================================================
 * CBZE-02: HS-mode CBZE=0 triggers illegal-instruction
 * =================================================================== */
TEST_REGISTER(test_cbze_02);
bool test_cbze_02(void)
{
    TEST_BEGIN("CBZE-02: HS-mode CBZE=0 triggers illegal-instruction");

    menvcfg_set_cbze(0);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(CBO_ZERO(0x80000000UL));
    goto_priv(PRIV_M);

    CHECK_TRAP("cbo.zero in HS with CBZE=0", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * CBZE-03: HS-mode CBZE=1 executes normally
 * =================================================================== */
TEST_REGISTER(test_cbze_03);
bool test_cbze_03(void)
{
    TEST_BEGIN("CBZE-03: HS-mode CBZE=1 executes normally");

    menvcfg_set_cbze(1);

    extern char __cmo_test_data_start[];
    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(CBO_ZERO(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.zero in HS with CBZE=1");

    TEST_END();
}

/* ===================================================================
 * CBZE-04: U-mode menvcfg.CBZE=0 triggers illegal-instruction
 * =================================================================== */
TEST_REGISTER(test_cbze_04);
bool test_cbze_04(void)
{
    TEST_BEGIN("CBZE-04: U-mode menvcfg.CBZE=0 triggers illegal-instruction");

    menvcfg_set_cbze(0);

    goto_priv(PRIV_U);
    PRIV_DO_TRAP(CBO_ZERO(0x80000000UL));
    goto_priv(PRIV_M);

    CHECK_TRAP("cbo.zero in U with menvcfg.CBZE=0", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * CBZE-05: U-mode menvcfg.CBZE=1 but senvcfg.CBZE=0
 * =================================================================== */
TEST_REGISTER(test_cbze_05);
bool test_cbze_05(void)
{
    TEST_BEGIN("CBZE-05: U-mode menvcfg.CBZE=1 but senvcfg.CBZE=0");

    menvcfg_set_cbze(1);
    senvcfg_set_cbze(0);

    goto_priv(PRIV_U);
    PRIV_DO_TRAP(CBO_ZERO(0x80000000UL));
    goto_priv(PRIV_M);

    CHECK_TRAP("cbo.zero in U with senvcfg.CBZE=0", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * CBZE-06: U-mode both CBZE=1 executes normally
 * =================================================================== */
TEST_REGISTER(test_cbze_06);
bool test_cbze_06(void)
{
    TEST_BEGIN("CBZE-06: U-mode both CBZE=1 executes normally");

    menvcfg_set_cbze(1);
    senvcfg_set_cbze(1);

    extern char __cmo_test_data_start[];
    goto_priv(PRIV_U);
    PRIV_DO_NO_TRAP(CBO_ZERO(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.zero in U with both CBZE=1");

    TEST_END();
}

/* ===================================================================
 * CBZE-12: envcfg CBZE fields do not affect each other
 * =================================================================== */
TEST_REGISTER(test_cbze_12);
bool test_cbze_12(void)
{
    TEST_BEGIN("CBZE-12: envcfg CBZE fields do not affect each other");

    /* Set senvcfg.CBZE=1 first */
    senvcfg_set_cbze(1);

    /* Now clear menvcfg.CBZE */
    menvcfg_set_cbze(0);

    /* senvcfg.CBZE should still be 1 */
    uintptr_t senvcfg_cbze = senvcfg_get_cbze();
    TEST_ASSERT_EQ("senvcfg.CBZE unaffected by menvcfg write",
                   senvcfg_cbze, 1);

    TEST_END();
}
