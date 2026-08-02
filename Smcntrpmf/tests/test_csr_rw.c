/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_csr_rw.c - Group 1: mcyclecfg / minstretcfg CSR Access & Field Constraints
 *
 * Tests PMF-CSR-01 through PMF-CSR-13
 * Validates CSR read/write attributes, field layout, and WARL constraints.
 */

/* ------------------------------------------------------------------ */
/* PMF-CSR-01: mcyclecfg basic read/write                            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_01_mcyclecfg_rw);
bool test_pmf_csr_01_mcyclecfg_rw(void)
{
    TEST_BEGIN("PMF-CSR-01: mcyclecfg basic read/write");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    /* Write all xINH bits, read back */
    mcyclecfg_write(CYCLECFG_ALL_XINH);
    uintptr_t val = mcyclecfg_read();

    /* At minimum MINH should be writable (M-mode always exists) */
    TEST_ASSERT("MINH bit writable", (val & CYCLECFG_MINH) != 0);

    /* WPRI bits 57:0 should read as zero */
    TEST_ASSERT("WPRI bits 57:0 are zero", (val & CYCLECFG_WPRI_MASK) == 0);

    /* Restore */
    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-02: minstretcfg basic read/write                          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_02_minstretcfg_rw);
bool test_pmf_csr_02_minstretcfg_rw(void)
{
    TEST_BEGIN("PMF-CSR-02: minstretcfg basic read/write");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    /* Write all xINH bits, read back */
    minstretcfg_write(CYCLECFG_ALL_XINH);
    uintptr_t val = minstretcfg_read();

    /* At minimum MINH should be writable */
    TEST_ASSERT("MINH bit writable", (val & CYCLECFG_MINH) != 0);

    /* WPRI bits 57:0 should read as zero */
    TEST_ASSERT("WPRI bits 57:0 are zero", (val & CYCLECFG_WPRI_MASK) == 0);

    /* Restore */
    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-03: mcyclecfg bit 63 (OF) read-only zero                  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_03_mcyclecfg_of_ro);
bool test_pmf_csr_03_mcyclecfg_of_ro(void)
{
    TEST_BEGIN("PMF-CSR-03: mcyclecfg bit 63 (OF) read-only zero");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    /* Write bit 63 = 1 */
    mcyclecfg_write(CYCLECFG_OF | CYCLECFG_MINH);
    uintptr_t val = mcyclecfg_read();

    TEST_ASSERT("bit 63 is read-only zero", (val & CYCLECFG_OF) == 0);
    TEST_ASSERT("MINH still set", (val & CYCLECFG_MINH) != 0);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-04: minstretcfg bit 63 (OF) read-only zero                */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_04_minstretcfg_of_ro);
bool test_pmf_csr_04_minstretcfg_of_ro(void)
{
    TEST_BEGIN("PMF-CSR-04: minstretcfg bit 63 (OF) read-only zero");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    /* Write bit 63 = 1 */
    minstretcfg_write(CYCLECFG_OF | CYCLECFG_MINH);
    uintptr_t val = minstretcfg_read();

    TEST_ASSERT("bit 63 is read-only zero", (val & CYCLECFG_OF) == 0);
    TEST_ASSERT("MINH still set", (val & CYCLECFG_MINH) != 0);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-06: SINH read-only zero without S-mode                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_06_sinh_no_smode);
bool test_pmf_csr_06_sinh_no_smode(void)
{
    TEST_BEGIN("PMF-CSR-06: SINH read-only zero without S-mode");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    if (has_smode())
        TEST_SKIP("S-mode is implemented");

    mcyclecfg_write(CYCLECFG_SINH);
    uintptr_t val = mcyclecfg_read();

    TEST_ASSERT("SINH is read-only zero", (val & CYCLECFG_SINH) == 0);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-07: UINH read-only zero without U-mode                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_07_uinh_no_umode);
bool test_pmf_csr_07_uinh_no_umode(void)
{
    TEST_BEGIN("PMF-CSR-07: UINH read-only zero without U-mode");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    if (has_umode())
        TEST_SKIP("U-mode is implemented");

    mcyclecfg_write(CYCLECFG_UINH);
    uintptr_t val = mcyclecfg_read();

    TEST_ASSERT("UINH is read-only zero", (val & CYCLECFG_UINH) == 0);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-08: mcyclecfg WPRI field write ignored                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_08_mcyclecfg_wpri);
