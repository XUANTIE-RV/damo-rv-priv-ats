/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_encoding.c - Group 7: Instruction Encoding Verification
 *
 * Tests:
 *   ENC-01: SINVAL.VMA instruction encoding valid
 *   ENC-02: SFENCE.W.INVAL instruction encoding valid
 *   ENC-03: SFENCE.INVAL.IR instruction encoding valid
 *
 * Verifies that Svinval instructions are recognized by the hardware
 * and do not trigger illegal-instruction exceptions in M-mode.
 * This is a prerequisite for all other functional tests.
 */

/* ===================================================================
 * ENC-01: SINVAL.VMA instruction encoding valid
 * =================================================================== */
TEST_REGISTER(test_sinval_vma_encoding);
bool test_sinval_vma_encoding(void) {
    TEST_BEGIN("ENC-01: SINVAL.VMA instruction encoding valid");

    /* M-mode: SINVAL.VMA should not trigger any exception */
    EXPECT_NO_TRAP(SINVAL_VMA(0, 0));

    TEST_END();
}

/* ===================================================================
 * ENC-02: SFENCE.W.INVAL instruction encoding valid
 * =================================================================== */
TEST_REGISTER(test_sfence_w_inval_encoding);
bool test_sfence_w_inval_encoding(void) {
    TEST_BEGIN("ENC-02: SFENCE.W.INVAL instruction encoding valid");

    EXPECT_NO_TRAP(SFENCE_W_INVAL());

    TEST_END();
}

/* ===================================================================
 * ENC-03: SFENCE.INVAL.IR instruction encoding valid
 * =================================================================== */
TEST_REGISTER(test_sfence_inval_ir_encoding);
bool test_sfence_inval_ir_encoding(void) {
    TEST_BEGIN("ENC-03: SFENCE.INVAL.IR instruction encoding valid");

    EXPECT_NO_TRAP(SFENCE_INVAL_IR());

    TEST_END();
}
