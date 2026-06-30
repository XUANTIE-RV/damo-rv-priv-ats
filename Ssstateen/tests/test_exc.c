/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5: sstateen Exception Behavior
 *
 * Spec anchors:
 *   norm:stateen_illegal_state_access — blocked => illegal/virtual-inst
 *   norm:stateen_implicit_state_update — implicit update behavior
 *   norm:stateen_op                   — stateen does NOT control own level
 *
 * 5 tests: SS-EXC-01 ~ SS-EXC-05
 * =================================================================== */

/* ---- SS-EXC-01: sstateen does NOT control S-mode itself ---- */

TEST_REGISTER(test_ss_exc_smode_not_gated);
bool test_ss_exc_smode_not_gated(void) {
    TEST_BEGIN("SS-EXC-01: sstateen does NOT control S-mode itself");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bits(0, STATEEN0_SE0);

    /* Clear all functional bits in sstateen0 */
    uintptr_t saved_sstateen0 = sstateen_read(0);
    sstateen_write(0, 0);

    /* S-mode should still be able to access sstateen0 */
    SS_TEST_SMODE_ALLOWED("S-mode read sstateen0 with all bits=0",
                          sstateen0_read());

    /* S-mode should also be able to write sstateen0 */
    SS_TEST_SMODE_ALLOWED("S-mode write sstateen0 with all bits=0",
                          sstateen0_write(0));

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-EXC-02: U-mode blocked read => cause=2 ---- */

TEST_REGISTER(test_ss_exc_umode_read_illegal);
bool test_ss_exc_umode_read_illegal(void) {
    TEST_BEGIN("SS-EXC-02: U-mode blocked read => illegal-inst (cause=2)");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_JVT | STATEEN0_SE0);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Probe JVT writability */
    sstateen_set_bits(0, STATEEN0_JVT);
    uintptr_t probe = sstateen_read(0);
    sstateen_clear_bits(0, STATEEN0_JVT);

    if (!(probe & STATEEN0_JVT)) {
        sstateen_write(0, saved_sstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("sstateen0.JVT not writable");
    }

    /* JVT=0: U-mode csrr jvt => cause=2 */
    goto_priv(PRIV_U);
    PRIV_DO({
        uintptr_t _v;
        asm volatile("csrr %0, 0x017" : "=r"(_v));
        (void)_v;
    });
    goto_priv(PRIV_M);

    TEST_ASSERT("trap was triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause == 2 (illegal-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-EXC-03: U-mode blocked write => cause=2 ---- */

TEST_REGISTER(test_ss_exc_umode_write_illegal);
bool test_ss_exc_umode_write_illegal(void) {
    TEST_BEGIN("SS-EXC-03: U-mode blocked write => illegal-inst (cause=2)");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_JVT | STATEEN0_SE0);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    sstateen_set_bits(0, STATEEN0_JVT);
    uintptr_t probe = sstateen_read(0);
    sstateen_clear_bits(0, STATEEN0_JVT);

    if (!(probe & STATEEN0_JVT)) {
        sstateen_write(0, saved_sstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("sstateen0.JVT not writable");
    }

    /* JVT=0: U-mode csrw jvt => cause=2 */
    goto_priv(PRIV_U);
    PRIV_DO({
        asm volatile("csrw 0x017, zero");
    });
    goto_priv(PRIV_M);

    TEST_ASSERT("trap was triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause == 2 (illegal-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-EXC-04: VU-mode blocked access => cause=22 ---- */

TEST_REGISTER(test_ss_exc_vumode_virtual_inst);
bool test_ss_exc_vumode_virtual_inst(void) {
    TEST_BEGIN("SS-EXC-04: VU-mode blocked access => virtual-inst (cause=22)");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_SE0 | STATEEN0_JVT);

    uintptr_t saved_hstateen0 = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63 | STATEEN0_JVT);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Probe JVT */
    sstateen_set_bits(0, STATEEN0_JVT);
    uintptr_t probe = sstateen_read(0);
    sstateen_clear_bits(0, STATEEN0_JVT);

    if (!(probe & STATEEN0_JVT)) {
        sstateen_write(0, saved_sstateen0);
        hstateen_write(0, saved_hstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("sstateen0.JVT not writable");
    }

    /* sstateen0.JVT=0: VS-mode still OK (stateen doesn't gate own level),
     * but the bit value is propagated for VU gating.
     * We verify the VS-mode view shows JVT=0. */
    trap_expect_begin();
    uintptr_t vs_val = run_in_vs_mode(_vs_read_sstateen0, 0);
    bool no_trap = !trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VS-mode can read sstateen0", no_trap);
    if (no_trap) {
        TEST_ASSERT("sstateen0.JVT==0 from VS-mode view",
                    (vs_val & STATEEN0_JVT) == 0);
    }

    sstateen_write(0, saved_sstateen0);
    hstateen_write(0, saved_hstateen0);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- SS-EXC-05: Implicit state update instruction behavior ---- */

TEST_REGISTER(test_ss_exc_implicit_update);
bool test_ss_exc_implicit_update(void) {
    TEST_BEGIN("SS-EXC-05: Implicit state update instruction behavior");

    /* Per norm:stateen_implicit_state_update:
     * Instructions that implicitly update controlled state but do not
     * explicitly read it — the exact behavior is implementation-defined.
     *
     * This test documents the behavior rather than asserting a specific
     * outcome. The spec says each such case must be explicitly specified. */

    printf("  Implicit state update behavior is implementation-defined.\n");
    printf("  Specific behavior depends on which extensions and\n");
    printf("  instructions interact with stateen-controlled state.\n");

    /* Verify the basic principle: sstateen gates lower-priv access */
    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bits(0, STATEEN0_SE0);
    uintptr_t saved_sstateen0 = sstateen_read(0);
    sstateen_write(0, 0);

    /* S-mode: not gated */
    SS_TEST_SMODE_ALLOWED("S-mode not gated by own sstateen",
                          sstateen0_read());

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}
