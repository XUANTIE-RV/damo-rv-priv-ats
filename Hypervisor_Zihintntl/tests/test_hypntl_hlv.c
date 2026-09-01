/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2.2: NTL applied to HLV/HSV/HLVX in HS-mode (NTL-HYP-04)
 *
 * Spec anchors:
 *   norm:NTL_range
 *     - NTL instructions affect all memory-access instructions except
 *       Zicbom cache-management instructions; the H-extension
 *       virtual-machine load/store instructions (HLV/HSV/HLVX) are
 *       within the affected range.
 *   norm:NTL_target_definition
 *     - architecturally visible effects must be identical to the
 *       unprefixed sequence.
 *
 * With hgatp=Bare (hyp_reset_state) guest-physical addresses are
 * physical addresses; the effective privilege of HLV/HSV is taken
 * from hstatus.SPVP. Comparison is against the same sequence without
 * the prefix.
 * =================================================================== */

TEST_REGISTER(test_ntl_hyp_04);
bool test_ntl_hyp_04(void)
{
    TEST_BEGIN("NTL-HYP-04: ntl.all + HLV/HSV/HLVX parity vs unprefixed");
    H_REQUIRED_OR_SKIP();

    /* --- HLV.D: prefixed vs unprefixed load value --- */
    ntl_gva_mem = NTL_MAGIC;

    trap_expect_begin();
    uint64_t hlv_pref = hs_ntl_all_hlv_d((uintptr_t)&ntl_gva_mem);
    bool pref_trapped = trap_was_triggered();
    trap_expect_end();

    trap_expect_begin();
    uint64_t hlv_plain = hlv_d((uintptr_t)&ntl_gva_mem);
    bool plain_trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("ntl.all + hlv.d: no trap", !pref_trapped);
    TEST_ASSERT("plain hlv.d: no trap", !plain_trapped);
    if (!pref_trapped && !plain_trapped) {
        TEST_ASSERT_EQ("prefixed hlv.d value", hlv_pref, NTL_MAGIC);
        TEST_ASSERT_EQ("prefixed == unprefixed hlv.d", hlv_pref, hlv_plain);
    }

    /* --- HSV.D: prefixed vs unprefixed store effect --- */
    ntl_gva_mem = 0;
    ntl_gva_aux = 0;

    trap_expect_begin();
    hs_ntl_all_hsv_d((uintptr_t)&ntl_gva_mem, NTL_MAGIC);
    bool hsv_pref_trapped = trap_was_triggered();
    trap_expect_end();

    trap_expect_begin();
    hsv_d((uintptr_t)&ntl_gva_aux, NTL_MAGIC);
    bool hsv_plain_trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("ntl.all + hsv.d: no trap", !hsv_pref_trapped);
    TEST_ASSERT("plain hsv.d: no trap", !hsv_plain_trapped);
    if (!hsv_pref_trapped && !hsv_plain_trapped) {
        TEST_ASSERT_EQ("prefixed hsv.d stored value", ntl_gva_mem, NTL_MAGIC);
        TEST_ASSERT_EQ("prefixed == unprefixed hsv.d effect",
                       ntl_gva_mem, ntl_gva_aux);
    }

    /* --- HLVX.WU: prefixed vs unprefixed execute-permission load --- */
    ntl_gva_mem = NTL_MAGIC;

    trap_expect_begin();
    uint32_t hlvx_pref = hs_ntl_all_hlvx_wu((uintptr_t)&ntl_gva_mem);
    bool hlvx_pref_trapped = trap_was_triggered();
    trap_expect_end();

    trap_expect_begin();
    uint32_t hlvx_plain = hlvx_wu((uintptr_t)&ntl_gva_mem);
    bool hlvx_plain_trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("ntl.all + hlvx.wu: no trap", !hlvx_pref_trapped);
    TEST_ASSERT("plain hlvx.wu: no trap", !hlvx_plain_trapped);
    if (!hlvx_pref_trapped && !hlvx_plain_trapped) {
        TEST_ASSERT_EQ("prefixed == unprefixed hlvx.wu",
                       hlvx_pref, hlvx_plain);
    }

    TEST_END();
}
