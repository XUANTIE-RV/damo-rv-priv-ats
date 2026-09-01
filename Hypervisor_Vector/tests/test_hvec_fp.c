/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.2: vector floating-point gating via vsstatus.fs
 * (HVEC-09 ~ HVEC-12)
 *
 * Spec anchors (vector-common.adoc):
 *   norm:vsstatus_mstatus_FS_off_hypervisor_V_fp_ill
 *     - H implemented and V=1: vsstatus.fs is additionally in effect
 *       for vector FP instructions; if vsstatus.fs OR mstatus.fs is
 *       Off, any vector FP instruction raises illegal-instruction.
 *   norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp
 *     - any vector FP instruction modifying FP state must set BOTH
 *       mstatus.fs and vsstatus.fs to Dirty.
 *
 * Prerequisite: V and F declared by the platform configuration
 * (V_SUPPORTED/F_SUPPORTED in rvtest_config.h); otherwise the whole
 * group skips (VF_REQUIRED_OR_SKIP).
 * =================================================================== */

/* ---- HVEC-09: vsstatus.fs=Off gates vector FP ---- */

TEST_REGISTER(test_hvec_09);
bool test_hvec_09(void)
{
    TEST_BEGIN("HVEC-09: vsstatus.fs=Off gates vector FP instructions");
    H_REQUIRED_OR_SKIP();
    VF_REQUIRED_OR_SKIP();

    hvec_mstatus_set_field(HVEC_FS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_FS_SHIFT, CTX_OFF);
    /* Vector context must stay enabled so the trap cause is the FP
     * gating, not the VS gating. */
    hvec_mstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);

    trap_expect_begin();
    (void)run_in_vs_mode(hvec_vs_vfseq, 0);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();

    TEST_ASSERT("VS-mode vector FP instruction trapped", trapped);
    if (trapped) {
        TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                       cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    HYP_TEST_END();
}

/* ---- HVEC-10: mstatus.fs=Off gates vector FP ---- */

TEST_REGISTER(test_hvec_10);
bool test_hvec_10(void)
{
    TEST_BEGIN("HVEC-10: mstatus.fs=Off gates vector FP instructions");
    H_REQUIRED_OR_SKIP();
    VF_REQUIRED_OR_SKIP();

    hvec_vsstatus_set_field(HVEC_FS_SHIFT, CTX_DIRTY);
    hvec_mstatus_set_field(HVEC_FS_SHIFT, CTX_OFF);
    hvec_mstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);

    trap_expect_begin();
    (void)run_in_vs_mode(hvec_vs_vfseq, 0);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();

    TEST_ASSERT("VS-mode vector FP instruction trapped", trapped);
    if (trapped) {
        TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                       cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    HYP_TEST_END();
}

/* ---- HVEC-11: VU-mode vector FP gating ---- */

TEST_REGISTER(test_hvec_11);
bool test_hvec_11(void)
{
    TEST_BEGIN("HVEC-11: VU-mode vector FP gating (vsstatus.fs=Off)");
    H_REQUIRED_OR_SKIP();
    VF_REQUIRED_OR_SKIP();

    hvec_mstatus_set_field(HVEC_FS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_FS_SHIFT, CTX_OFF);
    hvec_mstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);

    trap_expect_begin();
    (void)run_in_vu_mode(hvec_vu_vfseq, 0);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();

    TEST_ASSERT("VU-mode vector FP instruction trapped", trapped);
    if (trapped) {
        TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                       cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    HYP_TEST_END();
}

/* ---- HVEC-12: FP state change sets both fs fields Dirty ---- */

TEST_REGISTER(test_hvec_12);
bool test_hvec_12(void)
{
    TEST_BEGIN("HVEC-12: vector FP sets both fs fields Dirty");
    H_REQUIRED_OR_SKIP();
    VF_REQUIRED_OR_SKIP();

    hvec_mstatus_set_field(HVEC_FS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_FS_SHIFT, CTX_INITIAL);
    hvec_mstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
    hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);

    trap_expect_begin();
    (void)run_in_vs_mode(hvec_vs_vfseq, 0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VS-mode vector FP sequence: no trap", !trapped);
    if (!trapped) {
        /* vfdiv.vv computes 0/0, which deterministically raises the
         * NV flag: fcsr changes, i.e. FP state is modified, so both
         * fs fields must be Dirty afterwards. */
        TEST_ASSERT_EQ("mstatus.fs == Dirty",
                       hvec_mstatus_field(HVEC_FS_SHIFT), CTX_DIRTY);
        TEST_ASSERT_EQ("vsstatus.fs == Dirty",
                       hvec_vsstatus_field(HVEC_FS_SHIFT), CTX_DIRTY);
    }

    HYP_TEST_END();
}
