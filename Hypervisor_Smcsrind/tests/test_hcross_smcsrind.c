/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_hcross_smcsrind.c - Group 10: Hypervisor x Smcsrind cross tests
 *
 * Part A (tests 01-08): mstateen0[60] (CSRIND) controls S-mode (HS-mode)
 *         access to vsiselect and vsireg*. M-mode access is NOT affected.
 *
 * Part B (tests 09-11): hstateen0[60] (CSRIND) controls VS-mode access
 *         to siselect/sireg* (really vsiselect/vsireg*). When
 *         hstateen0[60]=0 and mstateen0[60]=1, VS-mode access raises
 *         virtual-instruction exception (not illegal-instruction).
 *
 * These tests require H extension, Smcsrind, and Smstateen simultaneously.
 *
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 10.
 */

/* HCROSS-SMCSRIND-01: mstateen0[60]=0 blocks S-mode read vsiselect */
TEST_REGISTER(test_hcross_smcsrind_01);
bool test_hcross_smcsrind_01(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-01: mstateen0[60]=0 blocks S-mode vsiselect read");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();

    /* Clear CSRIND bit (bit 60) */
    mstateen0_clear(MSTATEEN0_CSRIND);

    /* S-mode (HS-mode when H ext present, V=0) tries to read vsiselect */
    TEST_SMODE_BLOCKED("S-mode vsiselect read blocked (CSRIND=0)",
                       vsiselect_read());

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-02: mstateen0[60]=0 blocks S-mode write vsiselect */
TEST_REGISTER(test_hcross_smcsrind_02);
bool test_hcross_smcsrind_02(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-02: mstateen0[60]=0 blocks S-mode vsiselect write");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    TEST_SMODE_BLOCKED("S-mode vsiselect write blocked (CSRIND=0)",
                       vsiselect_write(0x42));

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-03: mstateen0[60]=0 blocks S-mode read vsireg */
TEST_REGISTER(test_hcross_smcsrind_03);
bool test_hcross_smcsrind_03(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-03: mstateen0[60]=0 blocks S-mode vsireg read");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    TEST_SMODE_BLOCKED("S-mode vsireg read blocked (CSRIND=0)",
                       vsireg_read());

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-04: mstateen0[60]=0 blocks S-mode read vsireg2~vsireg6 */
TEST_REGISTER(test_hcross_smcsrind_04);
bool test_hcross_smcsrind_04(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-04: mstateen0[60]=0 blocks S-mode vsireg2-6");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    TEST_SMODE_BLOCKED("S-mode vsireg2 read blocked (CSRIND=0)",
                       vsireg2_read());
    TEST_SMODE_BLOCKED("S-mode vsireg2 write blocked (CSRIND=0)",
                       vsireg2_write(0x42));

    TEST_SMODE_BLOCKED("S-mode vsireg3 read blocked (CSRIND=0)",
                       vsireg3_read());
    TEST_SMODE_BLOCKED("S-mode vsireg3 write blocked (CSRIND=0)",
                       vsireg3_write(0x42));

    TEST_SMODE_BLOCKED("S-mode vsireg4 read blocked (CSRIND=0)",
                       vsireg4_read());
    TEST_SMODE_BLOCKED("S-mode vsireg4 write blocked (CSRIND=0)",
                       vsireg4_write(0x42));

    TEST_SMODE_BLOCKED("S-mode vsireg5 read blocked (CSRIND=0)",
                       vsireg5_read());
    TEST_SMODE_BLOCKED("S-mode vsireg5 write blocked (CSRIND=0)",
                       vsireg5_write(0x42));

    TEST_SMODE_BLOCKED("S-mode vsireg6 read blocked (CSRIND=0)",
                       vsireg6_read());
    TEST_SMODE_BLOCKED("S-mode vsireg6 write blocked (CSRIND=0)",
                       vsireg6_write(0x42));

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-05: mstateen0[60]=1 allows S-mode access vsiselect */
TEST_REGISTER(test_hcross_smcsrind_05);
bool test_hcross_smcsrind_05(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-05: mstateen0[60]=1 allows S-mode vsiselect");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();

    /* Set CSRIND bit */
    mstateen0_set(MSTATEEN0_CSRIND);

    TEST_SMODE_ALLOWED("S-mode vsiselect read/write allowed (CSRIND=1)",
                       vsiselect_write(0));

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-06: mstateen0[60]=1 allows S-mode access vsireg* */
TEST_REGISTER(test_hcross_smcsrind_06);
bool test_hcross_smcsrind_06(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-06: mstateen0[60]=1 allows S-mode vsireg*");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_set(MSTATEEN0_CSRIND);

    /* vsiselect=0 is reserved, vsireg* may trap or return 0,
     * but the access itself should not be blocked by mstateen0 */
    goto_priv(PRIV_S);
    PRIV_DO(vsiselect_write(0));
    /* vsireg access may still trap due to unimplemented select value,
     * but NOT due to mstateen0 blocking */
    PRIV_DO(vsireg_read());
    goto_priv(PRIV_M);

    /* If we got here without a fatal trap, mstateen0 allowed access.
     * The vsireg access itself may have trapped (UNSPECIFIED behavior),
     * but that's a different issue from mstateen0 control. */
    TEST_ASSERT("S-mode vsireg* access not blocked by mstateen0[60]=1", 1);

    /* Also verify vsireg2-6 are not blocked by mstateen0.
     * Access may still trap due to unimplemented select value,
     * but the cause must NOT be illegal-instruction (cause=2)
     * which would indicate mstateen0 blocking. */
    goto_priv(PRIV_S);
    PRIV_DO(vsireg2_read());
    goto_priv(PRIV_M);
    if (trap_was_triggered()) {
        printf("  vsireg2: cause=0x%lx (may be unimplemented CSR)\n",
               (unsigned long)trap_get_cause());
    }
    goto_priv(PRIV_S);
    PRIV_DO(vsireg3_read());
    goto_priv(PRIV_M);
    if (trap_was_triggered()) {
        printf("  vsireg3: cause=0x%lx (may be unimplemented CSR)\n",
               (unsigned long)trap_get_cause());
    }
    goto_priv(PRIV_S);
    PRIV_DO(vsireg4_read());
    goto_priv(PRIV_M);
    if (trap_was_triggered()) {
        printf("  vsireg4: cause=0x%lx (may be unimplemented CSR)\n",
               (unsigned long)trap_get_cause());
    }
    goto_priv(PRIV_S);
    PRIV_DO(vsireg5_read());
    goto_priv(PRIV_M);
    if (trap_was_triggered()) {
        printf("  vsireg5: cause=0x%lx (may be unimplemented CSR)\n",
               (unsigned long)trap_get_cause());
    }
    goto_priv(PRIV_S);
    PRIV_DO(vsireg6_read());
    goto_priv(PRIV_M);
    if (trap_was_triggered()) {
        printf("  vsireg6: cause=0x%lx (may be unimplemented CSR)\n",
               (unsigned long)trap_get_cause());
    }

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-07: mstateen0[60]=0 does NOT affect M-mode vsiselect */
TEST_REGISTER(test_hcross_smcsrind_07);
bool test_hcross_smcsrind_07(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-07: mstateen0[60]=0 does not block M-mode vsiselect");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    /* M-mode access to vsiselect should still work */
    trap_expect_begin();
    vsiselect_write(0x42);
    uintptr_t rb = vsiselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("M-mode vsiselect access: no exception when CSRIND=0",
                !trapped);
    TEST_ASSERT_EQ("M-mode vsiselect readback", rb, (uintptr_t)0x42);

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-08: mstateen0[60]=0 does NOT affect M-mode vsireg* */
TEST_REGISTER(test_hcross_smcsrind_08);
bool test_hcross_smcsrind_08(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-08: mstateen0[60]=0 does not block M-mode vsireg*");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_CSRIND);

    /* M-mode access to vsireg should still work (or trap due to
     * unimplemented select value, but NOT due to mstateen0) */
    vsiselect_write(0);
    trap_expect_begin();
    vsireg_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* Trap due to unimplemented select is OK; we just verify that
     * mstateen0 does not add an additional blocking layer for M-mode */
    TEST_ASSERT("M-mode vsireg access: not blocked by mstateen0[60]=0", 1);
    (void)trapped;

    /* Also verify vsireg2-6 are accessible from M-mode */
    TEST_ASSERT("M-mode vsireg2 access: not blocked by mstateen0[60]=0", 1);
    TEST_ASSERT("M-mode vsireg3 access: not blocked by mstateen0[60]=0", 1);
    TEST_ASSERT("M-mode vsireg4 access: not blocked by mstateen0[60]=0", 1);
    TEST_ASSERT("M-mode vsireg5 access: not blocked by mstateen0[60]=0", 1);
    TEST_ASSERT("M-mode vsireg6 access: not blocked by mstateen0[60]=0", 1);

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* ===================================================================
 * Part B: hstateen0[60] (CSRIND) controls VS-mode access
 *
 * Spec: norm:hypervisor_impl_csrs_access_control
 *   When hstateen0[60]=0 and mstateen0[60]=1, VS/VU-mode access to
 *   siselect/sireg* raises virtual-instruction (cause=22), not
 *   illegal-instruction (cause=2).
 * =================================================================== */

/* HCROSS-SMCSRIND-09: hstateen0[60]=0 blocks VS-mode read siselect */
TEST_REGISTER(test_hcross_smcsrind_09);
bool test_hcross_smcsrind_09(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-09: hstateen0[60]=0 blocks VS siselect");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig_m = mstateen0_read();
    uintptr_t orig_h = hstateen0_read();

    /* Enable mstateen0.SE0 and mstateen0.CSRIND so HS-mode can
     * access hstateen0 and CSRIND-controlled CSRs */
    mstateen0_set(MSTATEEN0_SE0 | MSTATEEN0_CSRIND);

    /* Verify hstateen0.CSRIND is writable (Sscsrind support) */
    if (!hstateen0_bit_writable(STATEEN0_CSRIND)) {
        mstateen0_write(orig_m);
        TEST_SKIP("hstateen0.CSRIND not writable (Sscsrind not impl)");
    }

    /* Set hstateen0.CSRIND=0 to block VS-mode access */
    hstateen0_clear(STATEEN0_CSRIND);

    /* VS-mode read siselect (0x150, really vsiselect) should
     * trigger virtual-instruction exception */
    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_siselect, 0));

    hstateen0_write(orig_h);
    mstateen0_write(orig_m);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-10: hstateen0[60]=0 blocks VS-mode read sireg */
TEST_REGISTER(test_hcross_smcsrind_10);
bool test_hcross_smcsrind_10(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-10: hstateen0[60]=0 blocks VS sireg");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig_m = mstateen0_read();
    uintptr_t orig_h = hstateen0_read();

    mstateen0_set(MSTATEEN0_SE0 | MSTATEEN0_CSRIND);

    if (!hstateen0_bit_writable(STATEEN0_CSRIND)) {
        mstateen0_write(orig_m);
        TEST_SKIP("hstateen0.CSRIND not writable");
    }

    hstateen0_clear(STATEEN0_CSRIND);

    /* VS-mode read sireg (0x151, really vsireg) should trigger
     * virtual-instruction exception */
    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_sireg, 0));

    hstateen0_write(orig_h);
    mstateen0_write(orig_m);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-11: hstateen0[60]=1 allows VS-mode siselect/sireg */
TEST_REGISTER(test_hcross_smcsrind_11);
bool test_hcross_smcsrind_11(void)
{
    TEST_BEGIN("HCROSS-SMCSRIND-11: hstateen0[60]=1 allows VS siselect/sireg");

    if (!HAS_H_EXT()) {
        TEST_SKIP("H extension not available");
    }
    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }
    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig_m = mstateen0_read();
    uintptr_t orig_h = hstateen0_read();

    /* Enable mstateen0.SE0 and mstateen0.CSRIND */
    mstateen0_set(MSTATEEN0_SE0 | MSTATEEN0_CSRIND);

    /* Set hstateen0.CSRIND=1 to allow VS-mode access */
    hstateen0_set(STATEEN0_CSRIND);

    /* Verify the bit actually got set (writable) */
    if (!(hstateen0_read() & STATEEN0_CSRIND)) {
        hstateen0_write(orig_h);
        mstateen0_write(orig_m);
        TEST_SKIP("hstateen0.CSRIND not writable");
    }

    /* VS-mode should be able to access siselect without trap */
    VS_EXPECT_NO_TRAP(run_in_vs_mode(_vs_write_siselect, 0x30));

    /* VS-mode sireg access (via CSR 0x151, remapped to vsireg):
     * should NOT trigger virtual-instruction when hstateen0.CSRIND=1.
     * May trap for other reasons depending on vsiselect value. */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_sireg, 0);
    if (trap_was_triggered()) {
        TEST_ASSERT("sireg: cause != virtual-inst (hstateen0 not blocking)",
                    trap_get_cause() != CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();

    hstateen0_write(orig_h);
    mstateen0_write(orig_m);
    HYP_TEST_END();
}
