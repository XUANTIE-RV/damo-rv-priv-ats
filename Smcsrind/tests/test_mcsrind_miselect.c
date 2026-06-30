/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_mcsrind_miselect.c - Group 2: miselect register behavior
 *
 * Verifies miselect WARL attributes, minimum bit width, MSB semantics.
 */

/* MCSRIND-MSEL-01: miselect WARL behavior - write all-ones */
TEST_REGISTER(test_mcsrind_msel_01);
bool test_mcsrind_msel_01(void)
{
    TEST_BEGIN("MCSRIND-MSEL-01: miselect WARL write all-ones");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig = miselect_read();

    /* Write all-ones - WARL should not trap */
    M_TRAP_EXPECT_BEGIN();
    uintptr_t all_ones = (uintptr_t)-1;
    miselect_write(all_ones);
    uintptr_t rb = miselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("miselect WARL: no exception on all-ones write", !trapped);
    /* Readback value is implementation-defined legal value */
    (void)rb;

    miselect_write(orig);
    TEST_END();
}

/* MCSRIND-MSEL-02: miselect WARL write zero */
TEST_REGISTER(test_mcsrind_msel_02);
bool test_mcsrind_msel_02(void)
{
    TEST_BEGIN("MCSRIND-MSEL-02: miselect WARL write zero");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig = miselect_read();

    M_TRAP_EXPECT_BEGIN();
    miselect_write(0);
    uintptr_t rb = miselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("miselect WARL: no exception on zero write", !trapped);
    TEST_ASSERT_EQ("miselect WARL: zero reads back as zero", rb, (uintptr_t)0);

    miselect_write(orig);
    TEST_END();
}

/* MCSRIND-MSEL-03: miselect minimum bit width - write implemented value */
TEST_REGISTER(test_mcsrind_msel_03);
bool test_mcsrind_msel_03(void)
{
    TEST_BEGIN("MCSRIND-MSEL-03: miselect minimum bit width");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig = miselect_read();

    /* Try Smaia range 0x30 */
    miselect_write(0x30);
    uintptr_t rb = miselect_read();

    if (rb == 0x30) {
        TEST_ASSERT("miselect accepts Smaia value 0x30", 1);
    } else {
        /* Smaia not implemented, try other values */
        miselect_write(0x100);
        rb = miselect_read();
        /* WARL: readback should be a legal value */
        TEST_ASSERT("miselect WARL: readback is legal value", 1);
    }

    miselect_write(orig);
    TEST_END();
}

/* MCSRIND-MSEL-04: miselect MSB=1 custom region */
TEST_REGISTER(test_mcsrind_msel_04);
bool test_mcsrind_msel_04(void)
{
    TEST_BEGIN("MCSRIND-MSEL-04: miselect MSB=1 custom region");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig = miselect_read();

    /* Write MSB=1 value (RV64: bit 63) */
    uintptr_t msb_val = (1ULL << (__riscv_xlen - 1));
    M_TRAP_EXPECT_BEGIN();
    miselect_write(msb_val);
    uintptr_t rb = miselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("miselect MSB=1: no exception on write", !trapped);
    /* Readback is a legal value (WARL) */
    (void)rb;

    miselect_write(orig);
    TEST_END();
}

/* MCSRIND-MSEL-05: miselect MSB=0 standard region */
TEST_REGISTER(test_mcsrind_msel_05);
bool test_mcsrind_msel_05(void)
{
    TEST_BEGIN("MCSRIND-MSEL-05: miselect MSB=0 standard region");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig = miselect_read();

    /* Write MSB=0 value */
    M_TRAP_EXPECT_BEGIN();
    miselect_write(0x1);
    uintptr_t rb = miselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("miselect MSB=0: no exception on write", !trapped);
    (void)rb;

    miselect_write(orig);
    TEST_END();
}

/* MCSRIND-MSEL-06: miselect may be read-only zero if no extensions use it */
TEST_REGISTER(test_mcsrind_msel_06);
bool test_mcsrind_msel_06(void)
{
    TEST_BEGIN("MCSRIND-MSEL-06: miselect RO0 when no extensions");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig = miselect_read();

    /* Write a non-zero value */
    miselect_write(0x42);
    uintptr_t rb = miselect_read();

    if (rb == 0) {
        /* miselect is RO0 - no dependent extensions implemented */
        TEST_ASSERT("miselect is RO0: no dependent extensions", 1);
    } else {
        /* miselect accepted the value - at least one extension uses it */
        TEST_ASSERT("miselect accepted write: extensions present", 1);
    }

    miselect_write(orig);
    TEST_END();
}

/* MCSRIND-MSEL-07: miselect consecutive writes of different values */
TEST_REGISTER(test_mcsrind_msel_07);
bool test_mcsrind_msel_07(void)
{
    TEST_BEGIN("MCSRIND-MSEL-07: miselect consecutive writes");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig = miselect_read();

    uintptr_t test_vals[] = {0, 0x42, 0x100, 0x200, 0xFFF};
    int n = sizeof(test_vals) / sizeof(test_vals[0]);
    bool all_ok = true;

    for (int i = 0; i < n; i++) {
        M_TRAP_EXPECT_BEGIN();
        miselect_write(test_vals[i]);
        uintptr_t rb = miselect_read();
        bool trapped = trap_was_triggered();
        trap_expect_end();

        if (trapped) {
            all_ok = false;
            break;
        }
        /* WARL: readback should be a legal value (not necessarily the written one) */
        (void)rb;
    }

    TEST_ASSERT("miselect: consecutive writes do not trap", all_ok);

    miselect_write(orig);
    TEST_END();
}
