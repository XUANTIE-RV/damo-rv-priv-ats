/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_sdt_sie_mutex.c - Group 5: SDT/SIE mutex tests
 */

/* SDT-SIE-01: SDT=1 clears SIE */
TEST_REGISTER(test_sdt_sie_mutex_01);
bool test_sdt_sie_mutex_01(void)
{
    TEST_BEGIN("SDT-SIE-01: SDT=1 clears SIE");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Enable SDT by setting DTE=1 */
    set_dte();

    /* Set SIE=1 */
    set_sie();
    bool sie_before = get_sie();

    /* Set SDT=1 */
    set_sdt();

    /* Check if SIE was cleared */
    bool sie_after = get_sie();

    /* Clean up */
    clear_sdt();
    clear_dte();

    TEST_ASSERT("SIE was 1 before", sie_before);
    TEST_ASSERT("SIE cleared by SDT=1", !sie_after);
    TEST_END();
}

/* SDT-SIE-02: SDT=1 prevents SIE=1 */
TEST_REGISTER(test_sdt_sie_mutex_02);
bool test_sdt_sie_mutex_02(void)
{
    TEST_BEGIN("SDT-SIE-02: SDT=1 prevents SIE=1");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Enable SDT by setting DTE=1 */
    set_dte();

    /* Set SDT=1 */
    set_sdt();

    /* Try to set SIE=1 */
    set_sie();

    /* Check if SIE is still 0 */
    bool sie_val = get_sie();

    /* Clean up */
    clear_sdt();
    clear_dte();

    TEST_ASSERT("SIE stays 0 when SDT=1", !sie_val);
    TEST_END();
}

/* SDT-SIE-03: SDT=0 allows SIE=1 */
TEST_REGISTER(test_sdt_sie_mutex_03);
bool test_sdt_sie_mutex_03(void)
{
    TEST_BEGIN("SDT-SIE-03: SDT=0 allows SIE=1");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Enable SDT by setting DTE=1 */
    set_dte();

    /* Clear SDT */
    clear_sdt();

    /* Set SIE=1 */
    set_sie();

    /* Check if SIE is 1 */
    bool sie_val = get_sie();

    /* Clean up */
    clear_sie();
    clear_dte();

    TEST_ASSERT("SIE=1 when SDT=0", sie_val);
    TEST_END();
}

/* SDT-SIE-04: SDT=1 with SIE=1 clears SIE */
TEST_REGISTER(test_sdt_sie_mutex_04);
bool test_sdt_sie_mutex_04(void)
{
    TEST_BEGIN("SDT-SIE-04: SDT=1 with SIE=1 clears SIE");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Enable SDT by setting DTE=1 */
    set_dte();

    /* Clear SDT */
    clear_sdt();

    /* Set SIE=1 */
    set_sie();
    bool sie_before = get_sie();

    /* Set SDT=1 (should clear SIE to 0) */
    set_sdt();

    /* Check if SIE was cleared */
    bool sie_after = get_sie();

    /* Clean up */
    clear_sdt();
    clear_dte();

    TEST_ASSERT("SIE was 1 before SDT write", sie_before);
    TEST_ASSERT("SDT=1 clears SIE", !sie_after);
    TEST_END();
}

/* SDT-SIE-05: SIE=1 allows SDT=0 */
TEST_REGISTER(test_sdt_sie_mutex_05);
bool test_sdt_sie_mutex_05(void)
{
    TEST_BEGIN("SDT-SIE-05: SIE=1 allows SDT=0");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Enable SDT by setting DTE=1 */
    set_dte();

    /* Set SIE=1 */
    set_sie();

    /* Clear SDT (should be allowed) */
    clear_sdt();

    /* Check if SDT is 0 */
    bool sdt_val = get_sdt();

    /* Clean up */
    clear_sie();
    clear_dte();

    TEST_ASSERT("SDT=0 allowed when SIE=1", !sdt_val);
    TEST_END();
}
