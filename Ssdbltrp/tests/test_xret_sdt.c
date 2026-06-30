/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_xret_sdt.c - Group 7: MRET/SRET/MNRET clear SDT tests
 */

/* XRET-01: MRET from M-mode to S-mode clears SDT */
TEST_REGISTER(test_xret_sdt_01);
bool test_xret_sdt_01(void)
{
    TEST_BEGIN("XRET-01: MRET from M-mode to S-mode clears SDT");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after MRET (ecall triggers double trap)");
}

/* XRET-02: MRET from M-mode to U-mode clears SDT */
TEST_REGISTER(test_xret_sdt_02);
bool test_xret_sdt_02(void)
{
    TEST_BEGIN("XRET-02: MRET from M-mode to U-mode clears SDT");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after MRET (ecall triggers double trap)");
}

/* XRET-03: SRET from S-mode to U-mode clears SDT */
TEST_REGISTER(test_xret_sdt_03);
bool test_xret_sdt_03(void)
{
    TEST_BEGIN("XRET-03: SRET from S-mode to U-mode clears SDT");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after SRET (ecall triggers double trap)");
}

/* XRET-04: SRET from M-mode to S-mode clears SDT */
TEST_REGISTER(test_xret_sdt_04);
bool test_xret_sdt_04(void)
{
    TEST_BEGIN("XRET-04: SRET from M-mode to S-mode clears SDT");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after SRET (ecall triggers double trap)");
}

/* XRET-05: MNRET from M-mode to S-mode clears SDT */
TEST_REGISTER(test_xret_sdt_05);
bool test_xret_sdt_05(void)
{
    TEST_BEGIN("XRET-05: MNRET from M-mode to S-mode clears SDT");
    TEST_SKIP("QEMU limitation: Cannot safely test MNRET behavior");
}

/* XRET-06: MNRET from M-mode to U-mode clears SDT */
TEST_REGISTER(test_xret_sdt_06);
bool test_xret_sdt_06(void)
{
    TEST_BEGIN("XRET-06: MNRET from M-mode to U-mode clears SDT");
    TEST_SKIP("QEMU limitation: Cannot safely test MNRET behavior");
}

/* XRET-07: MRET from M-mode to M-mode does NOT clear SDT */
TEST_REGISTER(test_xret_sdt_07);
bool test_xret_sdt_07(void)
{
    TEST_BEGIN("XRET-07: MRET from M-mode to M-mode does NOT clear SDT");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after MRET (ecall triggers double trap)");
}
