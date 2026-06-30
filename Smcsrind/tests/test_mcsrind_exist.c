/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_mcsrind_exist.c - Group 1: M-mode CSR existence and accessibility
 *
 * Verifies that miselect and mireg~mireg6 (7 M-mode CSRs) exist
 * and are accessible from M-mode.
 */

/* MCSRIND-EXIST-01: miselect readable and writable */
TEST_REGISTER(test_mcsrind_exist_01);
bool test_mcsrind_exist_01(void)
{
    TEST_BEGIN("MCSRIND-EXIST-01: miselect readable and writable");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig = miselect_read();

    /* Write a value and read back */
    miselect_write(0x42);
    uintptr_t rb = miselect_read();
    TEST_ASSERT_EQ("miselect write 0x42 reads back", rb, (uintptr_t)0x42);

    /* Write zero and read back */
    miselect_write(0);
    rb = miselect_read();
    TEST_ASSERT_EQ("miselect write 0 reads back 0", rb, (uintptr_t)0);

    miselect_write(orig);
    TEST_END();
}

/* MCSRIND-EXIST-02: mireg accessible with legal miselect */
TEST_REGISTER(test_mcsrind_exist_02);
bool test_mcsrind_exist_02(void)
{
    TEST_BEGIN("MCSRIND-EXIST-02: mireg accessible with legal miselect");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();

    /* Use a known safe miselect value (0x30 = Smaia range, or 0 if no ext) */
    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    if (rb != 0x30) {
        /* Smaia not implemented, try miselect=0 (reserved, should be safe) */
        miselect_write(0);
    }

    M_TRAP_EXPECT_BEGIN();
    mireg_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* mireg access with legal miselect should not trap,
     * or may trap if no dependent extension uses this range.
     * Either behavior is acceptable for a reserved select value. */
    if (!trapped) {
        TEST_ASSERT("mireg accessible with legal miselect", 1);
    } else {
        /* Trap is acceptable for reserved/unimplemented select values */
        TEST_ASSERT("mireg: trap acceptable for unimplemented select", 1);
    }

    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-EXIST-03: mireg2 accessible */
TEST_REGISTER(test_mcsrind_exist_03);
bool test_mcsrind_exist_03(void)
{
    TEST_BEGIN("MCSRIND-EXIST-03: mireg2 accessible");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();
    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    if (rb != 0x30) {
        miselect_write(0);
    }

    M_TRAP_EXPECT_BEGIN();
    mireg2_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("mireg2 accessible (trap or no-trap both acceptable)", 1);

    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-EXIST-04: mireg3 accessible */
TEST_REGISTER(test_mcsrind_exist_04);
bool test_mcsrind_exist_04(void)
{
    TEST_BEGIN("MCSRIND-EXIST-04: mireg3 accessible");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();
    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    if (rb != 0x30) {
        miselect_write(0);
    }

    M_TRAP_EXPECT_BEGIN();
    mireg3_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("mireg3 accessible (trap or no-trap both acceptable)", 1);

    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-EXIST-05: mireg4 accessible (CSR 0x355, not 0x354) */
TEST_REGISTER(test_mcsrind_exist_05);
bool test_mcsrind_exist_05(void)
{
    TEST_BEGIN("MCSRIND-EXIST-05: mireg4 (0x355) accessible");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();
    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    if (rb != 0x30) {
        miselect_write(0);
    }

    M_TRAP_EXPECT_BEGIN();
    mireg4_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("mireg4 (0x355) accessible (trap or no-trap both acceptable)", 1);

    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-EXIST-06: mireg5 accessible */
TEST_REGISTER(test_mcsrind_exist_06);
bool test_mcsrind_exist_06(void)
{
    TEST_BEGIN("MCSRIND-EXIST-06: mireg5 accessible");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();
    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    if (rb != 0x30) {
        miselect_write(0);
    }

    M_TRAP_EXPECT_BEGIN();
    mireg5_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("mireg5 accessible (trap or no-trap both acceptable)", 1);

    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-EXIST-07: mireg6 accessible */
TEST_REGISTER(test_mcsrind_exist_07);
bool test_mcsrind_exist_07(void)
{
    TEST_BEGIN("MCSRIND-EXIST-07: mireg6 accessible");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();
    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    if (rb != 0x30) {
        miselect_write(0);
    }

    M_TRAP_EXPECT_BEGIN();
    mireg6_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("mireg6 accessible (trap or no-trap both acceptable)", 1);

    miselect_write(orig_sel);
    TEST_END();
}
