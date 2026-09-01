/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.1: vsstatus.vs field and vector instruction/CSR gating
 * (HVEC-01 ~ HVEC-08)
 *
 * Spec anchors (vector-common.adoc):
 *   norm:vsstatus_vs_sz_acc
 *     - with the H extension, vsstatus gains a VS field at [10:9],
 *       defined analogously to FS.
 *   norm:vsstatus_vs_mstatus_vs_op_off
 *     - V=1: both vsstatus.vs and mstatus.vs are in effect; any
 *       vector instruction or vector-CSR access raises
 *       illegal-instruction when either field is Off.
 *   norm:vsstatus_vs_mstatus_vs_op_active
 *     - V=1 with neither Off, any instruction changing vector state
 *       sets BOTH fields to Dirty.
 *   norm:hw_mstatus_vs_dirty_update
 *     - implementations MAY promote Initial/Clean to Dirty at any
 *       time (record-only case).
 *   norm:vsstatus_sd_op_vs
 *     - vsstatus.vs=Dirty implies vsstatus.sd=1.
 * =================================================================== */

/* ---- HVEC-01: vsstatus.vs field read/write ---- */

TEST_REGISTER(test_hvec_01);
bool test_hvec_01(void)
{
    TEST_BEGIN("HVEC-01: vsstatus.vs field read/write");
    H_REQUIRED_OR_SKIP();
    V_REQUIRED_OR_SKIP();

    /* Bits [10:9] must be writable/readable when H is present. */
    unsigned vals[3] = { CTX_DIRTY, CTX_INITIAL, CTX_OFF };
    for (unsigned i = 0; i < 3; i++) {
        hvec_vsstatus_set_field(HVEC_VS_SHIFT, vals[i]);
        TEST_ASSERT_EQ("vsstatus.vs readback",
                       hvec_vsstatus_field(HVEC_VS_SHIFT), vals[i]);
    }

    HYP_TEST_END();
}

/* ---- HVEC-02: vsstatus.vs=Off gates vector instructions ---- */

