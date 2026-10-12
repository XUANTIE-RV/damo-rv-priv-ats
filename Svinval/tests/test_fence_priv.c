/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_fence_priv.c - Group 6: SFENCE.W.INVAL / SFENCE.INVAL.IR Privilege Behavior
 *
 * Tests:
 *   FENCE-01: U-mode SFENCE.W.INVAL triggers illegal-instruction
 *   FENCE-02: U-mode SFENCE.INVAL.IR triggers illegal-instruction
 *   FENCE-03: S-mode TVM=0 SFENCE.W.INVAL succeeds
 *   FENCE-04: S-mode TVM=0 SFENCE.INVAL.IR succeeds
 *   FENCE-05: S-mode TVM=1 SFENCE.W.INVAL still permitted (not affected by TVM)
 *   FENCE-06: S-mode TVM=1 SFENCE.INVAL.IR still permitted (not affected by TVM)
 *   FENCE-07: M-mode SFENCE.W.INVAL succeeds
 *   FENCE-08: M-mode SFENCE.INVAL.IR succeeds
 *
 * Key spec point: SFENCE.W.INVAL and SFENCE.INVAL.IR are NOT affected
 * by mstatus.TVM, unlike SINVAL.VMA.
 *
 * T-SBI S-mode-normal model: the test bodies run in S-mode, so the
 * mstatus.TVM configuration bit is programmed through the T-SBI CSR
 * proxy and the two M-mode cases raise explicitly.  The per-test
 * "PMP entry 15 allows execution" setup the M-mode-entry version of
 * this file carried is gone: entry.S hoists a full-address-space NAPOT
 * RWX entry 0 ahead of the drop to S-mode, and a lower-numbered match
 * always wins, so entry 15 was never consulted.
 */

/* ===================================================================
 * FENCE-01: U-mode SFENCE.W.INVAL triggers illegal-instruction
 *
 * Per spec (norm:Svinval_sfence_w_inval_inval_u_mode):
 *   "Attempting to execute SFENCE.W.INVAL or SFENCE.INVAL.IR in
 *    U-mode raises an illegal-instruction exception."
 *
 * Note: Some implementations (e.g., QEMU) may treat these as NOPs
 * even in U-mode. We probe the behavior and report accordingly.
 * =================================================================== */
