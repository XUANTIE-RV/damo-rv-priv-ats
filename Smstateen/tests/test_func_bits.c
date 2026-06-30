/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_func_bits.c - Group 6: mstateen0 Functional Bit Control
 *
 * Tests each defined functional bit in mstateen0 for its access
 * control effect on S-mode / HS-mode CSR access.
 *
 * Sub-groups:
 *   6.1  C bit (bit 0) - Custom state
 *   6.2  FCSR bit (bit 1) - Zfinx fcsr
 *   6.3  JVT bit (bit 2) - Zcmt jvt
 *   6.4  SE0 bit (bit 63) - sstateen0/hstateen0 (covered in Group 5)
 *   6.5  ENVCFG bit (bit 62) - senvcfg/henvcfg
 *   6.6  CSRIND bit (bit 60) - siselect/sireg*
 *   6.7  IMSIC bit (bit 58) - stopei/vstopei
 *   6.8  AIA bit (bit 59) - Ssaia remaining state
 *   6.9  CONTEXT bit (bit 57) - scontext/hcontext
 *   6.10 P1P13 bit (bit 56) - hedelegh
 *   6.11 SRMCFG bit (bit 55) - srmcfg
 */

/* =================================================================
 * 6.1 C bit (bit 0) - Custom state control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* MSTA-C-01: mstateen0.C=0 blocks S-mode custom state               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_c_block);
bool test_mstateen0_c_block(void) {
    TEST_BEGIN("MSTA-C-01: mstateen0.C=0 blocks S-mode custom state");

    uintptr_t orig = mstateen0_read();

    /* Clear C bit, keep SE0 to allow sstateen0 access */
    mstateen0_write(MSTATEEN0_SE0);

    /* Verify C bit is actually cleared */
    if (mstateen0_read() & MSTATEEN0_C) {
        mstateen0_write(orig);
        TEST_SKIP("Cannot clear mstateen0.C");
    }

    /* S-mode access to sstateen0 should show C bit as RO0 */
    goto_priv(PRIV_S);
    PRIV_DO(sstateen0_write(MSTATEEN0_C));
    goto_priv(PRIV_M);

    /* Read sstateen0 from M-mode to verify C is zero */
    uintptr_t val = sstateen0_read();
    TEST_ASSERT_BITS("sstateen0.C is RO0 when mstateen0.C=0",
                     val, MSTATEEN0_C, 0);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-C-02: mstateen0.C=1 allows S-mode custom state               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_c_allow);
bool test_mstateen0_c_allow(void) {
    TEST_BEGIN("MSTA-C-02: mstateen0.C=1 allows S-mode custom state");

    uintptr_t orig = mstateen0_read();

    /* Set C and SE0 */
    mstateen0_write(MSTATEEN0_SE0 | MSTATEEN0_C);

    if (!(mstateen0_read() & MSTATEEN0_C)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.C is not writable (no custom state)");
    }

    /* sstateen0.C should now be writable from S-mode */
    goto_priv(PRIV_S);
    PRIV_DO(sstateen0_write(MSTATEEN0_C));
    goto_priv(PRIV_M);

    uintptr_t val = sstateen0_read();
    TEST_ASSERT_BITS("sstateen0.C writable when mstateen0.C=1",
                     val, MSTATEEN0_C, MSTATEEN0_C);

    sstateen0_write(0);
    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.2 FCSR bit (bit 1) - Zfinx fcsr control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* MSTA-FCSR-01: misa.F=1 -> FCSR bit is read-only zero              */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_fcsr_roz_when_f);
bool test_mstateen0_fcsr_roz_when_f(void) {
    TEST_BEGIN("MSTA-FCSR-01: misa.F=1 -> FCSR bit read-only zero");

    if (!HAS_MISA_F()) {
        TEST_SKIP("misa.F is not set");
    }

    uintptr_t orig = mstateen0_read();

    /* Try to set FCSR bit */
    mstateen0_set(MSTATEEN0_FCSR);
    uintptr_t val = mstateen0_read();

    TEST_ASSERT_BITS("FCSR bit is RO0 when misa.F=1",
                     val, MSTATEEN0_FCSR, 0);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-FCSR-02: misa.F=0, FCSR=0 -> FP instructions illegal         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_fcsr0_fp_illegal);
