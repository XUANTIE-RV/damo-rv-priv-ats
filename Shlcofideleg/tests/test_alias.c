/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 3: hideleg[13]=1 Alias Verification
 *
 * Tests LCFIDLG-ALIAS-01 through ALIAS-07 verify that when hideleg[13]=1,
 * vsip.LCOFIP is an alias of sip.LCOFIP, and vsie.LCOFIE is an alias
 * of sie.LCOFIE.
 *
 * Spec reference:
 *   norm:vsip_vsie_lcofi (R3): When hideleg[13]=1, vsip.LCOFIP is an
 *   alias of sip.LCOFIP, and vsie.LCOFIE is an alias of sie.LCOFIE.
 *
 * Note: LCOFIP is a pending bit set by hardware (counter overflow).
 * In tests we inject it via M-mode CSRS(mip, MIP_LCOFIP). The reverse
 * alias (vsip→sip write) does not apply to LCOFIP because it is
 * read-only from S-mode. Only LCOFIE (enable bit) supports bidirectional
 * alias testing.
 *
 * Prerequisite for all tests: mideleg[13]=1 is required because when
 * mideleg[13]=0, sip.LCOFIP and sie.LCOFIE are read-only zero.
 */

/* ------------------------------------------------------------------
 * LCFIDLG-ALIAS-01/02: sip.LCOFIP → vsip.LCOFIP forward alias
 *
 * Known QEMU limitation: QEMU does not implement the vsip.LCOFIP
 * alias to sip.LCOFIP even when hideleg[13]=1. The sip.LCOFIP value
 * is correct (verified via csr_read), but vsip.LCOFIP reads as 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_alias_lcofip_sip_to_vsip);
bool test_shlcofideleg_alias_lcofip_sip_to_vsip(void) {
    TEST_BEGIN("LCFIDLG-ALIAS: sip.LCOFIP → vsip.LCOFIP alias");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);

    /* Prerequisite: mideleg[13]=1, hideleg[13]=1. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg | LCOFI_BIT);

    /* ALIAS-01: M-mode sets LCOFIP=1, verify vsip sees it. */
    CSRS(mip, MIP_LCOFIP);
    TEST_ASSERT("vsip.LCOFIP reflects sip.LCOFIP=1",
                (csr_read(CSR_VSIP) & LCOFI_BIT) != 0);

    /* ALIAS-02: M-mode clears LCOFIP=0, verify vsip also clears. */
    CSRC(mip, MIP_LCOFIP);
    TEST_ASSERT("vsip.LCOFIP reflects sip.LCOFIP=0",
                (csr_read(CSR_VSIP) & LCOFI_BIT) == 0);

    /* Restore. */
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * LCFIDLG-ALIAS-03 ~ ALIAS-06: sie.LCOFIE ↔ vsie.LCOFIE bidirectional
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_alias_lcofie_bidirectional);
bool test_shlcofideleg_alias_lcofie_bidirectional(void) {
    TEST_BEGIN("LCFIDLG-ALIAS: sie.LCOFIE ↔ vsie.LCOFIE bidirectional alias");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_sie = csr_read(CSR_SIE);

    /* Prerequisite: mideleg[13]=1 (sie.LCOFIE is RO-zero when mideleg[13]=0),
     * hideleg[13]=1 (vsie aliases sie). */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg | LCOFI_BIT);

    /* ALIAS-03: sie→vsie forward set. */
    csr_write(CSR_SIE, csr_read(CSR_SIE) | LCOFI_BIT);
    TEST_ASSERT("vsie.LCOFIE=1 when sie.LCOFIE=1",
                (csr_read(CSR_VSIE) & LCOFI_BIT) != 0);

    /* ALIAS-04: sie→vsie forward clear. */
    csr_write(CSR_SIE, csr_read(CSR_SIE) & ~LCOFI_BIT);
    TEST_ASSERT("vsie.LCOFIE=0 when sie.LCOFIE=0",
                (csr_read(CSR_VSIE) & LCOFI_BIT) == 0);

    /* ALIAS-05: vsie→sie reverse set. */
    csr_write(CSR_VSIE, csr_read(CSR_VSIE) | LCOFI_BIT);
    TEST_ASSERT("sie.LCOFIE=1 when vsie.LCOFIE set",
                (csr_read(CSR_SIE) & LCOFI_BIT) != 0);

    /* ALIAS-06: vsie→sie reverse clear. */
    csr_write(CSR_VSIE, csr_read(CSR_VSIE) & ~LCOFI_BIT);
    TEST_ASSERT("sie.LCOFIE=0 when vsie.LCOFIE cleared",
                (csr_read(CSR_SIE) & LCOFI_BIT) == 0);

    /* Restore. */
    csr_write(CSR_SIE, saved_sie);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * LCFIDLG-ALIAS-07: mip.LCOFIP → vsip propagation
 *
 * Known QEMU limitation: same as ALIAS-01, vsip.LCOFIP does not
 * alias sip.LCOFIP even when hideleg[13]=1.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_shlcofideleg_alias_mip_to_vsip);
bool test_shlcofideleg_alias_mip_to_vsip(void) {
    TEST_BEGIN("LCFIDLG-ALIAS: mip.LCOFIP → vsip.LCOFIP propagation");

    if (!shlcofideleg_check_available()) {
        TEST_SKIP("Shlcofideleg not implemented");
    }

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);

    /* Prerequisite: mideleg[13]=1, hideleg[13]=1. */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg | LCOFI_BIT);

    /* ALIAS-07: Inject LCOFIP via mip, verify vsip sees it. */
    CSRS(mip, MIP_LCOFIP);

    /* Verify sip.LCOFIP is visible first (mideleg[13]=1). */
    TEST_ASSERT("sip.LCOFIP=1 after mip injection",
                (csr_read(CSR_SIP) & LCOFI_BIT) != 0);

    /* Verify vsip.LCOFIP is visible (hideleg[13]=1). */
    TEST_ASSERT("vsip.LCOFIP=1 via mip→sip→vsip chain",
                (csr_read(CSR_VSIP) & LCOFI_BIT) != 0);

    /* Restore. */
    CSRC(mip, MIP_LCOFIP);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}
