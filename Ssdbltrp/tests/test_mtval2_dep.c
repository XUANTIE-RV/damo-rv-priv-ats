/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_mtval2_dep.c - Group 9: mtval2 dependency tests
 */

/* MTVAL2-01: mtval2 CSR exists */
TEST_REGISTER(test_mtval2_01);
bool test_mtval2_01(void)
{
    TEST_BEGIN("MTVAL2-01: mtval2 CSR exists");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Try to read mtval2 */
    uint64_t mtval2;
    asm volatile("csrr %0, mtval2" : "=r"(mtval2));

    /* If we reach here without trap, mtval2 exists */
    TEST_END();
}

/* MTVAL2-02: mtval2 read/write */
TEST_REGISTER(test_mtval2_02);
bool test_mtval2_02(void)
{
    TEST_BEGIN("MTVAL2-02: mtval2 read/write");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    uint64_t test_val = 0x12345678ABCDEF00ULL;
    uint64_t mtval2;

    /* Write test value */
    asm volatile("csrw mtval2, %0" :: "r"(test_val));

    /* Read back */
    asm volatile("csrr %0, mtval2" : "=r"(mtval2));

    /* Clean up */
    asm volatile("csrw mtval2, %0" :: "r"(0ULL));

    TEST_ASSERT_EQ("mtval2 read/write", mtval2, test_val);
    TEST_END();
}

/* MTVAL2-03: mtval2 holds original trap cause on double trap */
TEST_REGISTER(test_mtval2_03);
bool test_mtval2_03(void)
{
    TEST_BEGIN("MTVAL2-03: mtval2 holds original trap cause on double trap");
    TEST_SKIP("QEMU limitation: Cannot safely test double trap behavior");
}
