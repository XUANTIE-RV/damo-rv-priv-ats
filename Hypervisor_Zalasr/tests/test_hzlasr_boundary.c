/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.8 - architectural boundary: no atomic-ordered load/store guest
 *             equivalent (HZLASR-35 ~ HZLASR-36)
 *
 * Spec basis:
 *   norm:hlsv_op - the virtual-machine load/store instructions map only
 *     the RV32I/RV64I base loads/stores (opcode 0x03 LB..LD / 0x23 SB..SD).
 *     load-acquire/store-release (opcode 0x2F) have NO HLV/HSV equivalent.
 *     BUT, unlike an AMO/CAS (a read-modify-write whose data cannot be
 *     atomically replicated by HLV+HSV), a load-acquire/store-release is
 *     itself a pure load/store, so HLV.W can replicate the load-acquire
 *     DATA transfer and HSV.W the store-release DATA transfer - while
 *     losing (a) the single-copy atomicity guarantee and (b) the
 *     acquire/release RCsc ordering annotation (HLV/HSV have no aq/rl
 *     variants). This "data replicable, atomicity/ordering not" boundary
 *     is subtler than Group 2/3.
 *   norm:hlsv_priv - HLV/HLVX/HSV effective privilege is hstatus.SPVP.
 * =================================================================== */

/* ------------------------------------------------------------------
 * HZLASR-35 (record-type): no atomic-ordered load/store guest equivalent
 *           exists, but HLV.W/HSV.W replicate the DATA transfer.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_35_no_atomic_ordered_guest_equiv);
bool test_hzlasr_35_no_atomic_ordered_guest_equiv(void)
{
    TEST_BEGIN("HZLASR-35: (record) no atomic-ordered guest equiv; HLV/HSV copy data");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    hzlasr_store_le32(va, 0x00005678u);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);

    /* HLV.W replicates the load-acquire DATA transfer (not its atomicity). */
    trap_expect_begin();
    int32_t gw = hlv_w(va);
    bool hlv_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("HLV.W (guest word load) available", !hlv_fired);
    TEST_ASSERT_EQ("HLV.W read the guest word (data transfer replicated)",
                   (uintptr_t)gw, (uintptr_t)0x00005678u);

    /* HSV.W replicates the store-release DATA transfer (not its atomicity). */
    trap_expect_begin();
    hsv_w(va, 0x00009abcu);
    bool hsv_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("HSV.W (guest word store) available", !hsv_fired);
    TEST_ASSERT_EQ("HSV.W wrote the guest word (data transfer replicated)",
                   hzlasr_load_le32(va), (uintptr_t)0x00009abcu);

    ts2_finish(&ctx);

    printf("  [RECORD] HLV.W/HSV.W replicate the load-acquire/store-release "
           "DATA transfer, but NO atomic-ordered (aq/rl) guest equivalent "
           "exists: HLV/HSV have no aq/rl variants and are not single-copy "
           "atomic (norm:hlsv_op maps only base opcode 0x03/0x23)\n");
    TEST_ASSERT("record-type boundary case executed", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZLASR-36 (record-type): HLV/HSV cannot replicate the load-acquire/
 *           store-release single-copy atomicity and RCsc ordering under
 *           concurrency. Requires a secondary hart; SKIP.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzlasr_36_hlv_hsv_not_atomic);
bool test_hzlasr_36_hlv_hsv_not_atomic(void)
{
    TEST_BEGIN("HZLASR-36: (record) HLV/HSV cannot replicate Zalasr atomicity/RCsc");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    TEST_SKIP(HZLASR_SMP_SKIP_REASON);
    HYP_TEST_END();
}
