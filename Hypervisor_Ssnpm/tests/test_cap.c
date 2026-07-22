/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_cap.c - Group 1: Capability detection and H-level PM CSR control
 *
 * HZPM-CAP-01 ~ HZPM-CAP-10
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 1.
 *
 * Spec references:
 *   - norm:henvcfg_pmm_op (hypervisor.adoc): henvcfg.PMM controls
 *     VS-mode PM iff Ssnpm implemented; read-only zero otherwise.
 *   - sec:hstatus HUPMM paragraph (hypervisor.adoc): hstatus.HUPMM
 *     controls PM for HLV/HSV in U-mode as VU; read-only zero without
 *     Ssnpm.
 *   - norm:pmlen_supported_values / norm:pmlen_illegal_warl (zpm.adoc):
 *     PMM encodings 00/10/11 legal, 01 reserved, WARL semantics.
 *   - norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0 (hypervisor.adoc):
 *     V=1 access to hypervisor CSRs raises virtual-instruction.
 */

/* ----- HZPM-CAP-01: probe henvcfg.PMM implementation ----- */
TEST_REGISTER(test_hzpm_cap_01);
bool test_hzpm_cap_01(void) {
    TEST_BEGIN("HZPM-CAP-01: probe henvcfg.PMM implementation");
    H_REQUIRED_OR_SKIP();

    bool ssnpm = detect_ssnpm();

    pm_set_vsmode(PMM_PMLEN16);
    unsigned rb16 = pm_get_vsmode();
    pm_set_vsmode(PMM_PMLEN7);
    unsigned rb7 = pm_get_vsmode();
    pm_set_vsmode(PMM_DISABLED);

    if (ssnpm) {
        /* norm:henvcfg_pmm_op: field must be writable when Ssnpm
         * is implemented (at least one PMLEN encoding accepted). */
        TEST_ASSERT("Ssnpm present: henvcfg.PMM writable",
                    rb16 == PMM_PMLEN16 || rb7 == PMM_PMLEN7);
    } else {
        /* norm:henvcfg_pmm_op: read-only zero without Ssnpm. */
        TEST_ASSERT_EQ("no Ssnpm: henvcfg.PMM read-only zero (rb16)",
                       rb16, PMM_DISABLED);
        TEST_ASSERT_EQ("no Ssnpm: henvcfg.PMM read-only zero (rb7)",
                       rb7, PMM_DISABLED);
    }
    HYP_TEST_END();
}

/* ----- HZPM-CAP-02: probe hstatus.HUPMM implementation ----- */
TEST_REGISTER(test_hzpm_cap_02);
bool test_hzpm_cap_02(void) {
    TEST_BEGIN("HZPM-CAP-02: probe hstatus.HUPMM implementation");
    H_REQUIRED_OR_SKIP();

    bool ssnpm = detect_ssnpm();

    pm_set_hupmm(PMM_PMLEN16);
    unsigned rb16 = pm_get_hupmm();
    pm_set_hupmm(PMM_PMLEN7);
    unsigned rb7 = pm_get_hupmm();
    pm_set_hupmm(PMM_DISABLED);

    if (ssnpm) {
        /* sec:hstatus HUPMM paragraph: writable when Ssnpm present. */
        TEST_ASSERT("Ssnpm present: hstatus.HUPMM writable",
                    rb16 == PMM_PMLEN16 || rb7 == PMM_PMLEN7);
    } else {
        TEST_ASSERT_EQ("no Ssnpm: hstatus.HUPMM read-only zero (rb16)",
                       rb16, PMM_DISABLED);
        TEST_ASSERT_EQ("no Ssnpm: hstatus.HUPMM read-only zero (rb7)",
                       rb7, PMM_DISABLED);
    }
    HYP_TEST_END();
}