bool test_mstateen0_fcsr0_fp_illegal(void) {
    TEST_BEGIN("MSTA-FCSR-02: misa.F=0 FCSR=0 -> FP instructions illegal");

    if (HAS_MISA_F()) {
        TEST_SKIP("misa.F=1, test requires Zfinx (misa.F=0)");
    }

    uintptr_t orig = mstateen0_read();

    /* Clear FCSR bit */
    mstateen0_clear(MSTATEEN0_FCSR);

    /* S-mode floating-point instruction should be illegal.
     * Use fcsr read as a proxy. */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t dummy;
        asm volatile("csrr %0, fcsr" : "=r"(dummy) :: "memory");
        (void)dummy;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("FP instruction illegal (misa.F=0, FCSR=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-FCSR-03: misa.F=0, FCSR=1 -> fcsr accessible                 */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_fcsr1_accessible);
bool test_mstateen0_fcsr1_accessible(void) {
    TEST_BEGIN("MSTA-FCSR-03: misa.F=0 FCSR=1 -> fcsr accessible");

    if (HAS_MISA_F()) {
        TEST_SKIP("misa.F=1, test requires Zfinx (misa.F=0)");
    }

    uintptr_t orig = mstateen0_read();

    /* Set FCSR bit */
    mstateen0_set(MSTATEEN0_FCSR);

    if (!(mstateen0_read() & MSTATEEN0_FCSR)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.FCSR is not writable");
    }

    /* S-mode fcsr access should succeed */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t dummy;
        asm volatile("csrr %0, fcsr" : "=r"(dummy) :: "memory");
        (void)dummy;
    });
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("fcsr accessible (misa.F=0, FCSR=1)");

    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.3 JVT bit (bit 2) - Zcmt jvt CSR control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* MSTA-JVT-01: mstateen0.JVT=0 blocks S-mode jvt access             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_jvt_block);
