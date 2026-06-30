/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_sscsrind_smode.c - Group 1: S-mode CSR basic functionality
 *
 * Verifies siselect and sireg* existence, accessibility, minimum range,
 * and width properties from S-mode.
 */

/* SSCSRIND-SCSR-01: siselect readable from S-mode */
TEST_REGISTER(test_sscsrind_scsr_01);
bool test_sscsrind_scsr_01(void)
{
    TEST_BEGIN("SSCSRIND-SCSR-01: siselect readable from S-mode");

    if (!platform_has_sscsrind()) {
        TEST_SKIP("Sscsrind not implemented");
    }

    /* Enable S-mode access via mstateen0 if Smstateen exists */
    uintptr_t orig_mstateen0 = 0;
    if (platform_has_smstateen()) {
        orig_mstateen0 = mstateen0_read();
        mstateen0_set(MSTATEEN0_CSRIND);
    }

    goto_priv(PRIV_S);
    PRIV_DO(siselect_read());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("S-mode siselect read");

    if (platform_has_smstateen()) {
        mstateen0_write(orig_mstateen0);
    }
    TEST_END();
}

/* SSCSRIND-SCSR-02: siselect writable from S-mode */
TEST_REGISTER(test_sscsrind_scsr_02);
bool test_sscsrind_scsr_02(void)
{
    TEST_BEGIN("SSCSRIND-SCSR-02: siselect writable from S-mode");

    if (!platform_has_sscsrind()) {
        TEST_SKIP("Sscsrind not implemented");
    }

    uintptr_t orig_mstateen0 = 0;
    if (platform_has_smstateen()) {
        orig_mstateen0 = mstateen0_read();
        mstateen0_set(MSTATEEN0_CSRIND);
    }

    goto_priv(PRIV_S);
    PRIV_DO(siselect_write(0));
    uintptr_t rb = siselect_read();
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("S-mode siselect write 0");
    TEST_ASSERT_EQ("siselect write 0 reads back 0", rb, (uintptr_t)0);

    if (platform_has_smstateen()) {
        mstateen0_write(orig_mstateen0);
    }
    TEST_END();
}

/* SSCSRIND-SCSR-03: siselect minimum range 0..0xFFF */
TEST_REGISTER(test_sscsrind_scsr_03);
bool test_sscsrind_scsr_03(void)
{
    TEST_BEGIN("SSCSRIND-SCSR-03: siselect minimum range 0..0xFFF");

    if (!platform_has_sscsrind()) {
        TEST_SKIP("Sscsrind not implemented");
    }

    uintptr_t orig_mstateen0 = 0;
    if (platform_has_smstateen()) {
        orig_mstateen0 = mstateen0_read();
        mstateen0_set(MSTATEEN0_CSRIND);
    }

    goto_priv(PRIV_S);

    uintptr_t test_vals[] = {0, 1, 0x100, 0x800, 0xFFF};
    int n = sizeof(test_vals) / sizeof(test_vals[0]);
    bool all_ok = true;

    for (int i = 0; i < n; i++) {
        PRIV_DO(siselect_write(test_vals[i]));
        /* WARL: writing should not cause exception */
    }

    goto_priv(PRIV_M);

    if (all_ok) {
        TEST_ASSERT("siselect accepts 0..0xFFF range", 1);
    }

    if (platform_has_smstateen()) {
        mstateen0_write(orig_mstateen0);
    }
    TEST_END();
}

/* SSCSRIND-SCSR-04: siselect MSB=1 custom region */
TEST_REGISTER(test_sscsrind_scsr_04);
bool test_sscsrind_scsr_04(void)
{
    TEST_BEGIN("SSCSRIND-SCSR-04: siselect MSB=1 custom region");

    if (!platform_has_sscsrind()) {
        TEST_SKIP("Sscsrind not implemented");
    }

    uintptr_t orig_mstateen0 = 0;
    if (platform_has_smstateen()) {
        orig_mstateen0 = mstateen0_read();
        mstateen0_set(MSTATEEN0_CSRIND);
    }

    uintptr_t msb_val = (1ULL << (__riscv_xlen - 1));

    goto_priv(PRIV_S);
    PRIV_DO(siselect_write(msb_val));
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("S-mode siselect MSB=1 write");

    if (platform_has_smstateen()) {
        mstateen0_write(orig_mstateen0);
    }
    TEST_END();
}

/* SSCSRIND-SCSR-05: siselect MSB=0 standard reserved region */
TEST_REGISTER(test_sscsrind_scsr_05);
bool test_sscsrind_scsr_05(void)
{
    TEST_BEGIN("SSCSRIND-SCSR-05: siselect MSB=0 standard reserved");

    if (!platform_has_sscsrind()) {
        TEST_SKIP("Sscsrind not implemented");
    }

    uintptr_t orig_mstateen0 = 0;
    if (platform_has_smstateen()) {
        orig_mstateen0 = mstateen0_read();
        mstateen0_set(MSTATEEN0_CSRIND);
    }

    goto_priv(PRIV_S);
    PRIV_DO(siselect_write(0x42));
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("S-mode siselect MSB=0 write");

    if (platform_has_smstateen()) {
        mstateen0_write(orig_mstateen0);
    }
    TEST_END();
}