/* ----- HZPM-CAP-03: henvcfg.PMM supported PMLEN values ----- */
TEST_REGISTER(test_hzpm_cap_03);
bool test_hzpm_cap_03(void) {
    TEST_BEGIN("HZPM-CAP-03: henvcfg.PMM supported PMLEN");
    SSNPM_HYP_REQUIRED_OR_SKIP();

    pm_set_vsmode(PMM_PMLEN7);
    bool pmlen7 = (pm_get_vsmode() == PMM_PMLEN7);
    pm_set_vsmode(PMM_PMLEN16);
    bool pmlen16 = (pm_get_vsmode() == PMM_PMLEN16);
    pm_set_vsmode(PMM_DISABLED);

    printf("  henvcfg.PMM: PMLEN=7 %s, PMLEN=16 %s\n",
           pmlen7 ? "supported" : "unsupported",
           pmlen16 ? "supported" : "unsupported");

    /* norm:pmlen_supported_values: at least one of PMLEN=7/16 must
     * be accepted for the field to be considered implemented. */
    TEST_ASSERT("at least one PMLEN supported", pmlen7 || pmlen16);
    HYP_TEST_END();
}

/* ----- HZPM-CAP-04: hstatus.HUPMM supported PMLEN values ----- */
TEST_REGISTER(test_hzpm_cap_04);
bool test_hzpm_cap_04(void) {
    TEST_BEGIN("HZPM-CAP-04: hstatus.HUPMM supported PMLEN");
    SSNPM_HYP_REQUIRED_OR_SKIP();

    pm_set_hupmm(PMM_PMLEN7);
    bool pmlen7 = (pm_get_hupmm() == PMM_PMLEN7);
    pm_set_hupmm(PMM_PMLEN16);
    bool pmlen16 = (pm_get_hupmm() == PMM_PMLEN16);
    pm_set_hupmm(PMM_DISABLED);

    printf("  hstatus.HUPMM: PMLEN=7 %s, PMLEN=16 %s\n",
           pmlen7 ? "supported" : "unsupported",
           pmlen16 ? "supported" : "unsupported");

    TEST_ASSERT("at least one PMLEN supported", pmlen7 || pmlen16);
    HYP_TEST_END();
}

/* ----- HZPM-CAP-05: henvcfg.PMM reserved encoding (0b01) WARL ----- */
TEST_REGISTER(test_hzpm_cap_05);
bool test_hzpm_cap_05(void) {
    TEST_BEGIN("HZPM-CAP-05: henvcfg.PMM reserved value rejected");
    SSNPM_HYP_REQUIRED_OR_SKIP();

    /* norm:pmlen_illegal_warl: writing the reserved encoding is an
     * illegal write and follows WARL semantics -- the field must end
     * up holding a *legal* value (00/10/11), never the reserved
     * encoding itself. The field is not required to stay unchanged. */
    pm_set_vsmode(PMM_DISABLED);
    pm_set_vsmode(PMM_RESERVED);
    unsigned rb0 = pm_get_vsmode();
    TEST_ASSERT_NEQ("reserved encoding never reads back (from 0)",
                    rb0, PMM_RESERVED);

    if (hzpm_try_set_vs_pmm(PMM_PMLEN7)) {
        pm_set_vsmode(PMM_RESERVED);
        unsigned rb7 = pm_get_vsmode();
        TEST_ASSERT_NEQ("reserved encoding never reads back (from PMLEN7)",
                        rb7, PMM_RESERVED);
    }

    pm_set_vsmode(PMM_DISABLED);
    HYP_TEST_END();
}

/* ----- HZPM-CAP-06: hstatus.HUPMM reserved encoding (0b01) WARL ----- */
TEST_REGISTER(test_hzpm_cap_06);
bool test_hzpm_cap_06(void) {
    TEST_BEGIN("HZPM-CAP-06: hstatus.HUPMM reserved value rejected");
    SSNPM_HYP_REQUIRED_OR_SKIP();

    /* norm:pmlen_illegal_warl: WARL semantics -- the field must
     * hold a legal value after the illegal write, never 0b01. */
    pm_set_hupmm(PMM_DISABLED);
    pm_set_hupmm(PMM_RESERVED);
    unsigned rb0 = pm_get_hupmm();
    TEST_ASSERT_NEQ("reserved encoding never reads back (from 0)",
                    rb0, PMM_RESERVED);

    if (hzpm_try_set_hupmm(PMM_PMLEN7)) {
        pm_set_hupmm(PMM_RESERVED);
        unsigned rb7 = pm_get_hupmm();
        TEST_ASSERT_NEQ("reserved encoding never reads back (from PMLEN7)",
                        rb7, PMM_RESERVED);
    }

    pm_set_hupmm(PMM_DISABLED);
    HYP_TEST_END();
}

