/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 4: VS-mode End-to-End Verification
 *
 * Tests LCFIDLG-VS-01 through VS-06 verify the end-to-end behavior
 * from VS-mode perspective: when V=1, csrr sip/sie actually accesses
 * vsip/vsie, and hideleg[13] controls LCOFI bit visibility.
 *
 * Spec references:
 *   norm:vsip_vsie_sz_acc_op: V=1 substitution (sip→vsip, sie→vsie)
 *   norm:vsip_vsie_lcofi (R2)(R3): hideleg[13] controls visibility
 */

/* ------------------------------------------------------------------
 * LCFIDLG-VS-01: VS-mode sees LCOFIP when hideleg[13]=1
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_vs_lcofip_visible);
bool test_shlcofideleg_vs_lcofip_visible(void) {
    TEST_BEGIN("LCFIDLG-VS-01: VS-mode sees LCOFIP when hideleg[13]=1");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);

    /* Prerequisite: mideleg[13]=1, hideleg[13]=1, inject LCOFIP. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg | LCOFI_BIT);
    CSRS(mip, MIP_LCOFIP);

    /* VS-mode reads sip (actually vsip in V=1), should see bit 13 = 1. */
    uintptr_t vs_sip = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT("VS-mode sees LCOFIP=1 via sip",
                (vs_sip & LCOFI_BIT) != 0);

    /* Restore. */
    CSRC(mip, MIP_LCOFIP);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * LCFIDLG-VS-02: VS-mode cannot see LCOFIP when hideleg[13]=0
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_vs_lcofip_hidden);
bool test_shlcofideleg_vs_lcofip_hidden(void) {
    TEST_BEGIN("LCFIDLG-VS-02: VS-mode cannot see LCOFIP when hideleg[13]=0");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);

    /* mideleg[13]=1, hideleg[13]=0, inject LCOFIP. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg & ~LCOFI_BIT);
    CSRS(mip, MIP_LCOFIP);

    /* VS-mode reads sip (actually vsip in V=1), should see bit 13 = 0. */
    uintptr_t vs_sip = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT("VS-mode sees LCOFIP=0 via sip",
                (vs_sip & LCOFI_BIT) == 0);

    /* Restore. */
    CSRC(mip, MIP_LCOFIP);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * LCFIDLG-VS-03: VS-mode sees LCOFIE when hideleg[13]=1
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_vs_lcofie_visible);
bool test_shlcofideleg_vs_lcofie_visible(void) {
    TEST_BEGIN("LCFIDLG-VS-03: VS-mode sees LCOFIE when hideleg[13]=1");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_sie = csr_read(CSR_SIE);

    /* Prerequisite: mideleg[13]=1 (sie.LCOFIE is RO-zero otherwise),
     * hideleg[13]=1, set sie.LCOFIE=1. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg | LCOFI_BIT);
    csr_write(CSR_SIE, saved_sie | LCOFI_BIT);

    /* VS-mode reads sie (actually vsie in V=1), should see bit 13 = 1. */
    uintptr_t vs_sie = run_in_vs_mode(vs_read_sie, 0);
    TEST_ASSERT("VS-mode sees LCOFIE=1 via sie",
                (vs_sie & LCOFI_BIT) != 0);

    /* Restore. */
    csr_write(CSR_SIE, saved_sie);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * LCFIDLG-VS-04: VS-mode cannot see LCOFIE when hideleg[13]=0
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_vs_lcofie_hidden);
bool test_shlcofideleg_vs_lcofie_hidden(void) {
    TEST_BEGIN("LCFIDLG-VS-04: VS-mode cannot see LCOFIE when hideleg[13]=0");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_sie = csr_read(CSR_SIE);

    /* mideleg[13]=1, hideleg[13]=0, set sie.LCOFIE=1. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg & ~LCOFI_BIT);
    csr_write(CSR_SIE, saved_sie | LCOFI_BIT);

    /* VS-mode reads sie (actually vsie in V=1), should see bit 13 = 0. */
    uintptr_t vs_sie = run_in_vs_mode(vs_read_sie, 0);
    TEST_ASSERT("VS-mode sees LCOFIE=0 via sie",
                (vs_sie & LCOFI_BIT) == 0);

    /* Restore. */
    csr_write(CSR_SIE, saved_sie);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * LCFIDLG-VS-05: VS-mode write sie.LCOFIE propagates (hideleg[13]=1)
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_vs_write_sie_delegated);
bool test_shlcofideleg_vs_write_sie_delegated(void) {
    TEST_BEGIN("LCFIDLG-VS-05: VS-mode write sie.LCOFIE propagates when hideleg[13]=1");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_sie = csr_read(CSR_SIE);

    /* Prerequisite: mideleg[13]=1, hideleg[13]=1, clear sie.LCOFIE first. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg | LCOFI_BIT);
    csr_write(CSR_SIE, saved_sie & ~LCOFI_BIT);

    /* VS-mode writes sie (actually vsie, aliased to sie). */
    run_in_vs_mode(vs_set_sie_lcofi, 0);

    /* HS-mode verifies sie.LCOFIE=1. */
    TEST_ASSERT("sie.LCOFIE=1 after VS-mode write",
                (csr_read(CSR_SIE) & LCOFI_BIT) != 0);

    /* Restore. */
    csr_write(CSR_SIE, saved_sie);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * LCFIDLG-VS-06: VS-mode write sie.LCOFIE has no effect (hideleg[13]=0)
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_vs_write_sie_not_delegated);
bool test_shlcofideleg_vs_write_sie_not_delegated(void) {
    TEST_BEGIN("LCFIDLG-VS-06: VS-mode write sie.LCOFIE no effect when hideleg[13]=0");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_sie = csr_read(CSR_SIE);

    /* mideleg[13]=1, hideleg[13]=0, clear sie.LCOFIE first. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg & ~LCOFI_BIT);
    csr_write(CSR_SIE, saved_sie & ~LCOFI_BIT);

    /* VS-mode writes sie (actually vsie, but vsie is RO-zero). */
    run_in_vs_mode(vs_set_sie_lcofi, 0);

    /* HS-mode verifies sie.LCOFIE is still 0 (vsie write had no effect). */
    TEST_ASSERT("sie.LCOFIE unchanged after VS-mode write when hideleg[13]=0",
                (csr_read(CSR_SIE) & LCOFI_BIT) == 0);

    /* Restore. */
    csr_write(CSR_SIE, saved_sie);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}
