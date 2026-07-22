/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_2stg.c - Group 5: H x Ssnpm - two-stage translation x PM
 *
 * HZPM-2STG-01 ~ HZPM-2STG-05
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 5.
 *
 * Spec references (hypervisor.adoc, [[pm-two-stage]]):
 *   - GPAs are 2 bits wider than the corresponding VA translation
 *     modes. With vsatp.MODE=Bare in VS/VU-mode, those 2 bits may be
 *     subject to pointer masking depending on hgatp.MODE and
 *     senvcfg/henvcfg.PMM. With vsatp.MODE!=Bare this does not apply.
 *   - When henvcfg.PMM changes from/to a value where (XLEN-PMLEN) is
 *     less than the GPA width of hgatp.MODE, the hypervisor should
 *     execute HFENCE.GVMA with rs1=x0.
 *   - norm:pm_not_apply_implicit: page-table walks are not masked.
 */

/* ----- HZPM-2STG-01: GPA extra 2 bits subject to PM (Sv48x4) ----- */
TEST_REGISTER(test_hzpm_2stg_01);
bool test_hzpm_2stg_01(void) {
    TEST_BEGIN("HZPM-2STG-01: GPA extra bits masked (Sv48x4+PMLEN16)");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV48X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN16))
        TEST_SKIP("PMLEN=16 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV48X4);

    /* Sv48x4 GPA width is 50 bits; the 2 extra bits over Sv48 are
     * [49:48], which fall inside the PMLEN=16 mask [63:48].
     * tag=0x3 sets exactly bits[49:48] of the tagged GPA. */
    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC1;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, 0x3, 16);
    TEST_ASSERT("tag sets GPA extra bits [49:48]",
                (tagged & (0x3ULL << 48)) == (0x3ULL << 48));

    /* PM on: zero-extend discards the extra bits -> correct SPA */
    uintptr_t r = two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged GPA (extra bits) load correct",
                   r, HZPM_MAGIC1);

    /* Control: PM off, the same tagged GPA is a *different* GPA
     * within the Sv48x4 width (0x30000_80000000-ish) which is
     * unmapped -> guest-page-fault. This proves the tag bits reach
     * into the GPA space and are not simply ignored by width. */
    pm_set_vsmode(PMM_DISABLED);
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT("control: untransformed tagged GPA trapped",
                trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=21 (load guest-page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    }
    trap_expect_end();

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-2STG-02: vsatp!=Bare -> extra-bits issue N/A ----- */
TEST_REGISTER(test_hzpm_2stg_02);
bool test_hzpm_2stg_02(void) {
    TEST_BEGIN("HZPM-2STG-02: VA sign-extend with two-stage active");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    /* Both stages active: VS-stage Sv39 + G-stage Sv39x4.
     * pm-two-stage: with vsatp.MODE!=Bare the GPA-extra-bits issue
     * does not apply; the VA ignore transformation (sign-extend) is
     * used and the result goes through both translation stages. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC2;
    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    uintptr_t r = two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged VA load via two-stage correct",
                   r, HZPM_MAGIC2);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-2STG-03: PMM change + HFENCE.GVMA (PMLEN16, Sv48x4) ----- */
TEST_REGISTER(test_hzpm_2stg_03);
bool test_hzpm_2stg_03(void) {
    TEST_BEGIN("HZPM-2STG-03: PMM change sync via HFENCE.GVMA (48x4)");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV48X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN16))
        TEST_SKIP("PMLEN=16 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV48X4);

    /* Warm the TLB with PM disabled */
    pm_set_vsmode(PMM_DISABLED);
    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC3;
    uintptr_t r0 = two_stage_run_in_vs(&ctx, hzpm_load64, HZPM_DATA1);
    TEST_ASSERT_EQ("untagged baseline load", r0, HZPM_MAGIC3);

    /* pm-two-stage: PMLEN=16 with hgatp.MODE=sv48x4 is a listed case
     * where (XLEN-PMLEN)=48 < GPA width=50. After changing
     * henvcfg.PMM the hypervisor must execute HFENCE.GVMA(rs1=x0). */
    pm_set_vsmode(PMM_PMLEN16);
    hfence_gvma_all();

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(16), 16);
    uintptr_t r1 = two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged load after PMM change + HFENCE.GVMA",
                   r1, HZPM_MAGIC3);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-2STG-04: PMM change + HFENCE.GVMA (PMLEN7, Sv57x4) ----- */
TEST_REGISTER(test_hzpm_2stg_04);
bool test_hzpm_2stg_04(void) {
    TEST_BEGIN("HZPM-2STG-04: PMM change sync via HFENCE.GVMA (57x4)");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV57X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV57X4);

    /* Warm the TLB with PM disabled */
    pm_set_vsmode(PMM_DISABLED);
    *(volatile uint64_t *)HZPM_DATA1 = HZPM_MAGIC1;
    uintptr_t r0 = two_stage_run_in_vs(&ctx, hzpm_load64, HZPM_DATA1);
    TEST_ASSERT_EQ("untagged baseline load", r0, HZPM_MAGIC1);

    /* pm-two-stage: PMLEN=7 with hgatp.MODE=sv57x4 is a listed case
     * where (XLEN-PMLEN)=57 < GPA width=59. */
    pm_set_vsmode(PMM_PMLEN7);
    hfence_gvma_all();

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);
    uintptr_t r1 = two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT_EQ("tagged load after PMM change + HFENCE.GVMA",
                   r1, HZPM_MAGIC1);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-2STG-05: G-stage page-table walk not masked ----- */
TEST_REGISTER(test_hzpm_2stg_05);
bool test_hzpm_2stg_05(void) {
    TEST_BEGIN("HZPM-2STG-05: G-stage walk unaffected by PM");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* norm:pm_not_apply_implicit / norm:pm_cpu_only: PM does not
     * apply to implicit accesses such as page-table walks. Flush all
     * G-stage TLB entries with PM enabled, then access several pages
     * (untagged) so the hart must perform fresh G-stage walks using
     * the raw (untransformed) page-table addresses. */
    hfence_gvma_all();

    uintptr_t addrs[] = { HZPM_DATA1, HZPM_DATA2, HZPM_DATA3, HZPM_DATA4 };
    for (unsigned i = 0; i < sizeof(addrs) / sizeof(addrs[0]); i++) {
        uintptr_t rc = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           addrs[i]);
        TEST_ASSERT("fresh G-stage walk + R/W OK", rc == 0);
    }

    ts2_finish(&ctx);
    HYP_TEST_END();
}
