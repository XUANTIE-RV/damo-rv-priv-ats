/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zicbom CBCFE CSR Control Tests (Group 2)
 *
 * Tests menvcfg.CBCFE / senvcfg.CBCFE control of cbo.clean and
 * cbo.flush execution across privilege levels.
 *
 * Reference: CMO_test_plan.md Group 2
 * Spec: norm:cbo-clean_cbo-flush, norm:cbxe_unaffected
 */

#include "test_framework.h"
#include "cmo.h"

/* ===================================================================
 * CBCFE-01: M-mode cbo.clean not restricted by CBCFE
 * =================================================================== */
TEST_REGISTER(test_cbcfe_01);
bool test_cbcfe_01(void)
{
    TEST_BEGIN("CBCFE-01: M-mode cbo.clean not restricted by CBCFE");

    menvcfg_set_cbcfe(0);

    extern char __cmo_test_data_start[];
    EXPECT_NO_TRAP(CBO_CLEAN(__cmo_test_data_start));

    TEST_END();
}

/* ===================================================================
 * CBCFE-02: M-mode cbo.flush not restricted by CBCFE
 * =================================================================== */
TEST_REGISTER(test_cbcfe_02);
bool test_cbcfe_02(void)
{
    TEST_BEGIN("CBCFE-02: M-mode cbo.flush not restricted by CBCFE");

    menvcfg_set_cbcfe(0);

    extern char __cmo_test_data_start[];
    EXPECT_NO_TRAP(CBO_FLUSH(__cmo_test_data_start));

    TEST_END();
}

/* ===================================================================
 * CBCFE-03: HS-mode CBCFE=0 cbo.clean triggers illegal-instruction
 * =================================================================== */
TEST_REGISTER(test_cbcfe_03);
bool test_cbcfe_03(void)
{
    TEST_BEGIN("CBCFE-03: HS-mode CBCFE=0 cbo.clean triggers illegal-instruction");

    menvcfg_set_cbcfe(0);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(CBO_CLEAN(0x80000000UL));
    goto_priv(PRIV_M);

    CHECK_TRAP("cbo.clean in HS with CBCFE=0", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * CBCFE-04: HS-mode CBCFE=0 cbo.flush triggers illegal-instruction
 * =================================================================== */
TEST_REGISTER(test_cbcfe_04);
bool test_cbcfe_04(void)
{
    TEST_BEGIN("CBCFE-04: HS-mode CBCFE=0 cbo.flush triggers illegal-instruction");

    menvcfg_set_cbcfe(0);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(CBO_FLUSH(0x80000000UL));
    goto_priv(PRIV_M);

    CHECK_TRAP("cbo.flush in HS with CBCFE=0", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * CBCFE-05: HS-mode CBCFE=1 cbo.clean executes normally
 * =================================================================== */
TEST_REGISTER(test_cbcfe_05);
bool test_cbcfe_05(void)
{
    TEST_BEGIN("CBCFE-05: HS-mode CBCFE=1 cbo.clean executes normally");

    menvcfg_set_cbcfe(1);

    extern char __cmo_test_data_start[];
    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(CBO_CLEAN(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.clean in HS with CBCFE=1");

    TEST_END();
}

/* ===================================================================
 * CBCFE-06: HS-mode CBCFE=1 cbo.flush executes normally
 * =================================================================== */
TEST_REGISTER(test_cbcfe_06);
bool test_cbcfe_06(void)
{
    TEST_BEGIN("CBCFE-06: HS-mode CBCFE=1 cbo.flush executes normally");

    menvcfg_set_cbcfe(1);

    extern char __cmo_test_data_start[];
    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(CBO_FLUSH(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.flush in HS with CBCFE=1");

    TEST_END();
}

/* ===================================================================
 * CBCFE-07: U-mode menvcfg.CBCFE=0 triggers illegal-instruction
 * =================================================================== */
TEST_REGISTER(test_cbcfe_07);
bool test_cbcfe_07(void)
{
    TEST_BEGIN("CBCFE-07: U-mode menvcfg.CBCFE=0 triggers illegal-instruction");

    menvcfg_set_cbcfe(0);

    goto_priv(PRIV_U);
    PRIV_DO_TRAP(CBO_CLEAN(0x80000000UL));
    goto_priv(PRIV_M);

    CHECK_TRAP("cbo.clean in U with menvcfg.CBCFE=0", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * CBCFE-08: U-mode menvcfg.CBCFE=1 but senvcfg.CBCFE=0
 * =================================================================== */
TEST_REGISTER(test_cbcfe_08);
bool test_cbcfe_08(void)
{
    TEST_BEGIN("CBCFE-08: U-mode menvcfg.CBCFE=1 but senvcfg.CBCFE=0");

    menvcfg_set_cbcfe(1);
    senvcfg_set_cbcfe(0);

    goto_priv(PRIV_U);
    PRIV_DO_TRAP(CBO_CLEAN(0x80000000UL));
    goto_priv(PRIV_M);

    CHECK_TRAP("cbo.clean in U with senvcfg.CBCFE=0", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * CBCFE-09: U-mode both CBCFE=1 cbo.clean executes
 * =================================================================== */
TEST_REGISTER(test_cbcfe_09);
bool test_cbcfe_09(void)
{
    TEST_BEGIN("CBCFE-09: U-mode both CBCFE=1 cbo.clean executes");

    menvcfg_set_cbcfe(1);
    senvcfg_set_cbcfe(1);

    extern char __cmo_test_data_start[];
    goto_priv(PRIV_U);
    PRIV_DO_NO_TRAP(CBO_CLEAN(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.clean in U with both CBCFE=1");

    TEST_END();
}

/* ===================================================================
 * CBCFE-10: U-mode both CBCFE=1 cbo.flush executes
 * =================================================================== */
TEST_REGISTER(test_cbcfe_10);
bool test_cbcfe_10(void)
{
    TEST_BEGIN("CBCFE-10: U-mode both CBCFE=1 cbo.flush executes");

    menvcfg_set_cbcfe(1);
    senvcfg_set_cbcfe(1);

    extern char __cmo_test_data_start[];
    goto_priv(PRIV_U);
    PRIV_DO_NO_TRAP(CBO_FLUSH(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.flush in U with both CBCFE=1");

    TEST_END();
}

/* ===================================================================
 * CBCFE-18: envcfg CBCFE fields do not affect each other
 * =================================================================== */
TEST_REGISTER(test_cbcfe_18);
bool test_cbcfe_18(void)
{
    TEST_BEGIN("CBCFE-18: envcfg CBCFE fields do not affect each other");

    /* Set senvcfg.CBCFE=1 first */
    senvcfg_set_cbcfe(1);

    /* Now clear menvcfg.CBCFE */
    menvcfg_set_cbcfe(0);

    /* senvcfg.CBCFE should still be 1 */
    uintptr_t senvcfg_cbcfe = senvcfg_get_cbcfe();
    TEST_ASSERT_EQ("senvcfg.CBCFE unaffected by menvcfg write",
                   senvcfg_cbcfe, 1);

    TEST_END();
}
