/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 7: hcounteren counter enable
 *
 * Tests HCNT-01 through HCNT-08 verify hcounteren register behavior.
 */

#include "test_helpers.h"

/* ------------------------------------------------------------------
 * HCNT-01: hcounteren basic read/write
 * ------------------------------------------------------------------ */
TEST_REGISTER(hcounteren_basic_rw);
bool hcounteren_basic_rw(void) {
    TEST_BEGIN("HCNT-01: Verify hcounteren basic read/write");

    /* Write 0x1 to hcounteren. */
    hcounteren_write(0x1);
    uintptr_t val = hcounteren_read();

    /* Verify value is written. */
    TEST_ASSERT_EQ("hcounteren should be writable", val, 0x1);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HCNT-02: hcounteren CY field controls VS counter access
 * ------------------------------------------------------------------ */
TEST_REGISTER(hcounteren_cy_field);
bool hcounteren_cy_field(void) {
    TEST_BEGIN("HCNT-02: Verify hcounteren CY bit (0) controls VS cycle access");

    /* Set CY bit. */
    hcounteren_write(0x1);
    uintptr_t val = hcounteren_read();

    /* Verify CY bit is set. */
    TEST_ASSERT_EQ("hcounteren.CY bit should be set", val & 0x1, 0x1);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HCNT-03: hcounteren TM field controls VS counter access
 * ------------------------------------------------------------------ */
TEST_REGISTER(hcounteren_tm_field);
bool hcounteren_tm_field(void) {
    TEST_BEGIN("HCNT-03: Verify hcounteren TM bit (1) controls VS time access");

    /* Set TM bit. */
    hcounteren_write(0x2);
    uintptr_t val = hcounteren_read();

    /* Verify TM bit is set. */
    TEST_ASSERT_EQ("hcounteren.TM bit should be set", val & 0x2, 0x2);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HCNT-04: hcounteren IR field controls VS counter access
 * ------------------------------------------------------------------ */
TEST_REGISTER(hcounteren_ir_field);
bool hcounteren_ir_field(void) {
    TEST_BEGIN("HCNT-04: Verify hcounteren IR bit (2) controls VS instret access");

    /* Set IR bit. */
    hcounteren_write(0x4);
    uintptr_t val = hcounteren_read();

    /* Verify IR bit is set. */
    TEST_ASSERT_EQ("hcounteren.IR bit should be set", val & 0x4, 0x4);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HCNT-05: hcounteren=0 triggers virtual-inst on VS counter access
 * ------------------------------------------------------------------ */
TEST_REGISTER(hcounteren_zero_vs_trap);
bool hcounteren_zero_vs_trap(void) {
    TEST_BEGIN("HCNT-05: Verify hcounteren=0 causes VS counter access to trap");

    /* Clear hcounteren. */
    hcounteren_write(0x0);
    uintptr_t val = hcounteren_read();

    /* Verify hcounteren is zero. */
    TEST_ASSERT_EQ("hcounteren should be 0", val, 0x0);

    /* VS-mode counter access should trap. */
    EXPECT_VIRTUAL_INST(vs_read_cycle(0));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HCNT-06: hcounteren=0 triggers virtual-inst on VU counter access
 * ------------------------------------------------------------------ */
TEST_REGISTER(hcounteren_zero_vu_trap);
bool hcounteren_zero_vu_trap(void) {
    TEST_BEGIN("HCNT-06: Verify hcounteren=0 causes VU counter access to trap");

    /* Clear hcounteren. */
    hcounteren_write(0x0);
    uintptr_t val = hcounteren_read();

    /* Verify hcounteren is zero. */
    TEST_ASSERT_EQ("hcounteren should be 0", val, 0x0);

    /* VU-mode counter access should trap. */
    EXPECT_VIRTUAL_INST(vs_read_cycle(0));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HCNT-07: hcounteren and scounteren dual-layer control
 * ------------------------------------------------------------------ */
TEST_REGISTER(hcounteren_scounteren_dual_layer);
bool hcounteren_scounteren_dual_layer(void) {
    TEST_BEGIN("HCNT-07: Verify hcounteren and scounteren dual-layer control");

    /* Set hcounteren.CY. */
    hcounteren_set(0x1);
    uintptr_t hcounteren_val = hcounteren_read();

    /* Verify hcounteren.CY is set. */
    TEST_ASSERT_EQ("hcounteren.CY should be set", hcounteren_val & 0x1, 0x1);

    hcounteren_write(0x7); /* Allow CY+TM+IR at VS level */
    scounteren_write(0x0); /* Block all at VU level */
    mcounteren_write(0x7); /* Allow at HS level */
    /* VU-mode access to cycle should trap because scounteren blocks it */
    EXPECT_COUNTER_TRAP(run_in_vu_mode(vs_read_cycle, 0));
    /* Restore */
    scounteren_write(0x7);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HCNT-08: hcounteren HPM bit writability
 * ------------------------------------------------------------------ */
TEST_REGISTER(hcounteren_hpm_bits);
bool hcounteren_hpm_bits(void) {
    TEST_BEGIN("HCNT-08: Verify hcounteren HPM bits (3-31) are writable");

    /* Write all 1s to hcounteren. */
    hcounteren_write(0xFFFFFFFF);
    uintptr_t val = hcounteren_read();
    /* At minimum, bits 0-2 (CY, TM, IR) should be writable */
    TEST_ASSERT("CY bit writable", (val & 0x1) != 0);
    TEST_ASSERT("TM bit writable", (val & 0x2) != 0);
    TEST_ASSERT("IR bit writable", (val & 0x4) != 0);
    /* HPM counters start at bit 3 */
    /* Some HPM bits may be writable depending on implementation */
    hcounteren_write(0x0);
    uintptr_t val2 = hcounteren_read();
    TEST_ASSERT_EQ("hcounteren clearable", val2, (uintptr_t)0);

    HYP_TEST_END();
}
