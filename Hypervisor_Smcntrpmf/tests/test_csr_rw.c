/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_csr_rw.c - Group 16: VSINH/VUINH field constraint (H-dependent)
 *
 * PMF-CSR-05: VSINH/VUINH read-only zero when H ext not implemented.
 *
 * Migrated from Smcntrpmf_test_plan.md Group 1 (PMF-CSR-05).
 */

/* ------------------------------------------------------------------ */
/* PMF-CSR-05: VSINH/VUINH read-only zero without H extension        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_pmf_csr_05_vsinh_vuinh_no_h);
bool test_pmf_csr_05_vsinh_vuinh_no_h(void)
{
    TEST_BEGIN("PMF-CSR-05: VSINH/VUINH read-only zero without H ext");

    if (!smcntrpmf_implemented())
        TEST_SKIP("Smcntrpmf not implemented");

    /* This test only applies when the H extension is NOT implemented.
     * On an H-enabled platform VSINH/VUINH are writable and functional
     * (covered by PMF-CYC-08/09 and PMF-INS-06/07). */
    if (HAS_H_EXT())
        TEST_SKIP("H extension implemented; VSINH/VUINH may be writable");

    /* Without H extension, VSINH and VUINH must be read-only zero
     * (norm:unimplemented_mode_bits). */
    mcyclecfg_write(CYCLECFG_VSINH | CYCLECFG_VUINH | CYCLECFG_MINH);
    uintptr_t val = mcyclecfg_read();

    TEST_ASSERT("mcyclecfg VSINH read-only zero", (val & CYCLECFG_VSINH) == 0);
    TEST_ASSERT("mcyclecfg VUINH read-only zero", (val & CYCLECFG_VUINH) == 0);

    minstretcfg_write(CYCLECFG_VSINH | CYCLECFG_VUINH | CYCLECFG_MINH);
    val = minstretcfg_read();

    TEST_ASSERT("minstretcfg VSINH read-only zero", (val & CYCLECFG_VSINH) == 0);
    TEST_ASSERT("minstretcfg VUINH read-only zero", (val & CYCLECFG_VUINH) == 0);

    mcyclecfg_write(0);
    minstretcfg_write(0);
    TEST_END();
}