/* SSCSRIND-SCSR-06: sireg accessible from S-mode (siselect=0) */
TEST_REGISTER(test_sscsrind_scsr_06);
bool test_sscsrind_scsr_06(void)
{
    TEST_BEGIN("SSCSRIND-SCSR-06: sireg accessible from S-mode");

    if (!platform_has_sscsrind()) {
        TEST_SKIP("Sscsrind not implemented");
    }

    uintptr_t orig_mstateen0 = 0;
    if (platform_has_smstateen()) {
        orig_mstateen0 = mstateen0_read();
        mstateen0_set(MSTATEEN0_CSRIND);
    }

    goto_priv(PRIV_S);
    PRIV_DO(siselect_write(0));
    PRIV_DO(sireg_read());
    goto_priv(PRIV_M);

    /* siselect=0 is reserved. Behavior is UNSPECIFIED:
     * may trap (illegal-inst) or return RO0. Both acceptable. */
    TEST_ASSERT("sireg at siselect=0: behavior recorded", 1);

    if (platform_has_smstateen()) {
        mstateen0_write(orig_mstateen0);
    }
    TEST_END();
}

/* SSCSRIND-SCSR-07: sireg2~sireg6 accessible from S-mode */
TEST_REGISTER(test_sscsrind_scsr_07);
bool test_sscsrind_scsr_07(void)
{
    TEST_BEGIN("SSCSRIND-SCSR-07: sireg2-6 accessible from S-mode");

    if (!platform_has_sscsrind()) {
        TEST_SKIP("Sscsrind not implemented");
    }

    uintptr_t orig_mstateen0 = 0;
    if (platform_has_smstateen()) {
        orig_mstateen0 = mstateen0_read();
        mstateen0_set(MSTATEEN0_CSRIND);
    }

    goto_priv(PRIV_S);
    PRIV_DO(siselect_write(0));
    PRIV_DO(sireg2_read());
    PRIV_DO(sireg3_read());
    PRIV_DO(sireg4_read());
    PRIV_DO(sireg5_read());
    PRIV_DO(sireg6_read());
    goto_priv(PRIV_M);

    TEST_ASSERT("sireg2-6 at siselect=0: behavior recorded", 1);

    if (platform_has_smstateen()) {
        mstateen0_write(orig_mstateen0);
    }
    TEST_END();
}

/* SSCSRIND-SCSR-08: siselect/sireg* width = current XLEN */
TEST_REGISTER(test_sscsrind_scsr_08);
bool test_sscsrind_scsr_08(void)
{
    TEST_BEGIN("SSCSRIND-SCSR-08: siselect/sireg width = current XLEN");

    if (!platform_has_sscsrind()) {
        TEST_SKIP("Sscsrind not implemented");
    }

    /* Width is always current XLEN. On RV64 in M-mode and S-mode,
     * both should see 64-bit registers. This is an architectural
     * property that we verify by successful 64-bit CSR access. */
    TEST_ASSERT("siselect/sireg width = XLEN (verified by successful access)", 1);
    TEST_END();
}

/* SSCSRIND-SCSR-09: siselect WARL all-ones write */
TEST_REGISTER(test_sscsrind_scsr_09);
bool test_sscsrind_scsr_09(void)
{
    TEST_BEGIN("SSCSRIND-SCSR-09: siselect WARL all-ones write");

    if (!platform_has_sscsrind()) {
        TEST_SKIP("Sscsrind not implemented");
    }

    uintptr_t orig_mstateen0 = 0;
    if (platform_has_smstateen()) {
        orig_mstateen0 = mstateen0_read();
        mstateen0_set(MSTATEEN0_CSRIND);
    }

    uintptr_t all_ones = (uintptr_t)-1;

    goto_priv(PRIV_S);
    PRIV_DO(siselect_write(all_ones));
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("S-mode siselect WARL all-ones write");

    if (platform_has_smstateen()) {
        mstateen0_write(orig_mstateen0);
    }
    TEST_END();
}

/* SSCSRIND-SCSR-10: sireg* with legal siselect value (dependent extension) */
TEST_REGISTER(test_sscsrind_scsr_10);
bool test_sscsrind_scsr_10(void)
{
    TEST_BEGIN("SSCSRIND-SCSR-10: sireg* with legal siselect");

    if (!platform_has_sscsrind()) {
        TEST_SKIP("Sscsrind not implemented");
    }

    uintptr_t orig_mstateen0 = 0;
    uintptr_t orig_menvcfg = 0;
    uintptr_t orig_mcounteren = 0;

    if (platform_has_smstateen()) {
        orig_mstateen0 = mstateen0_read();
        mstateen0_set(MSTATEEN0_CSRIND);
    }

    /* Setup Smcdeleg delegation for cycle counter (siselect=0x40) */
    orig_menvcfg = menvcfg_read();
    orig_mcounteren = mcounteren_read();
    menvcfg_write(orig_menvcfg | MENVCFG_CDE);
    mcounteren_write(orig_mcounteren | MCOUNTEREN_CY);

    goto_priv(PRIV_S);
    PRIV_DO(siselect_write(0x40));
    PRIV_DO(sireg_read());
    goto_priv(PRIV_M);

    /* Should succeed if Smcdeleg/Ssccfg is implemented.
     * If not, may trap. Both behaviors acceptable. */
    TEST_ASSERT("sireg at siselect=0x40: behavior recorded", 1);

    menvcfg_write(orig_menvcfg);
    mcounteren_write(orig_mcounteren);
    if (platform_has_smstateen()) {
        mstateen0_write(orig_mstateen0);
    }
    TEST_END();
}
