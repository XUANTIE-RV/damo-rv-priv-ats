/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 5: hideleg Dynamic Toggle Consistency
 *
 * Tests LCFIDLG-DYN-01 through DYN-04 verify that toggling hideleg[13]
 * between 0 and 1 immediately and correctly changes vsip/vsie LCOFI
 * bit behavior.
 *
 * Spec reference:
 *   norm:vsip_vsie_lcofi (R2)(R3): hideleg[13] changes should be
 *   immediately reflected in vsip/vsie behavior.
 */

/* ------------------------------------------------------------------
 * LCFIDLG-DYN-01 ~ DYN-04: hideleg[13] dynamic toggle
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_dynamic_toggle);
bool test_shlcofideleg_dynamic_toggle(void) {
    TEST_BEGIN("LCFIDLG-DYN: hideleg[13] dynamic toggle consistency");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_sie = csr_read(CSR_SIE);

    /* Prerequisites: mideleg[13]=1, inject LCOFIP, enable LCOFIE. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    CSRS(mip, MIP_LCOFIP);
    csr_write(CSR_SIE, saved_sie | LCOFI_BIT);

    /* DYN-01: hideleg 1→0, vsip.LCOFIP becomes 0. */
    hideleg_write(saved_hideleg | LCOFI_BIT);
    TEST_ASSERT("vsip.LCOFIP=1 when hideleg[13]=1",
                (csr_read(CSR_VSIP) & LCOFI_BIT) != 0);

    hideleg_write(saved_hideleg & ~LCOFI_BIT);
    TEST_ASSERT("vsip.LCOFIP=0 after hideleg[13] cleared",
                (csr_read(CSR_VSIP) & LCOFI_BIT) == 0);

    /* DYN-02: hideleg 0→1, vsip.LCOFIP becomes visible. */
    hideleg_write(saved_hideleg | LCOFI_BIT);
    TEST_ASSERT("vsip.LCOFIP=1 after hideleg[13] re-set",
                (csr_read(CSR_VSIP) & LCOFI_BIT) != 0);

    /* DYN-03: vsie.LCOFIE repeated toggle. */
    for (int i = 0; i < 4; i++) {
        bool delegated = (i % 2 == 0);

        if (delegated)
            hideleg_write(hideleg_read() | LCOFI_BIT);
        else
            hideleg_write(hideleg_read() & ~LCOFI_BIT);

        uintptr_t vsie_lcofie = csr_read(CSR_VSIE) & LCOFI_BIT;

        if (delegated) {
            TEST_ASSERT("vsie.LCOFIE visible after toggle",
                        vsie_lcofie != 0);
        } else {
            TEST_ASSERT("vsie.LCOFIE hidden after toggle",
                        vsie_lcofie == 0);
        }
    }

    /* DYN-04: sip.LCOFIP is unaffected by hideleg toggle. */
    hideleg_write(saved_hideleg | LCOFI_BIT);
    uintptr_t sip_before = csr_read(CSR_SIP) & LCOFI_BIT;
    hideleg_write(saved_hideleg & ~LCOFI_BIT);
    uintptr_t sip_after = csr_read(CSR_SIP) & LCOFI_BIT;
    TEST_ASSERT("sip.LCOFIP unchanged after hideleg toggle",
                sip_before == sip_after);

    /* Restore. */
    CSRC(mip, MIP_LCOFIP);
    csr_write(CSR_SIE, saved_sie);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}
