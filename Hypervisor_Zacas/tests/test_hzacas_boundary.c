/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 3.7 - Architectural boundary: no amocas guest-atomic equivalent
 *             (HZACAS-34 ~ HZACAS-35)
 *
 * Spec basis:
 *   norm:hlsv_op  - the virtual-machine load/store instructions map only
 *     the RV32I/RV64I base loads/stores. amocas (opcode 0x2F, funct5=00101)
 *     has NO virtual-machine equivalent, so HS-mode cannot perform an
 *     atomic CAS on guest memory with VS/VU effective privilege.
 *   norm:hlsv_priv - HLV/HLVX/HSV effective privilege is hstatus.SPVP.
 * =================================================================== */

/* ------------------------------------------------------------------
 * HZACAS-34 (record-type): HLV/HSV exist for guest load/store, but there
 *           is no guest-atomic amocas equivalent.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_34_no_guest_cas_equivalent);
bool test_hzacas_34_no_guest_cas_equivalent(void)
{
    TEST_BEGIN("HZACAS-34: (record) HLV/HSV exist, no guest-amocas equivalent");
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

    printf("  [RECORD] HLV/HSV cover base loads/stores only; no guest-amocas "
           "(opcode 0x2F, funct5=00101) equivalent exists (norm:hlsv_op)\n");
    TEST_ASSERT("record-type boundary case executed", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZACAS-35 (record-type): HLV+HSV cannot replace amocas atomicity under
 *           concurrency (TOCTOU on the software compare-exchange). Requires
 *           a secondary hart; SKIP.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzacas_35_hlv_hsv_not_atomic);
bool test_hzacas_35_hlv_hsv_not_atomic(void)
{
    TEST_BEGIN("HZACAS-35: (record) HLV+HSV cannot replace amocas atomicity");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    TEST_SKIP(HZACAS_SMP_SKIP_REASON);
    HYP_TEST_END();
}
