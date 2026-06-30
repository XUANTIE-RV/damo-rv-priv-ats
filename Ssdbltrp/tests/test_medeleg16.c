/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_medeleg16.c - Group 8: medeleg[16] read-only zero tests
 */

/* MDEL-01: medeleg[16] is read-only zero */
TEST_REGISTER(test_medeleg16_01);
bool test_medeleg16_01(void)
{
    TEST_BEGIN("MDEL-01: medeleg[16] is read-only zero");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Try to set medeleg[16] */
    asm volatile("csrs medeleg, %0" :: "r"(MEDELEG_DOUBLE_TRAP));

    /* Read back medeleg */
    uint64_t medeleg;
    asm volatile("csrr %0, medeleg" : "=r"(medeleg));

    /* Clean up */
    asm volatile("csrw medeleg, %0" :: "r"(0ULL));

    TEST_ASSERT("medeleg[16] read-only zero", (medeleg & MEDELEG_DOUBLE_TRAP) == 0);
    TEST_END();
}

/* MDEL-02: medeleg all-1 write, bit 16 still zero */
TEST_REGISTER(test_medeleg16_02);
bool test_medeleg16_02(void)
{
    TEST_BEGIN("MDEL-02: medeleg all-1 write, bit 16 still zero");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Write all 1s to medeleg */
    asm volatile("csrw medeleg, %0" :: "r"(0xFFFFFFFFFFFFFFFFULL));

    /* Read back medeleg */
    uint64_t medeleg;
    asm volatile("csrr %0, medeleg" : "=r"(medeleg));

    printf("[INFO] medeleg after all-1 write = 0x%lx\n", (unsigned long)medeleg);

    /* Clean up: restore medeleg to 0 */
    asm volatile("csrw medeleg, %0" :: "r"(0ULL));

    TEST_ASSERT("medeleg[16] zero after all-1 write", (medeleg & MEDELEG_DOUBLE_TRAP) == 0);
    TEST_END();
}

/* MDEL-03: Double trap not delegated to S-mode */
TEST_REGISTER(test_medeleg16_03);
bool test_medeleg16_03(void)
{
    TEST_BEGIN("MDEL-03: Double trap not delegated to S-mode");
    TEST_SKIP("QEMU limitation: Cannot safely test double trap delegation");
}