/* ----- HZPM-CAP-07: henvcfg.PMM does not disturb other fields ----- */
TEST_REGISTER(test_hzpm_cap_07);
bool test_hzpm_cap_07(void) {
    TEST_BEGIN("HZPM-CAP-07: henvcfg.PMM field isolation");
    SSNPM_HYP_REQUIRED_OR_SKIP();

    uintptr_t before = henvcfg_read();
    pm_set_vsmode(PMM_PMLEN7);
    pm_set_vsmode(PMM_PMLEN16);
    pm_set_vsmode(PMM_DISABLED);
    uintptr_t after = henvcfg_read();

    /* All fields except PMM must be unchanged by PMM toggling */
    TEST_ASSERT_EQ("non-PMM henvcfg fields unchanged",
                   after & ~HENVCFG_PMM_MASK,
                   before & ~HENVCFG_PMM_MASK);
    HYP_TEST_END();
}

/* ----- HZPM-CAP-08: hstatus.HUPMM does not disturb other fields ----- */
TEST_REGISTER(test_hzpm_cap_08);
bool test_hzpm_cap_08(void) {
    TEST_BEGIN("HZPM-CAP-08: hstatus.HUPMM field isolation");
    SSNPM_HYP_REQUIRED_OR_SKIP();

    uintptr_t before = hstatus_read();
    pm_set_hupmm(PMM_PMLEN7);
    pm_set_hupmm(PMM_PMLEN16);
    pm_set_hupmm(PMM_DISABLED);
    uintptr_t after = hstatus_read();

    TEST_ASSERT_EQ("non-HUPMM hstatus fields unchanged",
                   after & ~HSTATUS_HUPMM_MASK,
                   before & ~HSTATUS_HUPMM_MASK);
    HYP_TEST_END();
}

/* ----- HZPM-CAP-09: both fields read-only zero without Ssnpm ----- */
TEST_REGISTER(test_hzpm_cap_09);
bool test_hzpm_cap_09(void) {
    TEST_BEGIN("HZPM-CAP-09: PMM/HUPMM read-only zero without Ssnpm");
    H_REQUIRED_OR_SKIP();

    if (detect_ssnpm())
        TEST_SKIP("Ssnpm implemented, read-only-zero case N/A");

    /* norm:henvcfg_pmm_op + sec:hstatus: without Ssnpm both fields
     * are read-only zero; writes of any encoding must be ignored. */
    pm_set_vsmode(PMM_PMLEN7);
    TEST_ASSERT_EQ("henvcfg.PMM read-only zero",
                   pm_get_vsmode(), PMM_DISABLED);
    pm_set_vsmode(PMM_PMLEN16);
    TEST_ASSERT_EQ("henvcfg.PMM read-only zero (PMLEN16)",
                   pm_get_vsmode(), PMM_DISABLED);

    pm_set_hupmm(PMM_PMLEN7);
    TEST_ASSERT_EQ("hstatus.HUPMM read-only zero",
                   pm_get_hupmm(), PMM_DISABLED);
    pm_set_hupmm(PMM_PMLEN16);
    TEST_ASSERT_EQ("hstatus.HUPMM read-only zero (PMLEN16)",
                   pm_get_hupmm(), PMM_DISABLED);
    HYP_TEST_END();
}

/* ----- HZPM-CAP-10: VS-mode cannot modify henvcfg.PMM ----- */
TEST_REGISTER(test_hzpm_cap_10);
bool test_hzpm_cap_10(void) {
    TEST_BEGIN("HZPM-CAP-10: VS-mode henvcfg access traps (cause=22)");
    SSNPM_HYP_REQUIRED_OR_SKIP();

    /* norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0: when V=1, any
     * access to a hypervisor CSR raises a virtual-instruction
     * exception. The guest must not be able to change its own
     * VS-level PM setting (norm:pm_config_next_higher).
     *
     * If Smstateen is implemented, open the mstateen0.ENVCFG gate
     * first: with the gate closed the access is not HS-qualified
     * and raises illegal-instruction instead (norm:mstateen0_envcfg_op
     * + norm:stateen_illegal_state_access), which is a different
     * (also spec-compliant) interception path not under test here. */
    if (stateen_is_available())
        mstateen_set_bits(0, STATEEN0_ENVCFG);

    trap_expect_begin();
    run_in_vs_mode(hzpm_write_henvcfg, 0);
    TEST_ASSERT("virtual-instruction trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();
    HYP_TEST_END();
}
