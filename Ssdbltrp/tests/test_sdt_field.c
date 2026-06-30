/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_sdt_field.c - Group 1: sstatus.SDT field tests
 */

/* SDT-01: sstatus.SDT WARL field */
TEST_REGISTER(test_sdt_field_01);
bool test_sdt_field_01(void)
{
    TEST_BEGIN("SDT-01: sstatus.SDT WARL field");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Enable SDT by setting DTE=1 */
    set_dte();

    /* Try to write SDT=1 */
    set_sdt();
    bool sdt_val = get_sdt();
    clear_sdt();

    /* Cleanup */
    clear_dte();

    TEST_ASSERT("SDT write", sdt_val);
    TEST_END();
}

/* SDT-02: sstatus.SDT read/write */
TEST_REGISTER(test_sdt_field_02);
bool test_sdt_field_02(void)
{
    TEST_BEGIN("SDT-02: sstatus.SDT read/write");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Enable SDT by setting DTE=1 */
    set_dte();

    /* Write SDT=1 */
    set_sdt();
    bool val1 = get_sdt();

    /* Write SDT=0 */
    clear_sdt();
    bool val2 = get_sdt();

    /* Cleanup */
    clear_dte();

    TEST_ASSERT("SDT write 1", val1);
    TEST_ASSERT("SDT write 0", !val2);
    TEST_END();
}

/* SDT-03: sstatus.SDT bit position (bit 24) */
TEST_REGISTER(test_sdt_field_03);
bool test_sdt_field_03(void)
{
    TEST_BEGIN("SDT-03: sstatus.SDT bit position (bit 24)");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Enable SDT by setting DTE=1 */
    set_dte();

    /* Read sstatus and check SDT bit position */
    set_sdt();
    uint64_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    clear_sdt();

    /* Cleanup */
    clear_dte();

    TEST_ASSERT("SDT at bit 24", (sstatus & SSTATUS_SDT_BIT) != 0);
    TEST_END();
}
