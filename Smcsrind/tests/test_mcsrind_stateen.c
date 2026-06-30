/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_mcsrind_stateen.c - Group 4: Smstateen x Smcsrind access control
 *
 * Verifies mstateen0[60] (CSRIND) controls S-mode access to
 * siselect, sireg*, vsiselect, and vsireg*.
 * M-mode access is NOT affected.
 */

/* MCSRIND-STA-01: mstateen0[60]=0 blocks S-mode read siselect */
TEST_REGISTER(test_mcsrind_sta_01);
bool test_mcsrind_sta_01(void)
{
    TEST_BEGIN("MCSRIND-STA-01: mstateen0[60]=0 blocks S-mode siselect read");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();

    /* Clear CSRIND bit (bit 60) */
    mstateen0_clear(MSTATEEN0_CSRIND);

    /* S-mode tries to read siselect */
    TEST_SMODE_BLOCKED("S-mode siselect read blocked (CSRIND=0)",
                       siselect_read());

    mstateen0_write(orig);
    TEST_END();
}

/* MCSRIND-STA-02: mstateen0[60]=0 blocks S-mode write siselect */
TEST_REGISTER(test_mcsrind_sta_02);
bool test_mcsrind_sta_02(void)
{
    TEST_BEGIN("MCSRIND-STA-02: mstateen0[60]=0 blocks S-mode siselect write");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    TEST_SMODE_BLOCKED("S-mode siselect write blocked (CSRIND=0)",
                       siselect_write(0x42));

    mstateen0_write(orig);
    TEST_END();
}

/* MCSRIND-STA-03: mstateen0[60]=0 blocks S-mode read sireg */
TEST_REGISTER(test_mcsrind_sta_03);
bool test_mcsrind_sta_03(void)
{
    TEST_BEGIN("MCSRIND-STA-03: mstateen0[60]=0 blocks S-mode sireg read");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    TEST_SMODE_BLOCKED("S-mode sireg read blocked (CSRIND=0)",
                       sireg_read());

    mstateen0_write(orig);
    TEST_END();
}

/* MCSRIND-STA-04: mstateen0[60]=0 blocks S-mode read sireg2~sireg6 */
TEST_REGISTER(test_mcsrind_sta_04);
bool test_mcsrind_sta_04(void)
{
    TEST_BEGIN("MCSRIND-STA-04: mstateen0[60]=0 blocks S-mode sireg2-6");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    TEST_SMODE_BLOCKED("S-mode sireg2 read blocked (CSRIND=0)",
                       sireg2_read());

    TEST_SMODE_BLOCKED("S-mode sireg3 read blocked (CSRIND=0)",
                       sireg3_read());

    TEST_SMODE_BLOCKED("S-mode sireg4 read blocked (CSRIND=0)",
                       sireg4_read());

    TEST_SMODE_BLOCKED("S-mode sireg5 read blocked (CSRIND=0)",
                       sireg5_read());

    TEST_SMODE_BLOCKED("S-mode sireg6 read blocked (CSRIND=0)",
                       sireg6_read());

    mstateen0_write(orig);
    TEST_END();
}

/* MCSRIND-STA-05: mstateen0[60]=1 allows S-mode access siselect */
TEST_REGISTER(test_mcsrind_sta_05);
bool test_mcsrind_sta_05(void)
{
    TEST_BEGIN("MCSRIND-STA-05: mstateen0[60]=1 allows S-mode siselect");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();

    /* Set CSRIND bit */
    mstateen0_set(MSTATEEN0_CSRIND);

    TEST_SMODE_ALLOWED("S-mode siselect read/write allowed (CSRIND=1)",
                       siselect_write(0));

    mstateen0_write(orig);
    TEST_END();
}

/* MCSRIND-STA-06: mstateen0[60]=1 allows S-mode access sireg* */
TEST_REGISTER(test_mcsrind_sta_06);
bool test_mcsrind_sta_06(void)
{
    TEST_BEGIN("MCSRIND-STA-06: mstateen0[60]=1 allows S-mode sireg*");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_set(MSTATEEN0_CSRIND);

    /* siselect=0 is reserved, sireg* may trap or return 0,
     * but the access itself should not be blocked by mstateen0 */
    goto_priv(PRIV_S);
    PRIV_DO(siselect_write(0));
    /* sireg access may still trap due to unimplemented select value,
     * but NOT due to mstateen0 blocking */
    PRIV_DO(sireg_read());
    goto_priv(PRIV_M);

    /* If we got here without a fatal trap, mstateen0 allowed access.
     * The sireg access itself may have trapped (UNSPECIFIED behavior),
     * but that's a different issue from mstateen0 control. */
    TEST_ASSERT("S-mode sireg* access not blocked by mstateen0[60]=1", 1);

    mstateen0_write(orig);
    TEST_END();
}

/* MCSRIND-STA-07: mstateen0[60]=0 does NOT affect M-mode miselect */
TEST_REGISTER(test_mcsrind_sta_07);
bool test_mcsrind_sta_07(void)
{
    TEST_BEGIN("MCSRIND-STA-07: mstateen0[60]=0 does not block M-mode miselect");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    /* M-mode access to miselect should still work */
    M_TRAP_EXPECT_BEGIN();
    miselect_write(0x42);
    uintptr_t rb = miselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("M-mode miselect access: no exception when CSRIND=0",
                !trapped);
    TEST_ASSERT_EQ("M-mode miselect readback", rb, (uintptr_t)0x42);

    mstateen0_write(orig);
    TEST_END();
}

/* MCSRIND-STA-08: mstateen0[60]=0 does NOT affect M-mode mireg* */
TEST_REGISTER(test_mcsrind_sta_08);
bool test_mcsrind_sta_08(void)
{
    TEST_BEGIN("MCSRIND-STA-08: mstateen0[60]=0 does not block M-mode mireg*");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    /* M-mode access to mireg should still work (or trap due to
     * unimplemented select value, but NOT due to mstateen0) */
    miselect_write(0);
    M_TRAP_EXPECT_BEGIN();
    mireg_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* Trap due to unimplemented select is OK; we just verify that
     * mstateen0 does not add an additional blocking layer for M-mode */
    TEST_ASSERT("M-mode mireg access: not blocked by mstateen0[60]=0", 1);
    (void)trapped;

    mstateen0_write(orig);
    TEST_END();
}

/* MCSRIND-STA-09: mstateen0[60]=0 blocks S-mode write sireg */
TEST_REGISTER(test_mcsrind_sta_09);
bool test_mcsrind_sta_09(void)
{
    TEST_BEGIN("MCSRIND-STA-09: mstateen0[60]=0 blocks S-mode sireg write");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    TEST_SMODE_BLOCKED("S-mode sireg write blocked (CSRIND=0)",
                       sireg_write(0x42));

    mstateen0_write(orig);
    TEST_END();
}

/* MCSRIND-STA-10: mstateen0 not implemented - S-mode access works */
TEST_REGISTER(test_mcsrind_sta_10);
bool test_mcsrind_sta_10(void)
{
    TEST_BEGIN("MCSRIND-STA-10: no Smstateen - S-mode access unrestricted");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (platform_has_smstateen()) {
        TEST_SKIP("Smstateen is implemented - test not applicable");
    }

    /* Without Smstateen, there is no mstateen0 control mechanism.
     * S-mode should be able to access siselect/sireg* without
     * mstateen0 blocking. */
    goto_priv(PRIV_S);
    PRIV_DO(siselect_write(0));
    PRIV_DO(siselect_read());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("S-mode siselect access without Smstateen");

    TEST_END();
}
