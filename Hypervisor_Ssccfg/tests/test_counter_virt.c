/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_counter_virt.c - Group 11.1/11.2: scountovf and scountinhibit
 * virtualization in VS/VU-mode
 *
 * Tests HCROSS-SSCCFG-01 through HCROSS-SSCCFG-06
 * (migrated from SSCFG-OVF-01~04 in Ssccfg_test_plan.md; 05/06 are new
 * cases filling the gap for norm:ssccfg_illegal_scountinhibit_vs_vu).
 *
 * norm:ssccfg_virtual_scountovf_vs_vu:
 *   For implementations that support Smcdeleg/Ssccfg, Sscofpmf, and
 *   the H extension, when menvcfg.CDE=1, attempts to read scountovf
 *   from VS-mode or VU-mode raise a virtual-instruction exception.
 *
 * norm:ssccfg_illegal_scountinhibit_vs_vu:
 *   When Supervisor Counter Delegation is enabled, attempts to access
 *   scountinhibit from VS-mode or VU-mode raise a virtual-instruction
 *   exception.
 *
 * See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 11.
 */

#ifdef ENABLE_HYP

/* Common preconditions for the virtualization sub-groups:
 * H extension + Smcdeleg/Ssccfg with writable CDE. Returns false
 * (after recording the reason) when the test must be skipped. */
static bool counter_virt_preconditions(const char **skip_reason)
{
    if (!HAS_H_EXT()) {
        *skip_reason = "H extension not supported";
        return false;
    }
    if (!cde_settable()) {
        *skip_reason = "menvcfg.CDE not writable (counter delegation unavailable)";
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-01: VS-mode read scountovf (CDE=1) -> virtual-inst  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_01_vs_scountovf);
bool test_hcross_ssccfg_01_vs_scountovf(void) {
    TEST_BEGIN("HCROSS-SSCCFG-01: VS-mode scountovf read (CDE=1)");
    const char *skip;
    if (!counter_virt_preconditions(&skip)) TEST_SKIP(skip);
    if (!platform_has_sscofpmf()) TEST_SKIP("Sscofpmf not supported");

    stateen_allow_csrind();
    uintptr_t orig_cde = menvcfg_read();
    menvcfg_write(orig_cde | MENVCFG_CDE);

    TEST_VS_MODE_TRAP("VS-mode scountovf read virtual-inst",
                      _vs_read_scountovf, 0, CAUSE_VIRTUAL_INSTRUCTION);

    menvcfg_write(orig_cde);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-02: VU-mode read scountovf (CDE=1) -> virtual-inst  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_02_vu_scountovf);
bool test_hcross_ssccfg_02_vu_scountovf(void) {
    TEST_BEGIN("HCROSS-SSCCFG-02: VU-mode scountovf read (CDE=1)");
    const char *skip;
    if (!counter_virt_preconditions(&skip)) TEST_SKIP(skip);
    if (!platform_has_sscofpmf()) TEST_SKIP("Sscofpmf not supported");

    stateen_allow_csrind();
    uintptr_t orig_cde = menvcfg_read();
    menvcfg_write(orig_cde | MENVCFG_CDE);

    TEST_VU_MODE_TRAP("VU-mode scountovf read virtual-inst",
                      _vu_read_scountovf, 0, CAUSE_VIRTUAL_INSTRUCTION);

    menvcfg_write(orig_cde);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-03: HS-mode read scountovf (CDE=1) succeeds         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_03_hs_scountovf);
bool test_hcross_ssccfg_03_hs_scountovf(void) {
    TEST_BEGIN("HCROSS-SSCCFG-03: HS-mode scountovf read (CDE=1)");
    const char *skip;
    if (!counter_virt_preconditions(&skip)) TEST_SKIP(skip);
    if (!platform_has_sscofpmf()) TEST_SKIP("Sscofpmf not supported");

    stateen_allow_csrind();
    uintptr_t orig_cde = menvcfg_read();
    menvcfg_write(orig_cde | MENVCFG_CDE);

    /* HS-mode (V=0 S-mode) is NOT subject to the virtualization clause */
    goto_priv(PRIV_S);
    trap_expect_begin();
    scountovf_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    goto_priv(PRIV_M);
    TEST_ASSERT("HS-mode scountovf read no trap", !trapped);

    menvcfg_write(orig_cde);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-04: VS-mode read scountovf (CDE=0) not virtualized  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_04_vs_scountovf_cde0);
bool test_hcross_ssccfg_04_vs_scountovf_cde0(void) {
    TEST_BEGIN("HCROSS-SSCCFG-04: VS-mode scountovf read (CDE=0)");
    const char *skip;
    if (!counter_virt_preconditions(&skip)) TEST_SKIP(skip);
    if (!platform_has_sscofpmf()) TEST_SKIP("Sscofpmf not supported");

    stateen_allow_csrind();
    uintptr_t orig_cde = menvcfg_read();
    menvcfg_write(orig_cde & ~MENVCFG_CDE);

    /* CDE=0: the virtualization clause does not apply; the access
     * follows Sscofpmf base rules and must NOT raise
     * virtual-instruction because of this clause. */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_scountovf, 0);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trapped ? trap_get_cause() : 0;
    trap_expect_end();
    if (trapped) {
        TEST_ASSERT("CDE=0 VS read must not virtual-inst",
                    cause != CAUSE_VIRTUAL_INSTRUCTION);
    }

    menvcfg_write(orig_cde);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-05: VS-mode scountinhibit (CDE=1) -> virtual-inst   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_05_vs_scountinhibit);
bool test_hcross_ssccfg_05_vs_scountinhibit(void) {
    TEST_BEGIN("HCROSS-SSCCFG-05: VS-mode scountinhibit access (CDE=1)");
    const char *skip;
    if (!counter_virt_preconditions(&skip)) TEST_SKIP(skip);

    stateen_allow_csrind();
    uintptr_t orig_cde = menvcfg_read();
    menvcfg_write(orig_cde | MENVCFG_CDE);

    TEST_VS_MODE_TRAP("VS-mode scountinhibit read virtual-inst",
                      _vs_read_scountinhibit, 0, CAUSE_VIRTUAL_INSTRUCTION);

    menvcfg_write(orig_cde);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-06: VU-mode scountinhibit (CDE=1) -> virtual-inst   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_06_vu_scountinhibit);
bool test_hcross_ssccfg_06_vu_scountinhibit(void) {
    TEST_BEGIN("HCROSS-SSCCFG-06: VU-mode scountinhibit access (CDE=1)");
    const char *skip;
    if (!counter_virt_preconditions(&skip)) TEST_SKIP(skip);

    stateen_allow_csrind();
    uintptr_t orig_cde = menvcfg_read();
    menvcfg_write(orig_cde | MENVCFG_CDE);

    TEST_VU_MODE_TRAP("VU-mode scountinhibit read virtual-inst",
                      _vu_read_scountinhibit, 0, CAUSE_VIRTUAL_INSTRUCTION);

    menvcfg_write(orig_cde);
    HYP_TEST_END();
}

#endif /* ENABLE_HYP */
