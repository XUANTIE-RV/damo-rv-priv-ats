/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 3.8 - hstateen0 does not gate amocas (HZACAS-36)
 *
 * Spec basis:
 *   norm:stateen_op / norm:stateen_illegal_state_access - a stateen CSR
 *     gates access to PROTECTED STATE; the gate fires only for an
 *     instruction that reads/writes that state. Zacas has no CSR and
 *     introduces no architectural state, so no stateen bit can be assigned
 *     to it and amocas must NOT be gated.
 *   norm:stateen_unimplemented_state_roz - stateen bits controlling
 *     unimplemented state are read-only zero (corroborates that no stateen
 *     bit is assigned to the Za atomic instructions).
 *   norm:mstateen0_se0_op - mstateen0.SE0 must stay 1 so HS-mode can
 *     itself access hstateen0 (otherwise the precondition cannot be set).
 *
 * Clearing hstateen0 to 0 (most restrictive) must NOT prevent VS/VU-mode
 * amocas from executing, and must NOT raise cause=2 or cause=22.
 * =================================================================== */

TEST_REGISTER(test_hzacas_36_hstateen0_no_gate);
bool test_hzacas_36_hstateen0_no_gate(void)
{
    TEST_BEGIN("HZACAS-36: hstateen0=0 does not gate amocas in VS/VU");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    if (!SMSTATEEN_AVAILABLE)
        TEST_SKIP("Smstateen not implemented");

    /* Keep mstateen0.SE0=1 so HS-mode can access hstateen0. */
    mstateen_set_bit63(0, true);

    uintptr_t saved = hstateen_read(0);
    hstateen_write(0, 0);
    uintptr_t rb = hstateen_read(0);
    if (rb != 0) {
        hstateen_write(0, saved);
        TEST_SKIP("hstateen0 write did not take effect (cannot establish "
                  "the gated precondition)");
    }

    uintptr_t va = (uintptr_t)test_data_area;
    two_stage_ctx_t ctx;

    /* VS-mode: success-path amocas.w, expect no trap / no cause=2/22. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint32_t *)va = 0x00001000u;
    hz_cas_cmp = 0x00001000u;
    hz_cas_swap = 0x00002000u;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired_vs = trap_was_triggered();
    uintptr_t cause_vs = fired_vs ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    /* VS-mode: failure-path amocas.w. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint32_t *)va = 0x00003000u;
    hz_cas_cmp = 0x00009999u;   /* mismatch */
    hz_cas_swap = 0x00004000u;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_w_probe, va);
    bool fired_vs_f = trap_was_triggered();
    uintptr_t cause_vs_f = fired_vs_f ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    /* VU-mode: success-path amocas.w (U=1 pages). */
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint32_t *)va = 0x00005000u;
    hz_cas_cmp = 0x00005000u;
    hz_cas_swap = 0x00006000u;
    trap_expect_begin();
    (void)two_stage_run_in_vu(&ctx, hz_vs_amocas_w_probe, va);
    bool fired_vu = trap_was_triggered();
    uintptr_t cause_vu = fired_vu ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

#if __riscv_xlen == 64
    /* VS-mode: amocas.d success path. */
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    *(volatile uint64_t *)va = 0x0000000010000000ULL;
    hz_cas_cmp = 0x0000000010000000ULL;
    hz_cas_swap = 0x0000000020000000ULL;
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_amocas_d_probe, va);
    bool fired_vs_d = trap_was_triggered();
    uintptr_t cause_vs_d = fired_vs_d ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);
#endif

    hstateen_write(0, saved);   /* restore */

    printf("  [INFO] hstateen0=0: VS fired=%d cause=%lu | VS-fail fired=%d "
           "cause=%lu | VU fired=%d cause=%lu\n",
           (int)fired_vs, (unsigned long)cause_vs,
           (int)fired_vs_f, (unsigned long)cause_vs_f,
           (int)fired_vu, (unsigned long)cause_vu);

    TEST_ASSERT("VS-mode amocas (success) with hstateen0=0 took no trap",
                !fired_vs);
    TEST_ASSERT_NEQ("VS-mode amocas not gated (no cause=22)",
                    cause_vs, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_NEQ("VS-mode amocas not gated (no cause=2)",
                    cause_vs, (uintptr_t)CAUSE_ILLEGAL_INST);
    TEST_ASSERT("VS-mode amocas (failure) with hstateen0=0 took no trap",
                !fired_vs_f);
    TEST_ASSERT_NEQ("VS-mode failed amocas not gated (no cause=22)",
                    cause_vs_f, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT("VU-mode amocas with hstateen0=0 took no trap", !fired_vu);
    TEST_ASSERT_NEQ("VU-mode amocas not gated (no cause=22)",
                    cause_vu, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_NEQ("VU-mode amocas not gated (no cause=2)",
                    cause_vu, (uintptr_t)CAUSE_ILLEGAL_INST);
#if __riscv_xlen == 64
    TEST_ASSERT("VS-mode amocas.d with hstateen0=0 took no trap", !fired_vs_d);
    TEST_ASSERT_NEQ("VS-mode amocas.d not gated (no cause=22)",
                    cause_vs_d, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
#endif

    HYP_TEST_END();
}
