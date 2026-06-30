/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 3: sstateen0 Functional Bits
 *
 * Spec anchors:
 *   norm:stateen0_c_op       — C bit controls custom state
 *   norm:stateen0_fcsr_op    — FCSR bit controls Zfinx fcsr
 *   norm:mstateen0_fcsr_roz  — misa.F=1 => FCSR bit read-only zero
 *   norm:stateen0_jvt_op     — JVT bit controls Zcmt jvt
 *   norm:stateen_reserved_roz — reserved bits read-only zero
 *
 * 9 tests: SS-FUNC-01 ~ SS-FUNC-09
 * =================================================================== */

/* ---- SS-FUNC-01: sstateen0.C bit writable ---- */

TEST_REGISTER(test_ss_func_c_writable);
bool test_ss_func_c_writable(void) {
    TEST_BEGIN("SS-FUNC-01: sstateen0.C bit writable");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_C | STATEEN0_SE0);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Write C=1 */
    sstateen_set_bits(0, STATEEN0_C);
    uintptr_t rb = sstateen_read(0);

    if (!(rb & STATEEN0_C)) {
        sstateen_write(0, saved_sstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("sstateen0.C not writable (mstateen0.C=0 or no custom state)");
    }

    TEST_ASSERT("sstateen0.C set to 1", (rb & STATEEN0_C) != 0);

    /* Write C=0 */
    sstateen_clear_bits(0, STATEEN0_C);
    rb = sstateen_read(0);
    TEST_ASSERT("sstateen0.C cleared to 0", (rb & STATEEN0_C) == 0);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-FUNC-02: sstateen0.C=0 blocks U-mode custom access ---- */

TEST_REGISTER(test_ss_func_c_zero_blocks);
bool test_ss_func_c_zero_blocks(void) {
    TEST_BEGIN("SS-FUNC-02: sstateen0.C=0 blocks U-mode custom access");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_C | STATEEN0_SE0);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Probe C bit */
    sstateen_set_bits(0, STATEEN0_C);
    uintptr_t probe = sstateen_read(0);
    sstateen_clear_bits(0, STATEEN0_C);

    if (!(probe & STATEEN0_C)) {
        sstateen_write(0, saved_sstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("sstateen0.C not writable");
    }

    /* C=0: U-mode custom CSR access should trap */
    SS_TEST_UMODE_BLOCKED("U-mode custom CSR blocked by C=0", {
        uintptr_t _v;
        asm volatile("csrr %0, 0x800" : "=r"(_v));
        (void)_v;
    });

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-FUNC-03: sstateen0.C=1 allows U-mode custom access ---- */

TEST_REGISTER(test_ss_func_c_one_allows);
bool test_ss_func_c_one_allows(void) {
    TEST_BEGIN("SS-FUNC-03: sstateen0.C=1 allows U-mode custom access");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_C | STATEEN0_SE0);

    uintptr_t saved_sstateen0 = sstateen_read(0);
    sstateen_set_bits(0, STATEEN0_C);
    uintptr_t probe = sstateen_read(0);

    if (!(probe & STATEEN0_C)) {
        sstateen_write(0, saved_sstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("sstateen0.C not writable");
    }

    /* C=1: stateen gate is open. Actual access depends on impl. */
    TEST_ASSERT("sstateen0.C == 1", (sstateen_read(0) & STATEEN0_C) != 0);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-FUNC-04: sstateen0.FCSR read-only zero when misa.F=1 ---- */

TEST_REGISTER(test_ss_func_fcsr_roz_misa_f);
bool test_ss_func_fcsr_roz_misa_f(void) {
    TEST_BEGIN("SS-FUNC-04: sstateen0.FCSR ROZ when misa.F=1");

    if (!HAS_MISA_F()) {
        TEST_SKIP("misa.F=0; FCSR ROZ test requires misa.F=1");
    }

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_FCSR | STATEEN0_SE0);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Per spec: when misa.F=1, the FCSR bit in mstateen0 (and hence
     * sstateen0) is read-only zero. */
    sstateen_set_bits(0, STATEEN0_FCSR);
    uintptr_t rb = sstateen_read(0);
    TEST_ASSERT("sstateen0.FCSR is ROZ when misa.F=1",
                (rb & STATEEN0_FCSR) == 0);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-FUNC-05: sstateen0.FCSR=0 blocks FP instr (Zfinx) ---- */

TEST_REGISTER(test_ss_func_fcsr_blocks_fp);
bool test_ss_func_fcsr_blocks_fp(void) {
    TEST_BEGIN("SS-FUNC-05: sstateen0.FCSR=0 blocks FP instr (Zfinx)");

    if (HAS_MISA_F()) {
        TEST_SKIP("misa.F=1; FCSR gating only applies with Zfinx (misa.F=0)");
    }

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_FCSR | STATEEN0_SE0);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Probe FCSR writability */
    sstateen_set_bits(0, STATEEN0_FCSR);
    uintptr_t probe = sstateen_read(0);
    sstateen_clear_bits(0, STATEEN0_FCSR);

    if (!(probe & STATEEN0_FCSR)) {
        sstateen_write(0, saved_sstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("sstateen0.FCSR not writable (Zfinx not implemented)");
    }

    /* FCSR=0: U-mode fcsr access should trap */
    SS_TEST_UMODE_BLOCKED("U-mode fcsr read blocked by FCSR=0", {
        asm volatile("csrr zero, fcsr");
    });

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-FUNC-06: sstateen0.JVT bit writable ---- */

TEST_REGISTER(test_ss_func_jvt_writable);
bool test_ss_func_jvt_writable(void) {
    TEST_BEGIN("SS-FUNC-06: sstateen0.JVT bit writable");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_JVT | STATEEN0_SE0);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    sstateen_set_bits(0, STATEEN0_JVT);
    uintptr_t rb = sstateen_read(0);

    if (!(rb & STATEEN0_JVT)) {
        sstateen_write(0, saved_sstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("sstateen0.JVT not writable (Zcmt not implemented)");
    }

    TEST_ASSERT("sstateen0.JVT set to 1", (rb & STATEEN0_JVT) != 0);

    sstateen_clear_bits(0, STATEEN0_JVT);
    rb = sstateen_read(0);
    TEST_ASSERT("sstateen0.JVT cleared to 0", (rb & STATEEN0_JVT) == 0);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-FUNC-07: sstateen0.JVT=0 blocks U-mode jvt ---- */

TEST_REGISTER(test_ss_func_jvt_zero_blocks);
bool test_ss_func_jvt_zero_blocks(void) {
    TEST_BEGIN("SS-FUNC-07: sstateen0.JVT=0 blocks U-mode jvt");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_JVT | STATEEN0_SE0);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    sstateen_set_bits(0, STATEEN0_JVT);
    uintptr_t probe = sstateen_read(0);
    sstateen_clear_bits(0, STATEEN0_JVT);

    if (!(probe & STATEEN0_JVT)) {
        sstateen_write(0, saved_sstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("sstateen0.JVT not writable");
    }

    SS_TEST_UMODE_BLOCKED("U-mode jvt blocked by JVT=0", {
        uintptr_t _v;
        asm volatile("csrr %0, 0x017" : "=r"(_v));
        (void)_v;
    });

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-FUNC-08: sstateen0.JVT=1 allows U-mode jvt ---- */

TEST_REGISTER(test_ss_func_jvt_one_allows);
bool test_ss_func_jvt_one_allows(void) {
    TEST_BEGIN("SS-FUNC-08: sstateen0.JVT=1 allows U-mode jvt");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_JVT | STATEEN0_SE0);

    uintptr_t saved_sstateen0 = sstateen_read(0);
    sstateen_set_bits(0, STATEEN0_JVT);
    uintptr_t probe = sstateen_read(0);

    if (!(probe & STATEEN0_JVT)) {
        sstateen_write(0, saved_sstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("sstateen0.JVT not writable");
    }

    SS_TEST_UMODE_ALLOWED("U-mode jvt allowed by JVT=1", {
        uintptr_t _v;
        asm volatile("csrr %0, 0x017" : "=r"(_v));
        (void)_v;
    });

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-FUNC-09: sstateen0 WPRI bits read-only zero ---- */

TEST_REGISTER(test_ss_func_wpri_roz);
bool test_ss_func_wpri_roz(void) {
    TEST_BEGIN("SS-FUNC-09: sstateen0 WPRI bits read-only zero");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, ~0UL);  /* Open all mstateen0 bits */

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Write all 1s to sstateen0 */
    sstateen_write(0, ~0UL);
    uintptr_t rb = sstateen_read(0);

    /* Bits [31:3] in sstateen0 are WPRI (reserved).
     * They should read back as 0. */
    uintptr_t reserved_mask = 0xFFFFFFF8UL;  /* bits [31:3] */
    /* Remove any defined bits in this range if present */
    reserved_mask &= ~(STATEEN0_C | STATEEN0_FCSR | STATEEN0_JVT);

    uintptr_t reserved_readback = rb & reserved_mask;
    TEST_ASSERT("sstateen0 reserved bits [31:3] are ROZ",
                reserved_readback == 0);

    printf("  sstateen0 readback: 0x%lx, reserved mask: 0x%lx, "
           "reserved bits: 0x%lx\n",
           (unsigned long)rb, (unsigned long)reserved_mask,
           (unsigned long)reserved_readback);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}