bool test_mstateen0_jvt_block(void) {
    TEST_BEGIN("MSTA-JVT-01: mstateen0.JVT=0 blocks S-mode jvt access");

    uintptr_t orig = mstateen0_read();

    /* Clear JVT bit */
    mstateen0_clear(MSTATEEN0_JVT);

    /* First check if JVT CSR (0x017) exists at all */
    M_TRAP_EXPECT_BEGIN();
    uintptr_t dummy;
    asm volatile("csrr %0, 0x017" : "=r"(dummy) :: "memory");
    bool jvt_exists = !trap_was_triggered();
    trap_expect_end();
    (void)dummy;

    if (!jvt_exists) {
        mstateen0_write(orig);
        TEST_SKIP("Zcmt jvt CSR does not exist");
    }

    /* S-mode read of jvt should trigger illegal-instruction */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x017" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("jvt blocked (JVT=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-JVT-02: mstateen0.JVT=1 allows S-mode jvt access             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_jvt_allow);
bool test_mstateen0_jvt_allow(void) {
    TEST_BEGIN("MSTA-JVT-02: mstateen0.JVT=1 allows S-mode jvt access");

    uintptr_t orig = mstateen0_read();

    /* Set JVT bit */
    mstateen0_set(MSTATEEN0_JVT);

    if (!(mstateen0_read() & MSTATEEN0_JVT)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.JVT not writable (Zcmt not implemented)");
    }

    /* Check if jvt CSR (0x017) actually exists on this platform.
     * mstateen0.JVT may be writable even if the jvt CSR is not
     * implemented (WARL allows the bit to be settable). */
    M_TRAP_EXPECT_BEGIN();
    uintptr_t dummy;
    asm volatile("csrr %0, 0x017" : "=r"(dummy) :: "memory");
    bool jvt_exists = !trap_was_triggered();
    trap_expect_end();
    (void)dummy;

    if (!jvt_exists) {
        mstateen0_write(orig);
        TEST_SKIP("jvt CSR does not exist on this platform");
    }

    /* S-mode read of jvt should succeed */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x017" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("jvt allowed (JVT=1)");

    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.5 ENVCFG bit (bit 62) - senvcfg/henvcfg access control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* MSTA-ENVCFG-03: mstateen0.ENVCFG=1 allows access                  */
/* ------------------------------------------------------------------ */

/* =================================================================
 * 6.6 CSRIND bit (bit 60) - Indirect CSR access control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* MSTA-CSRIND-01: mstateen0.CSRIND=0 blocks S-mode siselect         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_csrind_block_siselect);
bool test_mstateen0_csrind_block_siselect(void) {
    TEST_BEGIN("MSTA-CSRIND-01: CSRIND=0 blocks S-mode siselect access");

    uintptr_t orig = mstateen0_read();

    /* Check if CSRIND bit is writable (Sscsrind implemented) */
    mstateen0_set(MSTATEEN0_CSRIND);
    if (!(mstateen0_read() & MSTATEEN0_CSRIND)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.CSRIND not writable (Sscsrind not implemented)");
    }

    /* Clear CSRIND */
    mstateen0_clear(MSTATEEN0_CSRIND);

    /* S-mode read of siselect (0x150) should trap */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x150" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("siselect blocked (CSRIND=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-CSRIND-02: mstateen0.CSRIND=0 blocks S-mode sireg*           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_csrind_block_sireg);
bool test_mstateen0_csrind_block_sireg(void) {
    TEST_BEGIN("MSTA-CSRIND-02: CSRIND=0 blocks S-mode sireg access");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_CSRIND);
    if (!(mstateen0_read() & MSTATEEN0_CSRIND)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.CSRIND not writable (Sscsrind not implemented)");
    }

    mstateen0_clear(MSTATEEN0_CSRIND);

    /* S-mode read of sireg (0x151) should trap */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x151" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("sireg blocked (CSRIND=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-CSRIND-04: mstateen0.CSRIND=1 allows access                  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_csrind_allow);
bool test_mstateen0_csrind_allow(void) {
    TEST_BEGIN("MSTA-CSRIND-04: CSRIND=1 allows S-mode siselect access");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_CSRIND);
    if (!(mstateen0_read() & MSTATEEN0_CSRIND)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.CSRIND not writable");
    }

    /* S-mode read of siselect should succeed */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x150" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("siselect allowed (CSRIND=1)");

    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.7 IMSIC bit (bit 58) - IMSIC state control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* MSTA-IMSIC-01: mstateen0.IMSIC=0 blocks S-mode stopei             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_imsic_block_stopei);
bool test_mstateen0_imsic_block_stopei(void) {
    TEST_BEGIN("MSTA-IMSIC-01: IMSIC=0 blocks S-mode stopei access");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_IMSIC);
    if (!(mstateen0_read() & MSTATEEN0_IMSIC)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.IMSIC not writable (Ssaia IMSIC not implemented)");
    }

    mstateen0_clear(MSTATEEN0_IMSIC);

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x15C" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("stopei blocked (IMSIC=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-IMSIC-03: mstateen0.IMSIC=1 allows access                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_imsic_allow);
bool test_mstateen0_imsic_allow(void) {
    TEST_BEGIN("MSTA-IMSIC-03: IMSIC=1 allows S-mode stopei access");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_IMSIC);
    if (!(mstateen0_read() & MSTATEEN0_IMSIC)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.IMSIC not writable");
    }

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x15C" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("stopei allowed (IMSIC=1)");

    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.8 AIA bit (bit 59) - Ssaia remaining state control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* MSTA-AIA-01: mstateen0.AIA=0 blocks S-mode Ssaia state            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_aia_block);
