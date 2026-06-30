/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_sscsrind_stateen.c - Group 4.1: State-Enable access control (S-mode only)
 *
 * Verifies mstateen0[60] access control for siselect/sireg* from S-mode.
 *
 * H-extension dependent tests (STA-05~10) have been migrated to
 * Hypervisor_Sscsrind/ as HCROSS-SSCSRIND-22~27.
 */

/* SSCSRIND-STA-01: mstateen0[60]=0 blocks S-mode read siselect */
TEST_REGISTER(test_sscsrind_sta_01);
bool test_sscsrind_sta_01(void)
{
    TEST_BEGIN("SSCSRIND-STA-01: mstateen0[60]=0 blocks S-mode siselect");

    if (!platform_has_sscsrind())
    {
        TEST_SKIP("Sscsrind not implemented");
    }
    if (!platform_has_smstateen())
    {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    TEST_SMODE_BLOCKED("S-mode siselect read blocked (CSRIND=0)",
                       siselect_read());

    mstateen0_write(orig);
    TEST_END();
}

/* SSCSRIND-STA-02: mstateen0[60]=0 blocks S-mode read sireg */
TEST_REGISTER(test_sscsrind_sta_02);
bool test_sscsrind_sta_02(void)
{
    TEST_BEGIN("SSCSRIND-STA-02: mstateen0[60]=0 blocks S-mode sireg");

    if (!platform_has_sscsrind())
    {
        TEST_SKIP("Sscsrind not implemented");
    }
    if (!platform_has_smstateen())
    {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    TEST_SMODE_BLOCKED("S-mode sireg read blocked (CSRIND=0)",
                       sireg_read());

    mstateen0_write(orig);
    TEST_END();
}

/* SSCSRIND-STA-03: mstateen0[60]=0 blocks S-mode write siselect */
TEST_REGISTER(test_sscsrind_sta_03);
bool test_sscsrind_sta_03(void)
{
    TEST_BEGIN("SSCSRIND-STA-03: mstateen0[60]=0 blocks S-mode siselect write");

    if (!platform_has_sscsrind())
    {
        TEST_SKIP("Sscsrind not implemented");
    }
    if (!platform_has_smstateen())
    {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    TEST_SMODE_BLOCKED("S-mode siselect write blocked (CSRIND=0)",
                       siselect_write(0x42));

    mstateen0_write(orig);
    TEST_END();
}

/* SSCSRIND-STA-04: mstateen0[60]=1 allows S-mode access */
TEST_REGISTER(test_sscsrind_sta_04);
bool test_sscsrind_sta_04(void)
{
    TEST_BEGIN("SSCSRIND-STA-04: mstateen0[60]=1 allows S-mode access");

    if (!platform_has_sscsrind())
    {
        TEST_SKIP("Sscsrind not implemented");
    }
    if (!platform_has_smstateen())
    {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_set(MSTATEEN0_CSRIND);

    TEST_SMODE_ALLOWED("S-mode siselect access allowed (CSRIND=1)",
                       siselect_write(0));

    mstateen0_write(orig);
    TEST_END();
}