bool test_pmf_csr_08_mcyclecfg_wpri(void)
{
    TEST_BEGIN("PMF-CSR-08: mcyclecfg WPRI field write ignored");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    /* Write all ones to WPRI bits 57:0 */
    mcyclecfg_write(CYCLECFG_WPRI_MASK);
    uintptr_t val = mcyclecfg_read();

    TEST_ASSERT("WPRI bits read back zero", (val & CYCLECFG_WPRI_MASK) == 0);

    mcyclecfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-09: minstretcfg WPRI field write ignored                  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_09_minstretcfg_wpri);
bool test_pmf_csr_09_minstretcfg_wpri(void)
{
    TEST_BEGIN("PMF-CSR-09: minstretcfg WPRI field write ignored");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    /* Write all ones to WPRI bits 57:0 */
    minstretcfg_write(CYCLECFG_WPRI_MASK);
    uintptr_t val = minstretcfg_read();

    TEST_ASSERT("WPRI bits read back zero", (val & CYCLECFG_WPRI_MASK) == 0);

    minstretcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-10: S-mode access mcyclecfg triggers exception            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_10_smode_access_trap);
bool test_pmf_csr_10_smode_access_trap(void)
{
    TEST_BEGIN("PMF-CSR-10: S-mode access mcyclecfg triggers exception");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    if (!has_smode())
        TEST_SKIP("S-mode not supported");

    /* Switch to S-mode and try to read mcyclecfg */
    goto_priv(PRIV_S);
    trap_expect_begin();
    asm volatile("csrr x0, 0x321" ::: "memory");
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();
    goto_priv(PRIV_M);

    TEST_ASSERT("trap triggered in S-mode", trapped);
    TEST_ASSERT_EQ("cause is illegal instruction", cause, CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-11: U-mode access mcyclecfg triggers exception            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_11_umode_access_trap);
bool test_pmf_csr_11_umode_access_trap(void)
{
    TEST_BEGIN("PMF-CSR-11: U-mode access mcyclecfg triggers exception");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    if (!has_umode())
        TEST_SKIP("U-mode not supported");

    /* Switch to U-mode and try to read mcyclecfg */
    goto_priv(PRIV_U);
    trap_expect_begin();
    asm volatile("csrr x0, 0x321" ::: "memory");
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();
    goto_priv(PRIV_M);

    TEST_ASSERT("trap triggered in U-mode", trapped);
    TEST_ASSERT_EQ("cause is illegal instruction", cause, CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-12: RV32 mcyclecfgh access high 32 bits                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_12_rv32_mcyclecfgh);
bool test_pmf_csr_12_rv32_mcyclecfgh(void)
{
    TEST_BEGIN("PMF-CSR-12: RV32 mcyclecfgh access high 32 bits");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

#if __riscv_xlen != 32
    TEST_SKIP("RV32-only test");
#else
    /* Write MINH bit (bit 62 = bit 30 of high half) via mcyclecfgh */
    mcyclecfgh_write(0x40000000);  /* bit 30 = MINH in high half */
    uintptr_t val = mcyclecfgh_read();

    TEST_ASSERT("MINH writable via mcyclecfgh", (val & 0x40000000) != 0);

    mcyclecfgh_write(0);
    TEST_END();
#endif
}

/* ------------------------------------------------------------------ */
/* PMF-CSR-13: RV32 minstretcfgh access high 32 bits                 */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_13_rv32_minstretcfgh);
bool test_pmf_csr_13_rv32_minstretcfgh(void)
{
    TEST_BEGIN("PMF-CSR-13: RV32 minstretcfgh access high 32 bits");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

#if __riscv_xlen != 32
    TEST_SKIP("RV32-only test");
#else
    /* Write MINH bit via minstretcfgh */
    minstretcfgh_write(0x40000000);
    uintptr_t val = minstretcfgh_read();

    TEST_ASSERT("MINH writable via minstretcfgh", (val & 0x40000000) != 0);

    minstretcfgh_write(0);
    TEST_END();
#endif
}
