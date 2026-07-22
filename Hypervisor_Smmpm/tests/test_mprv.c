/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mprv.c - Group 7: H x Smmpm - MPRV+MPV effective-privilege PM
 *
 * HZPM-MPRV-01 ~ HZPM-MPRV-05
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 7.
 *
 * Spec references:
 *   - norm:pm_mprv_spvp (zpm.adoc): MPRV and SPVP cause the PM
 *     settings of the *effective* privilege mode to be applied.
 *   - norm:mstatus_mprv_hypervisor (hypervisor.adoc): with MPRV=1,
 *     explicit accesses are translated/protected as though V=MPV and
 *     the nominal privilege mode were MPP.
 *   - norm:mseccfg_pmm_presence_op (machine.adoc): mseccfg.PMM
 *     controls M-mode PM only.
 */

static volatile uint64_t mprv_data __attribute__((aligned(8)));

/* ----- HZPM-MPRV-01: MPRV=1 MPV=1 MPP=S uses henvcfg.PMM ----- */
TEST_REGISTER(test_hzpm_mprv_01);
bool test_hzpm_mprv_01(void) {
    TEST_BEGIN("HZPM-MPRV-01: MPRV MPV=1 MPP=S uses henvcfg.PMM");
    H_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!detect_ssnpm_hyp())
        TEST_SKIP("Ssnpm hyp controls not implemented");
    pm_set_vsmode(PMM_PMLEN7);
    if (pm_get_vsmode() != PMM_PMLEN7)
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    /* Isolate: M-mode's own PM must not be the one applied */
    if (detect_smmpm())
        pm_set_mmode(PMM_DISABLED);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    two_stage_enable(&ctx, /*vmid=*/0);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    /* norm:pm_mprv_spvp + norm:mstatus_mprv_hypervisor: effective
     * mode is VS (V=1, S) -> henvcfg.PMM applies -> tagged load
     * succeeds. */
    bool trapped;
    uint64_t val = hzpm_mprv_load(tagged, PRIV_S, /*mpv=*/1, &trapped);

    TEST_ASSERT("MPRV tagged load should not fault", !trapped);
    TEST_ASSERT_EQ("MPRV load via VS PM correct", val, HZPM_MAGIC1);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-MPRV-02: MPRV=1 MPV=1 MPP=U uses senvcfg.PMM ----- */
TEST_REGISTER(test_hzpm_mprv_02);
bool test_hzpm_mprv_02(void) {
    TEST_BEGIN("HZPM-MPRV-02: MPRV MPV=1 MPP=U uses senvcfg.PMM");
    H_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!detect_ssnpm())
        TEST_SKIP("Ssnpm (senvcfg.PMM) not implemented");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7)
        TEST_SKIP("PMLEN=7 not supported for U/VU-mode");

    if (detect_smmpm())
        pm_set_mmode(PMM_DISABLED);

    /* U=1 mappings required for effective-VU access */
    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    two_stage_enable(&ctx, /*vmid=*/0);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC2;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    /* Effective mode is VU (V=1, U) -> senvcfg.PMM applies. */
    bool trapped;
    uint64_t val = hzpm_mprv_load(tagged, PRIV_U, /*mpv=*/1, &trapped);

    TEST_ASSERT("MPRV tagged load should not fault", !trapped);
    TEST_ASSERT_EQ("MPRV load via VU PM correct", val, HZPM_MAGIC2);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-MPRV-03: mseccfg.PMM NOT applied to effective VS ----- */
TEST_REGISTER(test_hzpm_mprv_03);
bool test_hzpm_mprv_03(void) {
    TEST_BEGIN("HZPM-MPRV-03: mseccfg.PMM not applied to effective VS");
    SMMPM_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!detect_ssnpm_hyp())
        TEST_SKIP("Ssnpm hyp controls not implemented");
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7)
        TEST_SKIP("PMLEN=7 not supported for M-mode");

    /* VS PM off: the effective VS-mode access must NOT be masked,
     * even though M-mode's own PM (mseccfg.PMM) is enabled. */
    pm_set_vsmode(PMM_DISABLED);
    TEST_ASSERT_EQ("henvcfg.PMM disabled", pm_get_vsmode(), PMM_DISABLED);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    two_stage_enable(&ctx, /*vmid=*/0);

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    /* norm:pm_mprv_spvp: the PM setting of the *effective* mode
     * (VS -> henvcfg.PMM=0) applies, so the tagged VA is used
     * verbatim -> invalid Sv39 VA -> load page-fault (13). */
    bool trapped;
    (void)hzpm_mprv_load(tagged, PRIV_S, /*mpv=*/1, &trapped);

    TEST_ASSERT("tagged load must fault (M-mode PM not applied)",
                trapped);
    if (trapped) {
        TEST_ASSERT_EQ("cause=13 (load page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    }

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-MPRV-04: MPRV=1 MPV=0 MPP=S uses menvcfg.PMM ----- */
TEST_REGISTER(test_hzpm_mprv_04);
bool test_hzpm_mprv_04(void) {
    TEST_BEGIN("HZPM-MPRV-04: MPRV MPV=0 MPP=S uses menvcfg.PMM");
    H_REQUIRED_OR_SKIP();
    if (!detect_smnpm())
        TEST_SKIP("Smnpm (menvcfg.PMM) not implemented");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7)
        TEST_SKIP("PMLEN=7 not supported for S/HS-mode");

    if (detect_smmpm())
        pm_set_mmode(PMM_DISABLED);

    /* satp=Bare: effective HS access uses a physical address ->
     * zero-extend transformation (norm:pm_ignore_pa). */
    mprv_data = HZPM_MAGIC3;
    uintptr_t tagged = pm_tag_address((uintptr_t)&mprv_data,
                                      pm_max_tag(7), 7);

    /* Effective mode is HS (V=0, S) -> menvcfg.PMM applies. */
    bool trapped;
    uint64_t val = hzpm_mprv_load(tagged, PRIV_S, /*mpv=*/0, &trapped);

    TEST_ASSERT("MPRV tagged load should not fault", !trapped);
    TEST_ASSERT_EQ("MPRV load via HS PM correct", val, HZPM_MAGIC3);
    HYP_TEST_END();
}

/* ----- HZPM-MPRV-05: MPRV=0 baseline uses mseccfg.PMM ----- */
TEST_REGISTER(test_hzpm_mprv_05);
bool test_hzpm_mprv_05(void) {
    TEST_BEGIN("HZPM-MPRV-05: MPRV=0 baseline uses mseccfg.PMM");
    SMMPM_REQUIRED_OR_SKIP();
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7)
        TEST_SKIP("PMLEN=7 not supported for M-mode");

    /* Ensure other modes' PM cannot be credited for the access */
    if (detect_smnpm())
        pm_set_smode(PMM_DISABLED);

    mprv_data = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address((uintptr_t)&mprv_data,
                                      pm_max_tag(7), 7);

    /* MPRV=0: M-mode's own PM (mseccfg.PMM) applies -> PA
     * zero-extend -> tagged load succeeds. */
    uintptr_t ms = CSRR(mstatus);
    CSRW(mstatus, ms & ~MSTATUS_MPRV_BIT);

    trap_expect_begin();
    uint64_t val = *(volatile uint64_t *)tagged;
    bool trapped = trap_was_triggered();
    trap_expect_end();

    CSRW(mstatus, ms);

    TEST_ASSERT("MPRV=0 tagged load should not fault", !trapped);
    TEST_ASSERT_EQ("MPRV=0 uses M-mode PM", val, HZPM_MAGIC1);
    HYP_TEST_END();
}
