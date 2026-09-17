/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4.9 - architectural boundary: no byte/half guest-atomic equivalent
 *             (HZABHA-38 ~ HZABHA-39)
 *
 * Spec basis:
 *   norm:hlsv_op - the virtual-machine load/store instructions map the
 *     RV32I/RV64I base loads/stores, INCLUDING byte/half:
 *     HLV.B/HLV.BU/HLV.H/HLV.HU and HSV.B/HSV.H exist for non-atomic
 *     guest byte/half access. But there is NO byte/half ATOMIC (AMO)
 *     guest equivalent, so HS-mode cannot do an atomic byte/half RMW on
 *     guest memory with VS/VU effective privilege.
 *   norm:hlsv_priv - HLV/HLVX/HSV effective privilege is hstatus.SPVP.
 * =================================================================== */

/* ------------------------------------------------------------------
 * HZABHA-38 (record-type): HLV.B/HLV.H/HSV.B/HSV.H exist for non-atomic
 *           guest byte/half access, but there is no byte/half guest-atomic
 *           equivalent.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_38_no_guest_bh_atomic_equivalent);
bool test_hzabha_38_no_guest_bh_atomic_equivalent(void)
{
    TEST_BEGIN("HZABHA-38: (record) HLV.B/H + HSV.B/H exist, no atomic equiv");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);

    /* HLV.B / HLV.H read guest byte/half. */
    trap_expect_begin();
    int8_t gb = hlv_b(va);
    bool hlvb_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("HLV.B (guest byte load) available", !hlvb_fired);
    TEST_ASSERT_EQ("HLV.B read the guest byte", gb, (int8_t)0x77);

    trap_expect_begin();
    int16_t gh = hlv_h(va);
    bool hlvh_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("HLV.H (guest half load) available", !hlvh_fired);
    TEST_ASSERT_EQ("HLV.H read the guest halfword", gh, (int16_t)0x6677);

    /* HSV.B writes a guest byte. */
    trap_expect_begin();
    hsv_b(va, 0x5Au);
    bool hsvb_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("HSV.B (guest byte store) available", !hsvb_fired);
    TEST_ASSERT_EQ("HSV.B wrote the guest byte",
                   *(volatile uint8_t *)va, (uintptr_t)0x5Au);

    ts2_finish(&ctx);

    printf("  [RECORD] HLV.B/HLV.H/HSV.B/HSV.H provide non-atomic guest "
           "byte/half access; no byte/half ATOMIC guest equivalent exists "
           "(norm:hlsv_op)\n");
    TEST_ASSERT("record-type boundary case executed", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZABHA-39 (record-type): HLV.B+HSV.B / HLV.H+HSV.H cannot replace
 *           byte/half AMO atomicity under concurrency. Requires a secondary
 *           hart; SKIP.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzabha_39_hlv_hsv_not_atomic);
bool test_hzabha_39_hlv_hsv_not_atomic(void)
{
    TEST_BEGIN("HZABHA-39: (record) HLV+HSV cannot replace byte/half AMO atomicity");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    TEST_SKIP(HZABHA_SMP_SKIP_REASON);
    HYP_TEST_END();
}
