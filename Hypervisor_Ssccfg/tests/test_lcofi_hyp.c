/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_lcofi_hyp.c - Group 11.3: LCOFI virtualization (hvip/hvien bit 13)
 *
 * Tests HCROSS-SSCCFG-07 through HCROSS-SSCCFG-11
 * (migrated from SSCFG-HLCOFI-01~05 in Ssccfg_test_plan.md).
 *
 * norm:ssccfg_lcofi_hvip_hvien:
 *   For implementations that support Smcdeleg/Ssccfg, Sscofpmf,
 *   Smaia/Ssaia, and the H extension, the LCOFI bit (bit 13) in each
 *   of hvip and hvien is implemented and writable. By virtue of
 *   implementing hvip.LCOFI, the LCOFI bit in each of vsie and vsip
 *   is also implemented.
 *
 * See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 11.
 */

#ifdef ENABLE_HYP

/* Preconditions: H + Smcdeleg/Ssccfg + Sscofpmf + Smaia/Ssaia */
static bool lcofi_preconditions(const char **skip_reason)
{
    if (!HAS_H_EXT()) {
        *skip_reason = "H extension not supported";
        return false;
    }
    if (!cde_settable()) {
        *skip_reason = "menvcfg.CDE not writable (Smcdeleg/Ssccfg unavailable)";
        return false;
    }
    if (!platform_has_sscofpmf()) {
        *skip_reason = "Sscofpmf not supported";
        return false;
    }
    if (!platform_has_hvien()) {
        *skip_reason = "Smaia/Ssaia not supported (hvien inaccessible)";
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-07: hvip bit 13 (LCOFI) writable                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_07_hvip_lcofi_rw);
bool test_hcross_ssccfg_07_hvip_lcofi_rw(void) {
    TEST_BEGIN("HCROSS-SSCCFG-07: hvip bit 13 (LCOFI) writable");
    const char *skip;
    if (!lcofi_preconditions(&skip)) TEST_SKIP(skip);

    uintptr_t orig = hvip_read();

    hvip_write(orig | LCOFI_BIT);
    uintptr_t rb1 = hvip_read();
    TEST_ASSERT("hvip bit 13 set to 1", (rb1 & LCOFI_BIT) != 0);

    hvip_write(orig & ~LCOFI_BIT);
    uintptr_t rb0 = hvip_read();
    TEST_ASSERT("hvip bit 13 cleared to 0", (rb0 & LCOFI_BIT) == 0);

    hvip_write(orig);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-08: hvien bit 13 (LCOFI) writable                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_08_hvien_lcofi_rw);
bool test_hcross_ssccfg_08_hvien_lcofi_rw(void) {
    TEST_BEGIN("HCROSS-SSCCFG-08: hvien bit 13 (LCOFI) writable");
    const char *skip;
    if (!lcofi_preconditions(&skip)) TEST_SKIP(skip);

    uintptr_t orig = hvien_read();

    hvien_write(orig | LCOFI_BIT);
    uintptr_t rb1 = hvien_read();
    TEST_ASSERT("hvien bit 13 set to 1", (rb1 & LCOFI_BIT) != 0);

    hvien_write(orig & ~LCOFI_BIT);
    uintptr_t rb0 = hvien_read();
    TEST_ASSERT("hvien bit 13 cleared to 0", (rb0 & LCOFI_BIT) == 0);

    hvien_write(orig);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-09: hvip.LCOFI independent verification             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_09_hvip_all_ones);
bool test_hcross_ssccfg_09_hvip_all_ones(void) {
    TEST_BEGIN("HCROSS-SSCCFG-09: hvip.LCOFI with all-ones write");
    const char *skip;
    if (!lcofi_preconditions(&skip)) TEST_SKIP(skip);

    uintptr_t orig = hvip_read();

    /* Write all writable bits; LCOFI must come back as 1 */
    hvip_write((uintptr_t)-1);
    uintptr_t rb = hvip_read();
    TEST_ASSERT("hvip bit 13 readable as 1 after all-ones write",
                (rb & LCOFI_BIT) != 0);

    hvip_write(orig);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-10: hvien.LCOFI independent verification            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_10_hvien_all_ones);
bool test_hcross_ssccfg_10_hvien_all_ones(void) {
    TEST_BEGIN("HCROSS-SSCCFG-10: hvien.LCOFI with all-ones write");
    const char *skip;
    if (!lcofi_preconditions(&skip)) TEST_SKIP(skip);

    uintptr_t orig = hvien_read();

    hvien_write((uintptr_t)-1);
    uintptr_t rb = hvien_read();
    TEST_ASSERT("hvien bit 13 readable as 1 after all-ones write",
                (rb & LCOFI_BIT) != 0);

    hvien_write(orig);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-11: vsie/vsip LCOFI bits implicitly implemented     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_11_vsie_vsip_lcofi);
bool test_hcross_ssccfg_11_vsie_vsip_lcofi(void) {
    TEST_BEGIN("HCROSS-SSCCFG-11: vsie/vsip LCOFI bits exist");
    const char *skip;
    if (!lcofi_preconditions(&skip)) TEST_SKIP(skip);

    /* Implementing hvip.LCOFI implies vsie/vsip bit 13 exist:
     * reading them must not trap, and vsie bit 13 must be writable
     * (vsie is the guest-visible enable for the virtual LCOFI). */
    trap_expect_begin();
    uintptr_t vsip_val = vsip_read();
    bool vsip_trapped = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("vsip read no trap (bit 13 implemented)", !vsip_trapped);
    (void)vsip_val;

    uintptr_t orig_vsie = vsie_read();
    trap_expect_begin();
    vsie_write(orig_vsie | LCOFI_BIT);
    bool vsie_trapped = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("vsie bit 13 write no trap", !vsie_trapped);
    uintptr_t rb = vsie_read();
    TEST_ASSERT("vsie bit 13 writable", (rb & LCOFI_BIT) != 0);
    vsie_write(orig_vsie);

    HYP_TEST_END();
}

#endif /* ENABLE_HYP */
