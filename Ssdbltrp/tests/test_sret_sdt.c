/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_sret_sdt.c - Group 3: SRET clears SDT tests
 */

/* SRET-01: SRET from S-mode to U-mode clears SDT */
TEST_REGISTER(test_sret_sdt_01);
bool test_sret_sdt_01(void)
{
    TEST_BEGIN("SRET-01: SRET from S-mode to U-mode clears SDT");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after SRET (ecall triggers double trap)");
}

/* SRET-02: SRET from S-mode to S-mode clears SDT */
TEST_REGISTER(test_sret_sdt_02);
bool test_sret_sdt_02(void)
{
    TEST_BEGIN("SRET-02: SRET from S-mode to S-mode clears SDT");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after SRET (ecall triggers double trap)");
}

/* SRET-03: SRET from M-mode to S-mode clears SDT */
TEST_REGISTER(test_sret_sdt_03);
bool test_sret_sdt_03(void)
{
    TEST_BEGIN("SRET-03: SRET from M-mode to S-mode clears SDT");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after SRET (ecall triggers double trap)");
}