TEST_REGISTER(test_hvec_02);
bool test_hvec_02(void)
{
    TEST_BEGIN("HVEC-02: vsstatus.vs=Off gates vector instructions");
    H_REQUIRED_OR_SKIP();
    V_REQUIRED_OR_SKIP();

    hvec_mstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_OFF);

    /* Single vector instruction (vsetivli): with vsstatus.vs=Off it
     * must raise illegal-instruction. A one-instruction callback is
     * used so exactly one armed trap can occur. */
    trap_expect_begin();
    (void)run_in_vs_mode(hvec_vs_vset, 0);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();

    TEST_ASSERT("VS-mode vector instruction trapped", trapped);
    if (trapped) {
        TEST_ASSERT_EQ("cause=2 (illegal-instruction, not virtual)",
                       cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    HYP_TEST_END();
}

/* ---- HVEC-03: mstatus.vs=Off gates vector instructions ---- */

TEST_REGISTER(test_hvec_03);
bool test_hvec_03(void)
{
    TEST_BEGIN("HVEC-03: mstatus.vs=Off gates vector instructions");
    H_REQUIRED_OR_SKIP();
    V_REQUIRED_OR_SKIP();

    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
    hvec_mstatus_set_field(HVEC_VS_SHIFT, CTX_OFF);

    /* Single vector instruction (vsetivli), see HVEC-02. */
    trap_expect_begin();
    (void)run_in_vs_mode(hvec_vs_vset, 0);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();

    TEST_ASSERT("VS-mode vector instruction trapped", trapped);
    if (trapped) {
        TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                       cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    HYP_TEST_END();
}

/* ---- HVEC-04: Off gates vector CSR access ---- */

TEST_REGISTER(test_hvec_04);
bool test_hvec_04(void)
{
    TEST_BEGIN("HVEC-04: vsstatus.vs=Off gates vector CSR access");
    H_REQUIRED_OR_SKIP();
    V_REQUIRED_OR_SKIP();

    hvec_mstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_OFF);

    /* csrr x0, vstart from VS-mode must be gated like instructions. */
    trap_expect_begin();
    (void)run_in_vs_mode(hvec_vs_vcsr_read, 0);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();

    TEST_ASSERT("VS-mode vector CSR access trapped", trapped);
    if (trapped) {
        TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                       cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    HYP_TEST_END();
}

/* ---- HVEC-05: both non-Off -> VS/VU normal execution ---- */

TEST_REGISTER(test_hvec_05);
bool test_hvec_05(void)
{
    TEST_BEGIN("HVEC-05: both non-Off -> VS/VU normal execution");
    H_REQUIRED_OR_SKIP();
    V_REQUIRED_OR_SKIP();

    hvec_mstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);

    trap_expect_begin();
    (void)run_in_vs_mode(hvec_vs_vseq, 0);
    bool vs_trapped = trap_was_triggered();
    trap_expect_end();

    trap_expect_begin();
    (void)run_in_vu_mode(hvec_vu_vseq, 0);
    bool vu_trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VS-mode vector sequence: no trap", !vs_trapped);
    TEST_ASSERT("VU-mode vector sequence: no trap", !vu_trapped);

    HYP_TEST_END();
}

/* ---- HVEC-06: vector state change sets both fields Dirty ---- */

TEST_REGISTER(test_hvec_06);
bool test_hvec_06(void)
{
    TEST_BEGIN("HVEC-06: vector state change sets both fields Dirty");
    H_REQUIRED_OR_SKIP();
    V_REQUIRED_OR_SKIP();

    hvec_mstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);

    trap_expect_begin();
    (void)run_in_vs_mode(hvec_vs_vseq, 0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VS-mode vector sequence: no trap", !trapped);
    if (!trapped) {
        TEST_ASSERT_EQ("mstatus.vs == Dirty",
                       hvec_mstatus_field(HVEC_VS_SHIFT), CTX_DIRTY);
        TEST_ASSERT_EQ("vsstatus.vs == Dirty",
                       hvec_vsstatus_field(HVEC_VS_SHIFT), CTX_DIRTY);
    }

    HYP_TEST_END();
}

/* ---- HVEC-07: vsstatus.sd linkage with vsstatus.vs ---- */

TEST_REGISTER(test_hvec_07);
bool test_hvec_07(void)
{
    TEST_BEGIN("HVEC-07: vsstatus.sd linkage with vsstatus.vs");
    H_REQUIRED_OR_SKIP();
    V_REQUIRED_OR_SKIP();

    /* Zero the other VS-visible context fields so SD reflects VS. */
    hvec_vsstatus_set_field(HVEC_FS_SHIFT, CTX_OFF);

    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_DIRTY);
    TEST_ASSERT("vs=Dirty -> vsstatus.sd=1",
                (hvec_vsstatus_read() & HVEC_SD_BIT) != 0);

    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
    TEST_ASSERT("vs=Initial (others non-Dirty) -> vsstatus.sd=0",
                (hvec_vsstatus_read() & HVEC_SD_BIT) == 0);

    HYP_TEST_END();
}

/* ---- HVEC-08: record-only Initial/Clean -> Dirty promotion ---- */

TEST_REGISTER(test_hvec_08);
bool test_hvec_08(void)
{
    TEST_BEGIN("HVEC-08: (record) Initial/Clean -> Dirty promotion");
    H_REQUIRED_OR_SKIP();
    V_REQUIRED_OR_SKIP();

    /* norm:hw_mstatus_vs_dirty_update permits the implementation to
     * promote Initial/Clean to Dirty at any time, even without a
     * vector state change. Run the vector sequence in M-mode (V=0,
     * so only mstatus.vs is governed) starting from Clean and record
     * the outcome; both Clean retention and Dirty promotion are legal
     * - only Off or Initial would be wrong. */
    hvec_mstatus_set_field(HVEC_VS_SHIFT, CTX_CLEAN);

    trap_expect_begin();
    HVEC_EXEC_VSEQ();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("M-mode vector sequence: no trap", !trapped);

    unsigned after = hvec_mstatus_field(HVEC_VS_SHIFT);
    printf("[I] mstatus.vs after Clean + vector seq: %u "
           "(Clean=2 retention or Dirty=3 promotion both legal)\n", after);
    TEST_ASSERT("mstatus.vs is Clean or Dirty after vector sequence",
                after == CTX_CLEAN || after == CTX_DIRTY);

    HYP_TEST_END();
}
