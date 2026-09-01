/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2.3: NTL + CMO virtual-instruction report in VS (NTL-HYP-05)
 *
 * Spec anchors:
 *   norm:NTL_target_definition
 *     - the NTL prefix must not change the architecturally visible
 *       effects of the target instruction, including trap reporting.
 *   cmo.adoc (norm:henvcfg_cbie / norm:henvcfg_cbcfe via the
 *   Hypervisor_CMO plan): with henvcfg.CBIE=0 / CBCFE=0, VS-mode
 *   cbo.inval / cbo.clean raise virtual-instruction (cause=22).
 *
 * Assertions: with and without the ntl.all prefix the trap must be
 * identical (cause=22), and the trap PC must point at the CBO
 * instruction itself, never at the NTL prefix.
 * =================================================================== */

/* Run one VS-mode CBO sequence (armed) and snapshot the report.
 * The sequence runs inside VS-mode (V=1) via run_in_vs_mode; the
 * henvcfg gating only applies when V=1, so HS-mode execution would
 * bypass the condition under test. */
static void ntl_cmo_vs_run(bool with_prefix, bool is_inval,
                           uintptr_t *cbo_pc_out, ntl_trap_rep_t *rep)
{
    uintptr_t (*fn)(uintptr_t);
    if (with_prefix)
        fn = is_inval ? vs_ntl_cbo_inval : vs_ntl_cbo_clean;
    else
        fn = is_inval ? vs_plain_cbo_inval : vs_plain_cbo_clean;

    ntl_g_cbo_pc = 0;
    trap_expect_begin();
    (void)run_in_vs_mode(fn, 0);
    ntl_capture_rep(rep);
    trap_expect_end();

    *cbo_pc_out = ntl_g_cbo_pc;
}

/* ---- NTL-HYP-05a: cbo.inval report unchanged by ntl prefix ---- */

TEST_REGISTER(test_ntl_hyp_05a);
bool test_ntl_hyp_05a(void)
{
    TEST_BEGIN("NTL-HYP-05a: ntl + cbo.inval virtual-instruction report");
    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    /* henvcfg.CBIE=0 and CBCFE=0 (hyp_reset_state wrote henvcfg=0);
     * menvcfg.CBIE enables the HS-level path per the cmo.adoc
     * pseudocode. */
    uintptr_t orig_menvcfg = menvcfg_read();
    menvcfg_set_cbie(CBIE_INVAL);
    ntl_henvcfg_set_cbie(0);
    ntl_henvcfg_set_cbcfe(0);

    ntl_trap_rep_t rep_pref, rep_plain;
    uintptr_t pc_pref = 0, pc_plain = 0;

    ntl_cmo_vs_run(true, true, &pc_pref, &rep_pref);
    ntl_cmo_vs_run(false, true, &pc_plain, &rep_plain);

    menvcfg_write(orig_menvcfg);

    TEST_ASSERT("prefixed: trap triggered", rep_pref.triggered);
    TEST_ASSERT("unprefixed: trap triggered", rep_plain.triggered);
    if (rep_pref.triggered && rep_plain.triggered) {
        TEST_ASSERT_EQ("prefixed: cause=22 (virtual-instruction)",
                       rep_pref.cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        TEST_ASSERT_EQ("identical cause with/without prefix",
                       rep_pref.cause, rep_plain.cause);
        TEST_ASSERT_EQ("identical tval with/without prefix",
                       rep_pref.tval, rep_plain.tval);
    }
    /* Trap PC must point at the CBO instruction, not the NTL prefix:
     * the recorded epc equals the captured CBO label in both runs. */
    if (rep_pref.triggered) {
        TEST_ASSERT_EQ("prefixed: trap epc == cbo.inval PC (not NTL)",
                       rep_pref.epc, pc_pref);
    }
    if (rep_plain.triggered) {
        TEST_ASSERT_EQ("unprefixed: trap epc == cbo.inval PC",
                       rep_plain.epc, pc_plain);
    }

    HYP_TEST_END();
}

/* ---- NTL-HYP-05b: cbo.clean report unchanged by ntl prefix ---- */

TEST_REGISTER(test_ntl_hyp_05b);
bool test_ntl_hyp_05b(void)
{
    TEST_BEGIN("NTL-HYP-05b: ntl + cbo.clean virtual-instruction report");
    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    /* norm:cbo-clean_cbo-flush pseudocode: the illegal-instruction
     * check on menvcfg.CBCFE precedes the virtual-instruction check
     * on henvcfg.CBCFE, so menvcfg.CBCFE must be enabled first to
     * reach the V=1 gating under test. */
    uintptr_t orig_menvcfg = menvcfg_read();
    menvcfg_set_cbcfe(1);
    ntl_henvcfg_set_cbcfe(0);

    ntl_trap_rep_t rep_pref, rep_plain;
    uintptr_t pc_pref = 0, pc_plain = 0;

    ntl_cmo_vs_run(true, false, &pc_pref, &rep_pref);
    ntl_cmo_vs_run(false, false, &pc_plain, &rep_plain);

    menvcfg_write(orig_menvcfg);

    TEST_ASSERT("prefixed: trap triggered", rep_pref.triggered);
    TEST_ASSERT("unprefixed: trap triggered", rep_plain.triggered);
    if (rep_pref.triggered && rep_plain.triggered) {
        TEST_ASSERT_EQ("prefixed: cause=22 (virtual-instruction)",
                       rep_pref.cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        TEST_ASSERT_EQ("identical cause with/without prefix",
                       rep_pref.cause, rep_plain.cause);
        TEST_ASSERT_EQ("identical tval with/without prefix",
                       rep_pref.tval, rep_plain.tval);
    }
    if (rep_pref.triggered) {
        TEST_ASSERT_EQ("prefixed: trap epc == cbo.clean PC (not NTL)",
                       rep_pref.epc, pc_pref);
    }
    if (rep_plain.triggered) {
        TEST_ASSERT_EQ("unprefixed: trap epc == cbo.clean PC",
                       rep_plain.epc, pc_plain);
    }

    HYP_TEST_END();
}
