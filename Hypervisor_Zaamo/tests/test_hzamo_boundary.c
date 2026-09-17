/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2.7 - Architectural boundary: no AMO guest-atomic equivalent
 *             (HZAMO-30 ~ HZAMO-31)
 *
 * Spec basis:
 *   norm:hlsv_op  - the virtual-machine load/store instructions map only
 *     the RV32I/RV64I base loads/stores (LB..LD -> HLV, SB..SD -> HSV).
 *     AMOs (opcode 0x2F) have NO virtual-machine equivalent, so HS-mode
 *     cannot perform an atomic RMW on guest memory with VS/VU effective
 *     privilege (shared conclusion with Group 1's HZLRSC-39/40).
 *   norm:hlsv_priv - HLV/HLVX/HSV effective privilege is hstatus.SPVP.
 * =================================================================== */

/* ------------------------------------------------------------------
 * HZAMO-30 (record-type): HLV/HSV exist for guest load/store, but there
 *           is no guest-atomic AMO equivalent.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_30_no_guest_amo_equivalent);
bool test_hzamo_30_no_guest_amo_equivalent(void)
{
    TEST_BEGIN("HZAMO-30: (record) HLV/HSV exist, no guest-AMO equivalent");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint64_t *)va = 0xDEADBEEF12345678ULL;

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);

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

    printf("  [RECORD] HLV/HSV cover base loads/stores only; no guest-AMO "
           "(opcode 0x2F) equivalent exists (norm:hlsv_op)\n");
    TEST_ASSERT("record-type boundary case executed", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZAMO-31 (record-type): HLV+HSV cannot replace AMO atomicity under
 *           concurrency. Requires a secondary hart; SKIP.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzamo_31_hlv_hsv_not_atomic);
bool test_hzamo_31_hlv_hsv_not_atomic(void)
{
    TEST_BEGIN("HZAMO-31: (record) HLV+HSV cannot replace AMO atomicity");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    TEST_SKIP(HZAMO_SMP_SKIP_REASON);
    HYP_TEST_END();
}
