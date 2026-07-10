/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 2: hideleg[13]=0 Read-Only Zero Verification
 *
 * Tests LCFIDLG-RO-01 through RO-06 verify that when hideleg[13]=0,
 * vsip.LCOFIP and vsie.LCOFIE are read-only zero regardless of the
 * actual sip/sie LCOFI state.
 *
 * Spec reference:
 *   norm:vsip_vsie_lcofi (R2): When hideleg[13]=0, vsip.LCOFIP and
 *   vsie.LCOFIE are read-only zero.
 */

/* ------------------------------------------------------------------
 * LCFIDLG-RO-01 ~ RO-06: vsip/vsie LCOFI bits read-only zero
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_ro_vsip_lcofip);
bool test_shlcofideleg_ro_vsip_lcofip(void) {
    TEST_BEGIN("LCFIDLG-RO: vsip.LCOFIP read-only zero when hideleg[13]=0");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_mip = CSRR(mip);

    /* Prerequisite: mideleg[13]=1 so sip.LCOFIP is visible in HS-mode. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    /* Set hideleg[13]=0. */
    hideleg_write(saved_hideleg & ~LCOFI_BIT);

    /* RO-01: vsip.LCOFIP reads 0. */
    TEST_ASSERT("vsip.LCOFIP reads 0",
                (csr_read(CSR_VSIP) & LCOFI_BIT) == 0);

    /* RO-03: Write vsip bit 13 should have no effect. */
    csr_write(CSR_VSIP, csr_read(CSR_VSIP) | LCOFI_BIT);
    TEST_ASSERT("vsip.LCOFIP stays 0 after write",
                (csr_read(CSR_VSIP) & LCOFI_BIT) == 0);

    /* RO-05: Even when mip.LCOFIP=1, vsip still reads 0. */
    CSRS(mip, MIP_LCOFIP);
    TEST_ASSERT("vsip.LCOFIP still 0 even when sip.LCOFIP=1",
                (csr_read(CSR_VSIP) & LCOFI_BIT) == 0);

    /* Restore. */
    CSRC(mip, MIP_LCOFIP);
    CSRW(mip, saved_mip);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

TEST_REGISTER(test_shlcofideleg_ro_vsie_lcofie);
bool test_shlcofideleg_ro_vsie_lcofie(void) {
    TEST_BEGIN("LCFIDLG-RO: vsie.LCOFIE read-only zero when hideleg[13]=0");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_sie = csr_read(CSR_SIE);

    /* Prerequisite: mideleg[13]=1 so sie.LCOFIE is visible in HS-mode. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    /* Set hideleg[13]=0. */
    hideleg_write(saved_hideleg & ~LCOFI_BIT);

    /* RO-02: vsie.LCOFIE reads 0. */
    TEST_ASSERT("vsie.LCOFIE reads 0",
                (csr_read(CSR_VSIE) & LCOFI_BIT) == 0);

    /* RO-04: Write vsie bit 13 should have no effect. */
    csr_write(CSR_VSIE, csr_read(CSR_VSIE) | LCOFI_BIT);
    TEST_ASSERT("vsie.LCOFIE stays 0 after write",
                (csr_read(CSR_VSIE) & LCOFI_BIT) == 0);

    /* RO-06: Even when sie.LCOFIE=1, vsie still reads 0. */
    csr_write(CSR_SIE, saved_sie | LCOFI_BIT);
    TEST_ASSERT("vsie.LCOFIE still 0 even when sie.LCOFIE=1",
                (csr_read(CSR_VSIE) & LCOFI_BIT) == 0);

    /* Restore. */
    csr_write(CSR_SIE, saved_sie);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}
