/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_menvcfg_dte.c - Group 4: menvcfg.DTE control tests
 */

/* DTE-01: menvcfg.DTE bit position (bit 59) */
TEST_REGISTER(test_menvcfg_dte_01);
bool test_menvcfg_dte_01(void)
{
    TEST_BEGIN("DTE-01: menvcfg.DTE bit position (bit 59)");

    /* Read menvcfg and check DTE bit position */
    uint64_t menvcfg;
    asm volatile("csrr %0, menvcfg" : "=r"(menvcfg));

    /* Try to set DTE */
    asm volatile("csrs menvcfg, %0" :: "r"(MENVCFG_DTE_BIT));
    asm volatile("csrr %0, menvcfg" : "=r"(menvcfg));

    bool dte_set = (menvcfg & MENVCFG_DTE_BIT) != 0;

    /* Clear DTE */
    asm volatile("csrc menvcfg, %0" :: "r"(MENVCFG_DTE_BIT));

    TEST_ASSERT("DTE at bit 59", dte_set);
    TEST_END();
}

/* DTE-02: menvcfg.DTE read/write */
TEST_REGISTER(test_menvcfg_dte_02);
bool test_menvcfg_dte_02(void)
{
    TEST_BEGIN("DTE-02: menvcfg.DTE read/write");

    uint64_t menvcfg;

    /* Set DTE=1 */
    asm volatile("csrs menvcfg, %0" :: "r"(MENVCFG_DTE_BIT));
    asm volatile("csrr %0, menvcfg" : "=r"(menvcfg));
    bool val1 = (menvcfg & MENVCFG_DTE_BIT) != 0;

    /* Clear DTE=0 */
    asm volatile("csrc menvcfg, %0" :: "r"(MENVCFG_DTE_BIT));
    asm volatile("csrr %0, menvcfg" : "=r"(menvcfg));
    bool val2 = (menvcfg & MENVCFG_DTE_BIT) != 0;

    TEST_ASSERT("DTE write 1", val1);
    TEST_ASSERT("DTE write 0", !val2);
    TEST_END();
}

/* DTE-03: DTE=0 disables SDT */
TEST_REGISTER(test_menvcfg_dte_03);
bool test_menvcfg_dte_03(void)
{
    TEST_BEGIN("DTE-03: DTE=0 disables SDT");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Clear DTE */
    asm volatile("csrc menvcfg, %0" :: "r"(MENVCFG_DTE_BIT));

    /* Try to set SDT */
    set_sdt();
    bool sdt_val = get_sdt();
    clear_sdt();

    TEST_ASSERT("SDT disabled when DTE=0", !sdt_val);
    TEST_END();
}

/* DTE-04: DTE=1 enables SDT */
TEST_REGISTER(test_menvcfg_dte_04);
bool test_menvcfg_dte_04(void)
{
    TEST_BEGIN("DTE-04: DTE=1 enables SDT");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Set DTE=1 */
    asm volatile("csrs menvcfg, %0" :: "r"(MENVCFG_DTE_BIT));

    /* Try to set SDT */
    set_sdt();
    bool sdt_val = get_sdt();

    /* Clean up */
    clear_sdt();
    asm volatile("csrc menvcfg, %0" :: "r"(MENVCFG_DTE_BIT));

    TEST_ASSERT("SDT enabled when DTE=1", sdt_val);
    TEST_END();
}
