/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2.1: NTL execution in HS/VS/VU modes (NTL-HYP-01~03)
 *
 * Spec anchors:
 *   norm:NTL_target_definition
 *     - NTL instructions do not change architectural state, nor alter
 *       the architecturally visible effects of the target instruction;
 *       the virtualized environment must behave identically.
 *
 * Assertion strategy: HINT no-side-effect comparison - the prefixed
 * load must return the same value as the unprefixed load, and no
 * exception (in particular no virtual-instruction) may fire.
 * =================================================================== */

/* ---- NTL-HYP-01: HS-mode ntl.all + ld normal ---- */

TEST_REGISTER(test_ntl_hyp_01);
bool test_ntl_hyp_01(void)
{
    TEST_BEGIN("NTL-HYP-01: HS-mode ntl.all + ld normal");
    H_REQUIRED_OR_SKIP();

    ntl_mem = NTL_MAGIC;

    /* HS-mode == PRIV_S when V=0 (ENABLE_HYP build). The armed
     * window catches any trap raised by the prefixed/unprefixed
     * loads (none is expected). */
    goto_priv(PRIV_S);
    trap_expect_begin();
    hs_ntl_all_ld(&ntl_mem);
    trap_expect_end();
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("HS-mode ntl.all + ld");

    goto_priv(PRIV_S);
    trap_expect_begin();
    hs_plain_ld(&ntl_mem);
    trap_expect_end();
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("HS-mode plain ld");

    /* Value comparison from M-mode (same physical memory): the
     * prefixed load must observe the identical value. */
    uint64_t v_pref = hs_ntl_all_ld(&ntl_mem);
    uint64_t v_plain = hs_plain_ld(&ntl_mem);

    TEST_ASSERT_EQ("prefixed load value == NTL_MAGIC", v_pref, NTL_MAGIC);
    TEST_ASSERT_EQ("prefixed value == unprefixed value", v_pref, v_plain);

    TEST_END();
}

/* ---- NTL-HYP-02: VS-mode ntl.all + ld normal ---- */

TEST_REGISTER(test_ntl_hyp_02);
bool test_ntl_hyp_02(void)
{
    TEST_BEGIN("NTL-HYP-02: VS-mode ntl.all + ld normal");
    H_REQUIRED_OR_SKIP();

    ntl_mem = NTL_MAGIC;

    /* vsatp/hgatp stay Bare (hyp_reset_state): VS callbacks run on
     * physical addresses, no translation involved. */
    trap_expect_begin();
    uintptr_t v_pref = run_in_vs_mode(vs_ntl_all_ld, (uintptr_t)&ntl_mem);
    bool pref_trapped = trap_was_triggered();
    trap_expect_end();

    trap_expect_begin();
    uintptr_t v_plain = run_in_vs_mode(vs_plain_ld, (uintptr_t)&ntl_mem);
    bool plain_trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VS-mode ntl.all + ld: no trap (no virtual-instruction)",
                !pref_trapped);
    TEST_ASSERT("VS-mode plain ld: no trap", !plain_trapped);
    if (!pref_trapped && !plain_trapped) {
        TEST_ASSERT_EQ("VS-mode prefixed load value", v_pref, NTL_MAGIC);
        TEST_ASSERT_EQ("VS-mode prefixed == unprefixed", v_pref, v_plain);
    }

    TEST_END();
}

/* ---- NTL-HYP-03: VU-mode ntl.all + ld normal ---- */

TEST_REGISTER(test_ntl_hyp_03);
bool test_ntl_hyp_03(void)
{
    TEST_BEGIN("NTL-HYP-03: VU-mode ntl.all + ld normal");
    H_REQUIRED_OR_SKIP();

    ntl_mem = NTL_MAGIC;

    trap_expect_begin();
    uintptr_t v_pref = run_in_vu_mode(vu_ntl_all_ld, (uintptr_t)&ntl_mem);
    bool pref_trapped = trap_was_triggered();
    trap_expect_end();

    trap_expect_begin();
    uintptr_t v_plain = run_in_vu_mode(vu_plain_ld, (uintptr_t)&ntl_mem);
    bool plain_trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VU-mode ntl.all + ld: no trap", !pref_trapped);
    TEST_ASSERT("VU-mode plain ld: no trap", !plain_trapped);
    if (!pref_trapped && !plain_trapped) {
        TEST_ASSERT_EQ("VU-mode prefixed load value", v_pref, NTL_MAGIC);
        TEST_ASSERT_EQ("VU-mode prefixed == unprefixed", v_pref, v_plain);
    }

    TEST_END();
}
