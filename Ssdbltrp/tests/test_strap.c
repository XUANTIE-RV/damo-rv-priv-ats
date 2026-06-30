/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_strap.c - Group 2: S-mode trap delivery tests
 *
 * Note: Most tests skipped due to double trap limitations
 */

/* STRAP-01: SDT=0 normal trap delivery sets SDT=1 */
TEST_REGISTER(test_strap_01);
bool test_strap_01(void)
{
    TEST_BEGIN("STRAP-01: SDT=0 normal trap delivery sets SDT=1");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after trap (ecall triggers double trap)");
}

/* STRAP-02: SDT=1 trap is unexpected trap */
TEST_REGISTER(test_strap_02);
bool test_strap_02(void)
{
    TEST_BEGIN("STRAP-02: SDT=1 trap is unexpected trap");
    TEST_SKIP("QEMU limitation: Cannot safely test double trap behavior");
}

/* STRAP-03: Trap delivery from U-mode to S-mode */
TEST_REGISTER(test_strap_03);
bool test_strap_03(void)
{
    TEST_BEGIN("STRAP-03: Trap delivery from U-mode to S-mode");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after trap (ecall triggers double trap)");
}

/* STRAP-04: Trap delivery from S-mode to S-mode */
TEST_REGISTER(test_strap_04);
bool test_strap_04(void)
{
    TEST_BEGIN("STRAP-04: Trap delivery from S-mode to S-mode");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after trap (ecall triggers double trap)");
}

/* STRAP-05: Trap delivery with interrupt */
TEST_REGISTER(test_strap_05);
bool test_strap_05(void)
{
    TEST_BEGIN("STRAP-05: Trap delivery with interrupt");
    TEST_SKIP("QEMU limitation: Cannot return to M-mode after trap (ecall triggers double trap)");
}
