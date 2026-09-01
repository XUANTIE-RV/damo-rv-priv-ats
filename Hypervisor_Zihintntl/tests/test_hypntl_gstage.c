/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2.4: NTL + load G-stage guest-page-fault in VS (NTL-HYP-06)
 *
 * Spec anchors:
 *   norm:NTL_target_definition
 *     - the trap reporting of the target instruction must be
 *       identical with and without the NTL prefix.
 *   hypervisor.adoc guest-page-fault reporting (GVA=1, stval/htval):
 *     the load faults on the G-stage translation while the VS-stage
 *     mapping is valid.
 *
 * Layout: two-stage identity with a G-stage victim page invalidated
 * (ts2_setup_with_g_victim, flags=0); VS-stage maps the whole test
 * region S-level RWX. A VS-mode "ntl.p1 + ld" on the victim page
 * must raise load guest-page-fault (cause=21) with an identical trap
 * report (cause/epc/stval/GVA/htval) to the unprefixed load.
 * =================================================================== */

typedef struct {
    bool      triggered;
    uintptr_t cause;
    uintptr_t epc;
    uintptr_t tval;
    uintptr_t gva;
    uintptr_t htval;
} ntl_gpf_rep_t;

static void ntl_gpf_capture(ntl_gpf_rep_t *r)
{
    r->triggered = trap_was_triggered();
    r->cause     = trap_get_cause();
    r->epc       = trap_get_epc();
    r->tval      = trap_get_tval();
    r->gva       = trap_get_gva();
    r->htval     = trap_get_htval();
}

TEST_REGISTER(test_ntl_hyp_06);
bool test_ntl_hyp_06(void)
{
    TEST_BEGIN("NTL-HYP-06: ntl.p1 + ld G-stage guest-page-fault report");
    H_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    /* VS-stage valid (identity, S-level RWX) + G-stage invalid on
     * the victim 4KB page. */
    ts2_setup_with_g_victim(&ctx, SATP_MODE_SV39, SUITE_HGATP_MODE,
                            victim, 0 /* invalid */);

    /* Run 1: ntl.p1 + ld (armed). */
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_ntl_p1_ld, victim);
    ntl_gpf_rep_t rep_pref;
    ntl_gpf_capture(&rep_pref);
    trap_expect_end();

    /* Run 2: plain ld (armed). */
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_ld_only, victim);
    ntl_gpf_rep_t rep_plain;
    ntl_gpf_capture(&rep_plain);
    trap_expect_end();

    ts2_finish(&ctx);

    /* Both runs must raise load guest-page-fault (cause=21) into
     * HS-mode: G-stage faults are never delegated to VS. */
    TEST_ASSERT("prefixed: trap triggered", rep_pref.triggered);
    TEST_ASSERT("unprefixed: trap triggered", rep_plain.triggered);

    if (rep_pref.triggered && rep_plain.triggered) {
        TEST_ASSERT_EQ("prefixed: cause=21 (load guest-page-fault)",
                       rep_pref.cause,
                       (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
        TEST_ASSERT_EQ("identical cause with/without prefix",
                       rep_pref.cause, rep_plain.cause);
        /* stval: faulting guest virtual address (identity: == victim),
         * identical in both runs. */
        TEST_ASSERT_EQ("identical stval with/without prefix",
                       rep_pref.tval, rep_plain.tval);
        /* GVA=1: trap report carries a guest virtual address. */
        TEST_ASSERT_EQ("prefixed: hstatus.GVA=1", rep_pref.gva, (uintptr_t)1);
        TEST_ASSERT_EQ("unprefixed: hstatus.GVA=1",
                       rep_plain.gva, (uintptr_t)1);
        /* htval: faulting GPA>>2 (implementation may write zero);
         * whatever the implementation writes must be identical for
         * the prefixed and unprefixed sequences. */
        TEST_ASSERT_EQ("identical htval with/without prefix",
                       rep_pref.htval, rep_plain.htval);
    }

    TEST_END();
}
