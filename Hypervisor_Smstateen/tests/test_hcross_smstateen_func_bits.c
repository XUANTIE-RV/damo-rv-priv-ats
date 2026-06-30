/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_smstateen_func_bits.c - HCROSS-SMSTA-05~12: Functional bit control
 *
 * Verifies that mstateen0 functional bits control HS-mode access
 * to various Hypervisor-related CSRs (hstateen0, henvcfg, vsiselect,
 * vstopei, hcontext, hedelegh).
 */

/* =================================================================
 * 6.4 SE0 bit (bit 63) - hstateen0/hstateen0h access control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-05: mstateen0.SE0=0 blocks HS-mode hstateen0        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_05);
bool test_hcross_smsta_05(void) {
    TEST_BEGIN("HCROSS-SMSTA-05: SE0=0 blocks HS-mode hstateen0 access");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    /* Clear SE0 */
    mstateen0_clear(MSTATEEN0_SE0);

    /* HS-mode hstateen0 access should be blocked */
    SMSTATEEN_TEST_SMODE_BLOCKED(
        "hstateen0 blocked (SE0=0)",
        hstateen0_read());

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-06: mstateen0.SE0=0 blocks HS-mode hstateen0h (RV32)*/
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_06);
bool test_hcross_smsta_06(void) {
    TEST_BEGIN("HCROSS-SMSTA-06: SE0=0 blocks HS-mode hstateen0h (RV32)");

#if __riscv_xlen == 32
    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    /* Clear SE0 */
    mstateen0_clear(MSTATEEN0_SE0);

    /* HS-mode hstateen0h (0x61C) access should be blocked */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x61C" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("hstateen0h blocked (SE0=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
#else
    TEST_SKIP("RV32-only test");
#endif

    TEST_END();
}

/* =================================================================
 * 6.5 ENVCFG bit (bit 62) - henvcfg access control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-07: mstateen0.ENVCFG=0 blocks HS-mode henvcfg       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_07);
bool test_hcross_smsta_07(void) {
    TEST_BEGIN("HCROSS-SMSTA-07: ENVCFG=0 blocks HS-mode henvcfg access");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    /* Set SE0 to allow sstateen0 access, then clear ENVCFG */
    mstateen0_set(MSTATEEN0_SE0);
    mstateen0_clear(MSTATEEN0_ENVCFG);

    if (mstateen0_read() & MSTATEEN0_ENVCFG) {
        mstateen0_write(orig);
        TEST_SKIP("Cannot clear mstateen0.ENVCFG");
    }

    /* HS-mode henvcfg access should be blocked */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x60A" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("henvcfg blocked (ENVCFG=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.6 CSRIND bit (bit 60) - vsiselect access control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-08: mstateen0.CSRIND=0 blocks HS-mode vsiselect     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_08);
bool test_hcross_smsta_08(void) {
    TEST_BEGIN("HCROSS-SMSTA-08: CSRIND=0 blocks HS-mode vsiselect");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_CSRIND);
    if (!(mstateen0_read() & MSTATEEN0_CSRIND)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.CSRIND not writable");
    }

    mstateen0_clear(MSTATEEN0_CSRIND);

    /* HS-mode read of vsiselect (0x250) should trap */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x250" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("vsiselect blocked (CSRIND=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.7 IMSIC bit (bit 58) - vstopei access control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-09: mstateen0.IMSIC=0 blocks HS-mode vstopei        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_09);
bool test_hcross_smsta_09(void) {
    TEST_BEGIN("HCROSS-SMSTA-09: IMSIC=0 blocks HS-mode vstopei access");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_IMSIC);
    if (!(mstateen0_read() & MSTATEEN0_IMSIC)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.IMSIC not writable");
    }

    mstateen0_clear(MSTATEEN0_IMSIC);

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x25C" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("vstopei blocked (IMSIC=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.9 CONTEXT bit (bit 57) - hcontext access control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-10: mstateen0.CONTEXT=0 blocks HS-mode hcontext     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_10);
bool test_hcross_smsta_10(void) {
    TEST_BEGIN("HCROSS-SMSTA-10: CONTEXT=0 blocks HS-mode hcontext access");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_CONTEXT);
    if (!(mstateen0_read() & MSTATEEN0_CONTEXT)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.CONTEXT not writable");
    }

    mstateen0_clear(MSTATEEN0_CONTEXT);

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x6A8" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("hcontext blocked (CONTEXT=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.10 P1P13 bit (bit 56) - hedelegh access control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-11: mstateen0.P1P13=0 blocks HS-mode hedelegh       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_11);
bool test_hcross_smsta_11(void) {
    TEST_BEGIN("HCROSS-SMSTA-11: P1P13=0 blocks HS-mode hedelegh access");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_P1P13);
    if (!(mstateen0_read() & MSTATEEN0_P1P13)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.P1P13 not writable (hedelegh not implemented)");
    }

    mstateen0_clear(MSTATEEN0_P1P13);

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x613" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("hedelegh blocked (P1P13=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-12: mstateen0.P1P13=1 allows HS-mode hedelegh       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_12);
bool test_hcross_smsta_12(void) {
    TEST_BEGIN("HCROSS-SMSTA-12: P1P13=1 allows HS-mode hedelegh access");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_P1P13);
    if (!(mstateen0_read() & MSTATEEN0_P1P13)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.P1P13 not writable");
    }

    /* Check if hedelegh CSR (0x613) actually exists on this platform.
     * mstateen0.P1P13 may be writable even if hedelegh is not
     * implemented. Probe from M-mode first. */
    M_TRAP_EXPECT_BEGIN();
    uintptr_t dummy;
    asm volatile("csrr %0, 0x613" : "=r"(dummy) :: "memory");
    bool hedelegh_exists = !trap_was_triggered();
    trap_expect_end();
    (void)dummy;

    if (!hedelegh_exists) {
        mstateen0_write(orig);
        TEST_SKIP("hedelegh CSR does not exist on this platform");
    }

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x613" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("hedelegh allowed (P1P13=1)");

    mstateen0_write(orig);
    TEST_END();
}