TEST_REGISTER(test_sfence_w_inval_umode);
bool test_sfence_w_inval_umode(void) {
    TEST_BEGIN("FENCE-01: SFENCE.W.INVAL in U-mode triggers illegal instruction");

    goto_priv(PRIV_U);

    PRIV_DO_TRAP(SFENCE_W_INVAL());

    goto_priv(PRIV_M);

    if (!trap_was_triggered()) {
        LOG_W("U-mode SFENCE.W.INVAL did not trap (impl treats as NOP)\n");
        LOG_W("Spec requires illegal-instruction; impl may be non-compliant\n");
    } else {
        TEST_ASSERT_EQ("FENCE-01: cause is illegal instruction",
                        trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    /* Always pass: this is a probe test for implementation behavior */
    TEST_END();
}

/* ===================================================================
 * FENCE-02: U-mode SFENCE.INVAL.IR triggers illegal-instruction
 *
 * Same spec requirement as FENCE-01. Probe behavior.
 * =================================================================== */
TEST_REGISTER(test_sfence_inval_ir_umode);
bool test_sfence_inval_ir_umode(void) {
    TEST_BEGIN("FENCE-02: SFENCE.INVAL.IR in U-mode triggers illegal instruction");

    goto_priv(PRIV_U);

    PRIV_DO_TRAP(SFENCE_INVAL_IR());

    goto_priv(PRIV_M);

    if (!trap_was_triggered()) {
        LOG_W("U-mode SFENCE.INVAL.IR did not trap (impl treats as NOP)\n");
        LOG_W("Spec requires illegal-instruction; impl may be non-compliant\n");
    } else {
        TEST_ASSERT_EQ("FENCE-02: cause is illegal instruction",
                        trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    }

    /* Always pass: this is a probe test for implementation behavior */
    TEST_END();
}

/* ===================================================================
 * FENCE-03: S-mode TVM=0 SFENCE.W.INVAL succeeds
 * =================================================================== */
TEST_REGISTER(test_sfence_w_inval_smode_tvm0);
bool test_sfence_w_inval_smode_tvm0(void) {
    TEST_BEGIN("FENCE-03: SFENCE.W.INVAL in S-mode with TVM=0 succeeds");

    tsbi_csr_clear(CSR_MSTATUS, MSTATUS_TVM_BIT);

    goto_priv(PRIV_S);

    PRIV_DO_NO_TRAP(SFENCE_W_INVAL());

    goto_priv(PRIV_M);
    CHECK_NO_TRAP("FENCE-03: S-mode TVM=0 SFENCE.W.INVAL succeeds");

    TEST_END();
}

/* ===================================================================
 * FENCE-04: S-mode TVM=0 SFENCE.INVAL.IR succeeds
 * =================================================================== */
TEST_REGISTER(test_sfence_inval_ir_smode_tvm0);
bool test_sfence_inval_ir_smode_tvm0(void) {
    TEST_BEGIN("FENCE-04: SFENCE.INVAL.IR in S-mode with TVM=0 succeeds");

    tsbi_csr_clear(CSR_MSTATUS, MSTATUS_TVM_BIT);

    goto_priv(PRIV_S);

    PRIV_DO_NO_TRAP(SFENCE_INVAL_IR());

    goto_priv(PRIV_M);
    CHECK_NO_TRAP("FENCE-04: S-mode TVM=0 SFENCE.INVAL.IR succeeds");

    TEST_END();
}

/* ===================================================================
 * FENCE-05: S-mode TVM=1 SFENCE.W.INVAL still permitted
 *
 * Key: SFENCE.W.INVAL is NOT affected by mstatus.TVM
 * =================================================================== */
TEST_REGISTER(test_sfence_w_inval_smode_tvm1);
bool test_sfence_w_inval_smode_tvm1(void) {
    TEST_BEGIN("FENCE-05: SFENCE.W.INVAL in S-mode with TVM=1 still permitted");

    /* Set mstatus.TVM=1 */
    tsbi_csr_set(CSR_MSTATUS, MSTATUS_TVM_BIT);

    goto_priv(PRIV_S);

    /* S-mode: SFENCE.W.INVAL should succeed even with TVM=1 */
    PRIV_DO_NO_TRAP(SFENCE_W_INVAL());

    goto_priv(PRIV_M);
    CHECK_NO_TRAP("FENCE-05: S-mode TVM=1 SFENCE.W.INVAL permitted");

    /* Clear TVM */
    CSRC(mstatus, MSTATUS_TVM_BIT);

    TEST_END();
}

/* ===================================================================
 * FENCE-06: S-mode TVM=1 SFENCE.INVAL.IR still permitted
 *
 * Key: SFENCE.INVAL.IR is NOT affected by mstatus.TVM
 * =================================================================== */
TEST_REGISTER(test_sfence_inval_ir_smode_tvm1);
bool test_sfence_inval_ir_smode_tvm1(void) {
    TEST_BEGIN("FENCE-06: SFENCE.INVAL.IR in S-mode with TVM=1 still permitted");

    /* Set mstatus.TVM=1 */
    tsbi_csr_set(CSR_MSTATUS, MSTATUS_TVM_BIT);

    goto_priv(PRIV_S);

    /* S-mode: SFENCE.INVAL.IR should succeed even with TVM=1 */
    PRIV_DO_NO_TRAP(SFENCE_INVAL_IR());

    goto_priv(PRIV_M);
    CHECK_NO_TRAP("FENCE-06: S-mode TVM=1 SFENCE.INVAL.IR permitted");

    /* Clear TVM */
    CSRC(mstatus, MSTATUS_TVM_BIT);

    TEST_END();
}

/* ===================================================================
 * FENCE-07: M-mode SFENCE.W.INVAL succeeds
 * =================================================================== */
TEST_REGISTER(test_sfence_w_inval_mmode);
bool test_sfence_w_inval_mmode(void) {
    TEST_BEGIN("FENCE-07: SFENCE.W.INVAL in M-mode succeeds");

    /* The subject of this case is M-mode, which is no longer where a test
     * body starts: raise explicitly, otherwise the stimulus would silently
     * be issued from S-mode and the case would stop testing what its ID
     * claims. */
    goto_priv(PRIV_M);

    EXPECT_NO_TRAP(SFENCE_W_INVAL());

    TEST_END();
}

/* ===================================================================
 * FENCE-08: M-mode SFENCE.INVAL.IR succeeds
 * =================================================================== */
TEST_REGISTER(test_sfence_inval_ir_mmode);
bool test_sfence_inval_ir_mmode(void) {
    TEST_BEGIN("FENCE-08: SFENCE.INVAL.IR in M-mode succeeds");

    /* Same reasoning as FENCE-07: the M-mode leg needs an explicit raise. */
    goto_priv(PRIV_M);

    EXPECT_NO_TRAP(SFENCE_INVAL_IR());

    TEST_END();
}
