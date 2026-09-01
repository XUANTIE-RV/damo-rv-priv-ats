/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_vsireg_rules.c - Group 11.4: vsiselect/vsireg* multi-privilege
 * access rules in the delegated-counter range (0x40-0x5F)
 *
 * Tests HCROSS-SSCCFG-12 through HCROSS-SSCCFG-21
 * (migrated from SSCFG-HYP-01~10 in Ssccfg_test_plan.md).
 *
 * norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal:
 *   With the H extension, VS/VU-mode direct access to vsiselect or
 *   vsireg*, or VU-mode access to siselect or sireg*, raises a
 *   virtual-instruction exception.
 * norm:ssccfg_hyp_m_s_vsireg_illegal:
 *   While vsiselect holds 0x40-0x5F, M/S-mode access to vsireg*
 *   raises an illegal-instruction exception.
 * norm:ssccfg_hyp_vs_access_sireg_conditional:
 *   VS-mode access to sireg* (really vsireg*) raises illegal-inst if
 *   menvcfg.CDE=0, or virtual-inst if menvcfg.CDE=1.
 *
 * See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 11.
 */

#ifdef ENABLE_HYP

/* Preconditions: H extension + Smcdeleg/Ssccfg presence */
static bool vsireg_preconditions(const char **skip_reason)
{
    if (!HAS_H_EXT()) {
        *skip_reason = "H extension not supported";
        return false;
    }
    trap_expect_begin();
    vsiselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    if (trapped) {
        *skip_reason = "vsiselect inaccessible (Sscsrind/Ssccfg unavailable)";
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-12: VS-mode direct vsiselect -> virtual-inst        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_12_vs_vsiselect);
bool test_hcross_ssccfg_12_vs_vsiselect(void) {
    TEST_BEGIN("HCROSS-SSCCFG-12: VS-mode direct vsiselect access");
    const char *skip;
    if (!vsireg_preconditions(&skip)) TEST_SKIP(skip);

    stateen_allow_csrind();
    vsiselect_write(SISELECT_DELEG_BASE);
    TEST_VS_MODE_TRAP("VS direct vsiselect virtual-inst",
                      _vs_read_vsiselect_direct, 0, CAUSE_VIRTUAL_INSTRUCTION);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-13: VS-mode direct vsireg -> virtual-inst           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_13_vs_vsireg);
bool test_hcross_ssccfg_13_vs_vsireg(void) {
    TEST_BEGIN("HCROSS-SSCCFG-13: VS-mode direct vsireg access");
    const char *skip;
    if (!vsireg_preconditions(&skip)) TEST_SKIP(skip);

    stateen_allow_csrind();
    vsiselect_write(SISELECT_DELEG_BASE);
    TEST_VS_MODE_TRAP("VS direct vsireg virtual-inst",
                      _vs_read_vsireg_direct, 0, CAUSE_VIRTUAL_INSTRUCTION);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-14: VU-mode direct vsiselect -> virtual-inst        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_14_vu_vsiselect);
bool test_hcross_ssccfg_14_vu_vsiselect(void) {
    TEST_BEGIN("HCROSS-SSCCFG-14: VU-mode direct vsiselect access");
    const char *skip;
    if (!vsireg_preconditions(&skip)) TEST_SKIP(skip);

    stateen_allow_csrind();
    TEST_VU_MODE_TRAP("VU direct vsiselect virtual-inst",
                      _vu_read_vsiselect_direct, 0, CAUSE_VIRTUAL_INSTRUCTION);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-15: VU-mode direct vsireg -> virtual-inst           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_15_vu_vsireg);
bool test_hcross_ssccfg_15_vu_vsireg(void) {
    TEST_BEGIN("HCROSS-SSCCFG-15: VU-mode direct vsireg access");
    const char *skip;
    if (!vsireg_preconditions(&skip)) TEST_SKIP(skip);

    stateen_allow_csrind();
    TEST_VU_MODE_TRAP("VU direct vsireg virtual-inst",
                      _vu_read_vsireg_direct, 0, CAUSE_VIRTUAL_INSTRUCTION);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-16: VU-mode siselect -> virtual-inst                */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_16_vu_siselect);
bool test_hcross_ssccfg_16_vu_siselect(void) {
    TEST_BEGIN("HCROSS-SSCCFG-16: VU-mode siselect access");
    const char *skip;
    if (!vsireg_preconditions(&skip)) TEST_SKIP(skip);

    stateen_allow_csrind();
    /* Cause ambiguity: Ssccfg "V=1" wording implies virtual-inst, but
     * the base rule (VU accessing an S-level CSR) gives illegal-inst.
     * Both are accepted, as in SRMCFG-21 (Hypervisor_Ss Group 9). */
    TEST_VU_MODE_TRAP_ILLEGAL_OR_VIRTUAL("VU siselect trap",
                                         _vu_read_siselect, 0);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-17: VU-mode sireg -> virtual-inst                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_17_vu_sireg);
bool test_hcross_ssccfg_17_vu_sireg(void) {
    TEST_BEGIN("HCROSS-SSCCFG-17: VU-mode sireg access");
    const char *skip;
    if (!vsireg_preconditions(&skip)) TEST_SKIP(skip);

    stateen_allow_csrind();
    /* Same cause ambiguity as HCROSS-SSCCFG-16, see comment there. */
    TEST_VU_MODE_TRAP_ILLEGAL_OR_VIRTUAL("VU sireg trap",
                                         _vu_read_sireg, 0);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-18: M-mode vsireg (vsiselect 0x40-0x5F) -> illegal  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_18_mmode_vsireg);
bool test_hcross_ssccfg_18_mmode_vsireg(void) {
    TEST_BEGIN("HCROSS-SSCCFG-18: M-mode vsireg access in deleg range");
    const char *skip;
    if (!vsireg_preconditions(&skip)) TEST_SKIP(skip);

    uintptr_t orig = vsiselect_read();
    vsiselect_write(SISELECT_DELEG_BASE);

    /* M-mode must NOT directly access vsireg* while vsiselect is in
     * the delegated-counter range. */
    EXPECT_ILLEGAL_INST(vsireg_read());

    vsiselect_write(orig);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-19: HS-mode vsireg (vsiselect 0x40-0x5F) -> illegal */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_19_hsmode_vsireg);
bool test_hcross_ssccfg_19_hsmode_vsireg(void) {
    TEST_BEGIN("HCROSS-SSCCFG-19: HS-mode vsireg access in deleg range");
    const char *skip;
    if (!vsireg_preconditions(&skip)) TEST_SKIP(skip);

    stateen_allow_csrind();
    uintptr_t orig = vsiselect_read();
    vsiselect_write(SISELECT_DELEG_BASE);

    /* HS-mode (V=0 S-mode) is equally restricted. */
    goto_priv(PRIV_S);
    EXPECT_ILLEGAL_INST(vsireg_read());
    goto_priv(PRIV_M);

    vsiselect_write(orig);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-20: VS via sireg* (CDE=0) -> illegal-inst           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_20_vs_sireg_cde0);
bool test_hcross_ssccfg_20_vs_sireg_cde0(void) {
    TEST_BEGIN("HCROSS-SSCCFG-20: VS sireg* access (CDE=0) illegal");
    const char *skip;
    if (!vsireg_preconditions(&skip)) TEST_SKIP(skip);
    if (!cde_settable()) TEST_SKIP("menvcfg.CDE not writable");

    stateen_allow_csrind();
    uintptr_t orig_cde = menvcfg_read();
    menvcfg_write(orig_cde & ~MENVCFG_CDE);

    TEST_VS_MODE_TRAP("VS sireg (CDE=0) illegal-inst",
                      _vs_read_sireg_deleg, SISELECT_DELEG_BASE,
                      CAUSE_ILLEGAL_INST);

    menvcfg_write(orig_cde);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-21: VS via sireg* (CDE=1) -> virtual-inst           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_21_vs_sireg_cde1);
bool test_hcross_ssccfg_21_vs_sireg_cde1(void) {
    TEST_BEGIN("HCROSS-SSCCFG-21: VS sireg* access (CDE=1) virtual-inst");
    const char *skip;
    if (!vsireg_preconditions(&skip)) TEST_SKIP(skip);
    if (!cde_settable()) TEST_SKIP("menvcfg.CDE not writable");

    stateen_allow_csrind();
    uintptr_t orig_cde = menvcfg_read();
    menvcfg_write(orig_cde | MENVCFG_CDE);

    TEST_VS_MODE_TRAP("VS sireg (CDE=1) virtual-inst",
                      _vs_read_sireg_deleg, SISELECT_DELEG_BASE,
                      CAUSE_VIRTUAL_INSTRUCTION);

    menvcfg_write(orig_cde);
    HYP_TEST_END();
}

#endif /* ENABLE_HYP */
