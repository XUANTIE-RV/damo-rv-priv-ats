/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_sinval_priv.c - Group 5: SINVAL.VMA Privilege Level Exceptions
 *
 * Tests:
 *   PRIV-01: U-mode SINVAL.VMA triggers illegal-instruction
 *   PRIV-02: S-mode TVM=0 SINVAL.VMA succeeds
 *   PRIV-03: S-mode TVM=1 SINVAL.VMA triggers illegal-instruction
 *   PRIV-04: M-mode SINVAL.VMA succeeds
 *
 * Verifies SINVAL.VMA privilege level access control.
 */

/* ===================================================================
 * PRIV-01: U-mode SINVAL.VMA triggers illegal-instruction
 * =================================================================== */
TEST_REGISTER(test_sinval_umode_illegal);
bool test_sinval_umode_illegal(void) {
    TEST_BEGIN("PRIV-01: SINVAL.VMA in U-mode triggers illegal instruction");

    /* Configure PMP entry 15 to allow U-mode code execution */
    pmp_entry_t fw = PMP_ENTRY_NAPOT(PLATFORM_MEM_BASE, 0x4000000, PMP_RWX);
    pmp_set_entry(15, &fw);

    goto_priv(PRIV_U);

    /* U-mode: SINVAL.VMA should trigger illegal-instruction */
    PRIV_DO_TRAP(SINVAL_VMA(0, 0));

    goto_priv(PRIV_M);
    CHECK_TRAP("PRIV-01: U-mode SINVAL.VMA triggers illegal instruction",
               CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* ===================================================================
 * PRIV-02: S-mode TVM=0 SINVAL.VMA succeeds
 * =================================================================== */
TEST_REGISTER(test_sinval_smode_tvm0);
bool test_sinval_smode_tvm0(void) {
    TEST_BEGIN("PRIV-02: SINVAL.VMA in S-mode with TVM=0 succeeds");

    /* Ensure TVM=0 */
    CSRC(mstatus, MSTATUS_TVM_BIT);

    /* Configure PMP entry 15 to allow S-mode code execution */
    pmp_entry_t fw = PMP_ENTRY_NAPOT(PLATFORM_MEM_BASE, 0x4000000, PMP_RWX);
    pmp_set_entry(15, &fw);

    goto_priv(PRIV_S);

    /* S-mode: SINVAL.VMA with TVM=0 should succeed */
    PRIV_DO_NO_TRAP(SINVAL_VMA(0, 0));

    goto_priv(PRIV_M);
    CHECK_NO_TRAP("PRIV-02: S-mode TVM=0 SINVAL.VMA succeeds");

    TEST_END();
}

/* ===================================================================
 * PRIV-03: S-mode TVM=1 SINVAL.VMA triggers illegal-instruction
 * =================================================================== */
TEST_REGISTER(test_sinval_smode_tvm1);
bool test_sinval_smode_tvm1(void) {
    TEST_BEGIN("PRIV-03: SINVAL.VMA in S-mode with TVM=1 triggers illegal instruction");

    /* Set mstatus.TVM=1 */
    CSRS(mstatus, MSTATUS_TVM_BIT);

    /* Configure PMP entry 15 to allow S-mode code execution */
    pmp_entry_t fw = PMP_ENTRY_NAPOT(PLATFORM_MEM_BASE, 0x4000000, PMP_RWX);
    pmp_set_entry(15, &fw);

    goto_priv(PRIV_S);

    /* S-mode: SINVAL.VMA with TVM=1 should trigger illegal-instruction */
    PRIV_DO_TRAP(SINVAL_VMA(0, 0));

    goto_priv(PRIV_M);
    CHECK_TRAP("PRIV-03: S-mode TVM=1 SINVAL.VMA triggers illegal instruction",
               CAUSE_ILLEGAL_INST);

    /* Clear TVM */
    CSRC(mstatus, MSTATUS_TVM_BIT);

    TEST_END();
}

/* ===================================================================
 * PRIV-04: M-mode SINVAL.VMA succeeds
 * =================================================================== */
TEST_REGISTER(test_sinval_mmode);
bool test_sinval_mmode(void) {
    TEST_BEGIN("PRIV-04: SINVAL.VMA in M-mode succeeds");

    /* M-mode: SINVAL.VMA should always succeed */
    EXPECT_NO_TRAP(SINVAL_VMA(0, 0));

    TEST_END();
}
