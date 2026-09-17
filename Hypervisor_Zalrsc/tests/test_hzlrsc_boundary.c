/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.9 - Architectural boundary: no LR/SC guest-atomic equivalent
 *             (HZLRSC-39 ~ HZLRSC-40)
 *
 * Spec basis:
 *   norm:hlsv_op  - the virtual-machine load/store instructions map only
 *     the RV32I/RV64I base loads/stores (LB..LD -> HLV, SB..SD -> HSV).
 *     LR/SC live in the AMO opcode and have NO virtual-machine equivalent
 *     (there is no HLR/HSC), so HS-mode cannot perform an atomic RMW on
 *     guest memory with VS/VU effective privilege.
 *   norm:hlsv_priv - HLV/HLVX/HSV effective privilege is selected by
 *     hstatus.SPVP; no atomic instruction has such a guest-qualified form.
 * =================================================================== */

/* ------------------------------------------------------------------
 * HZLRSC-39 (record-type): HLV/HSV exist for guest load/store, but there
 *            is no guest-atomic (HLR/HSC) equivalent for LR/SC.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_39_no_guest_atomic_equivalent);
bool test_hzlrsc_39_no_guest_atomic_equivalent(void)
{
    TEST_BEGIN("HZLRSC-39: (record) HLV/HSV exist, no HLR/HSC equivalent");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint64_t *)va = 0xDEADBEEF12345678ULL;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    two_stage_enable(&ctx, 0);        /* activate vsatp/hgatp, stay in HS */
    hstatus_set_spvp(PRIV_S);         /* effective privilege = VS */

    /* Positive: the base guest load/store equivalents ARE available. */
    trap_expect_begin();
    uint64_t got = hlv_d(va);
    bool hlv_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("HLV.D (guest load equivalent) available", !hlv_fired);
    TEST_ASSERT_EQ("HLV.D read the guest value",
                   got, (uintptr_t)0xDEADBEEF12345678ULL);

    trap_expect_begin();
    hsv_d(va, 0xCAFEBABE0BADF00DULL);
    bool hsv_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("HSV.D (guest store equivalent) available", !hsv_fired);
    TEST_ASSERT_EQ("HSV.D wrote the guest value",
                   *(volatile uint64_t *)va,
                   (uintptr_t)0xCAFEBABE0BADF00DULL);

    ts2_finish(&ctx);

    /* Record the architectural boundary (norm:hlsv_op): only LB..LD /
     * SB..SD have HLV/HSV equivalents; LR/SC (AMO opcode) have none, so a
     * hypervisor cannot do a guest-privileged atomic RMW directly - it
     * must use non-atomic HLV+HSV or map the GPA into its own space. */
    printf("  [RECORD] HLV/HSV cover base loads/stores only; no HLR/HSC "
           "guest-atomic equivalent exists (norm:hlsv_op)\n");
    TEST_ASSERT("record-type boundary case executed", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLRSC-40 (record-type): HLV+HSV cannot replace LR/SC atomicity under
 *            concurrency. Requires a secondary hart; SKIP (framework runs
 *            only hart 0).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlrsc_40_hlv_hsv_not_atomic);
bool test_hzlrsc_40_hlv_hsv_not_atomic(void)
{
    TEST_BEGIN("HZLRSC-40: (record) HLV+HSV cannot replace LR/SC atomicity");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    TEST_SKIP(HZLRSC_SMP_SKIP_REASON);
    HYP_TEST_END();
}