bool test_mstateen0_aia_block(void) {
    TEST_BEGIN("MSTA-AIA-01: AIA=0 blocks S-mode Ssaia remaining state");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_AIA);
    if (!(mstateen0_read() & MSTATEEN0_AIA)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.AIA not writable (Ssaia not implemented)");
    }

    mstateen0_clear(MSTATEEN0_AIA);

    /* stopi (0xDB0) is one of the Ssaia CSRs not covered by CSRIND/IMSIC */
    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0xDB0" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("Ssaia state blocked (AIA=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-AIA-02: mstateen0.AIA=1 allows access                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_aia_allow);
bool test_mstateen0_aia_allow(void) {
    TEST_BEGIN("MSTA-AIA-02: AIA=1 allows S-mode Ssaia remaining state");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_AIA);
    if (!(mstateen0_read() & MSTATEEN0_AIA)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.AIA not writable");
    }

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0xDB0" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("Ssaia state allowed (AIA=1)");

    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.9 CONTEXT bit (bit 57) - scontext/hcontext control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* MSTA-CTX-01: mstateen0.CONTEXT=0 blocks S-mode scontext           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_context_block_scontext);
bool test_mstateen0_context_block_scontext(void) {
    TEST_BEGIN("MSTA-CTX-01: CONTEXT=0 blocks S-mode scontext access");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_CONTEXT);
    if (!(mstateen0_read() & MSTATEEN0_CONTEXT)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.CONTEXT not writable (Sdtrig not implemented)");
    }

    mstateen0_clear(MSTATEEN0_CONTEXT);

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x5A8" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("scontext blocked (CONTEXT=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-CTX-03: mstateen0.CONTEXT=1 allows access                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_context_allow);
bool test_mstateen0_context_allow(void) {
    TEST_BEGIN("MSTA-CTX-03: CONTEXT=1 allows S-mode scontext access");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_CONTEXT);
    if (!(mstateen0_read() & MSTATEEN0_CONTEXT)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.CONTEXT not writable");
    }

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x5A8" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("scontext allowed (CONTEXT=1)");

    mstateen0_write(orig);
    TEST_END();
}

/* =================================================================
 * 6.10 P1P13 bit (bit 56) - hedelegh control
 * ================================================================= */

/* =================================================================
 * 6.11 SRMCFG bit (bit 55) - srmcfg control
 * ================================================================= */

/* ------------------------------------------------------------------ */
/* MSTA-SRMCFG-01: mstateen0.SRMCFG=0 blocks S-mode srmcfg          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_srmcfg_block);
bool test_mstateen0_srmcfg_block(void) {
    TEST_BEGIN("MSTA-SRMCFG-01: SRMCFG=0 blocks S-mode srmcfg access");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_SRMCFG);
    if (!(mstateen0_read() & MSTATEEN0_SRMCFG)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.SRMCFG not writable (Ssqosid not implemented)");
    }

    mstateen0_clear(MSTATEEN0_SRMCFG);

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x181" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_TRAP("srmcfg blocked (SRMCFG=0)", CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-SRMCFG-02: mstateen0.SRMCFG=1 allows S-mode srmcfg          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_srmcfg_allow);
bool test_mstateen0_srmcfg_allow(void) {
    TEST_BEGIN("MSTA-SRMCFG-02: SRMCFG=1 allows S-mode srmcfg access");

    uintptr_t orig = mstateen0_read();

    mstateen0_set(MSTATEEN0_SRMCFG);
    if (!(mstateen0_read() & MSTATEEN0_SRMCFG)) {
        mstateen0_write(orig);
        TEST_SKIP("mstateen0.SRMCFG not writable");
    }

    goto_priv(PRIV_S);
    PRIV_DO({
        uintptr_t v;
        asm volatile("csrr %0, 0x181" : "=r"(v) :: "memory");
        (void)v;
    });
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("srmcfg allowed (SRMCFG=1)");

    mstateen0_write(orig);
    TEST_END();
}
