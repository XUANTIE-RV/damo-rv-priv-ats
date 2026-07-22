/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zicbom CBIE CSR Control Tests (Group 1)
 *
 * Tests menvcfg.CBIE / senvcfg.CBIE control of cbo.inval execution
 * across privilege levels.
 *
 * Reference: CMO_test_plan.md Group 1
 * Spec: norm:cbo-inval, norm:cbxe_unaffected
 */

#include "test_framework.h"
#include "cmo.h"

/* ===================================================================
 * CBIE-01: M-mode cbo.inval not restricted by CBIE
 * =================================================================== */
TEST_REGISTER(test_cbie_01);
bool test_cbie_01(void)
{
    TEST_BEGIN("CBIE-01: M-mode cbo.inval not restricted by CBIE");

    /* Set menvcfg.CBIE=00 (disable for lower privs) */
    menvcfg_set_cbie(CBIE_DISABLE);

    /* M-mode should still execute cbo.inval regardless of CBIE */
    extern char __cmo_test_data_start[];
    EXPECT_NO_TRAP(CBO_INVAL(__cmo_test_data_start));

    TEST_END();
}

/* ===================================================================
 * CBIE-02: HS-mode CBIE=00 triggers illegal-instruction
 * =================================================================== */
TEST_REGISTER(test_cbie_02);
bool test_cbie_02(void)
{
    TEST_BEGIN("CBIE-02: HS-mode CBIE=00 triggers illegal-instruction");

    menvcfg_set_cbie(CBIE_DISABLE);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(CBO_INVAL(0x80000000UL));
    goto_priv(PRIV_M);

    CHECK_TRAP("cbo.inval in HS with CBIE=00", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * CBIE-03: HS-mode CBIE=01 executes flush
 * =================================================================== */
TEST_REGISTER(test_cbie_03);
bool test_cbie_03(void)
{
    TEST_BEGIN("CBIE-03: HS-mode CBIE=01 executes flush");

    menvcfg_set_cbie(CBIE_FLUSH);

    extern char __cmo_test_data_start[];
    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(CBO_INVAL(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.inval in HS with CBIE=01");

    TEST_END();
}

/* ===================================================================
 * CBIE-04: HS-mode CBIE=11 executes invalidate
 * =================================================================== */
TEST_REGISTER(test_cbie_04);
bool test_cbie_04(void)
{
    TEST_BEGIN("CBIE-04: HS-mode CBIE=11 executes invalidate");

    menvcfg_set_cbie(CBIE_INVAL);

    extern char __cmo_test_data_start[];
    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(CBO_INVAL(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.inval in HS with CBIE=11");

    TEST_END();
}

/* ===================================================================
 * CBIE-05: U-mode menvcfg.CBIE=00 triggers illegal-instruction
 * =================================================================== */
TEST_REGISTER(test_cbie_05);
bool test_cbie_05(void)
{
    TEST_BEGIN("CBIE-05: U-mode menvcfg.CBIE=00 triggers illegal-instruction");

    menvcfg_set_cbie(CBIE_DISABLE);

    goto_priv(PRIV_U);
    PRIV_DO_TRAP(CBO_INVAL(0x80000000UL));
    goto_priv(PRIV_M);

    CHECK_TRAP("cbo.inval in U with menvcfg.CBIE=00", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * CBIE-06: U-mode menvcfg.CBIE!=00 but senvcfg.CBIE=00
 * =================================================================== */
TEST_REGISTER(test_cbie_06);
bool test_cbie_06(void)
{
    TEST_BEGIN("CBIE-06: U-mode menvcfg.CBIE!=00 but senvcfg.CBIE=00");

    menvcfg_set_cbie(CBIE_INVAL);
    senvcfg_set_cbie(CBIE_DISABLE);

    goto_priv(PRIV_U);
    PRIV_DO_TRAP(CBO_INVAL(0x80000000UL));
    goto_priv(PRIV_M);

    CHECK_TRAP("cbo.inval in U with senvcfg.CBIE=00", CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * CBIE-07: U-mode both CBIE nonzero executes
 * =================================================================== */
TEST_REGISTER(test_cbie_07);
bool test_cbie_07(void)
{
    TEST_BEGIN("CBIE-07: U-mode both CBIE nonzero executes");

    menvcfg_set_cbie(CBIE_INVAL);
    senvcfg_set_cbie(CBIE_INVAL);

    extern char __cmo_test_data_start[];
    goto_priv(PRIV_U);
    PRIV_DO_NO_TRAP(CBO_INVAL(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.inval in U with both CBIE=11");

    TEST_END();
}

/* ===================================================================
 * CBIE-08: U-mode senvcfg.CBIE=01 executes flush
 * =================================================================== */
TEST_REGISTER(test_cbie_08);
bool test_cbie_08(void)
{
    TEST_BEGIN("CBIE-08: U-mode senvcfg.CBIE=01 executes flush");

    menvcfg_set_cbie(CBIE_INVAL);
    senvcfg_set_cbie(CBIE_FLUSH);

    extern char __cmo_test_data_start[];
    goto_priv(PRIV_U);
    PRIV_DO_NO_TRAP(CBO_INVAL(__cmo_test_data_start));
    goto_priv(PRIV_M);

    CHECK_NO_TRAP("cbo.inval in U with senvcfg.CBIE=01");

    TEST_END();
}

/* ===================================================================
 * CBIE-17: CBIE WARL reserved encoding 10 behavior
 * =================================================================== */
TEST_REGISTER(test_cbie_17);
bool test_cbie_17(void)
{
    TEST_BEGIN("CBIE-17: CBIE WARL reserved encoding 10 behavior");

    /* Write CBIE=10 (reserved), read back - should not retain 10 */
    menvcfg_set_cbie(CBIE_RESERVED);
    uintptr_t readback = menvcfg_get_cbie();

    TEST_ASSERT("CBIE=10 not retained (WARL)", readback != CBIE_RESERVED);

    TEST_END();
}

/* ===================================================================
 * CBIE-18: envcfg CBIE fields do not affect each other
 * =================================================================== */
TEST_REGISTER(test_cbie_18);
bool test_cbie_18(void)
{
    TEST_BEGIN("CBIE-18: envcfg CBIE fields do not affect each other");

    /* Set senvcfg.CBIE=11 first */
    senvcfg_set_cbie(CBIE_INVAL);

    /* Now set menvcfg.CBIE=00 */
    menvcfg_set_cbie(CBIE_DISABLE);

    /* senvcfg.CBIE should still be 11 */
    uintptr_t senvcfg_cbie = senvcfg_get_cbie();
    TEST_ASSERT_EQ("senvcfg.CBIE unaffected by menvcfg write",
                   senvcfg_cbie, CBIE_INVAL);

    TEST_END();
}
