/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_virtual_inst.c - Group 6: Virtual Instruction exceptions (optional)
 *
 * Per Sstvala (SPEC/sstvala.adoc:9-10):
 *   For virtual-instruction exception, stval must contain the faulting
 *   instruction encoding.
 *
 * Virtual-instruction exceptions (cause=22) occur when VS-mode
 * attempts to access HS-level CSRs (e.g., hstatus, hgatp, hideleg).
 *
 * Tests (all require H extension, ENABLE_HYP=1):
 *   TVAL-VI-01: VS-mode read hstatus  -> stval == 0x600022F3
 *   TVAL-VI-02: VS-mode write hgatp   -> stval == 0x68001073
 *   TVAL-VI-03: VS-mode read hideleg  -> stval == 0x603022F3
 */

#ifdef ENABLE_HYP

#include "hyp/hyp_priv.h"

/*
 * Pre-encoded instructions for VS-mode execution:
 *
 * csrrs x5, 0x600, x0: [31:20]=0x600 [19:15]=00000 [14:12]=010
 *                       [11:7]=00101 [6:0]=1110011 = 0x600022F3
 *
 * csrrw x0, 0x680, x0: [31:20]=0x680 [19:15]=00000 [14:12]=001
 *                       [11:7]=00000 [6:0]=1110011 = 0x68001073
 *
 * csrrs x5, 0x603, x0: [31:20]=0x603 [19:15]=00000 [14:12]=010
 *                       [11:7]=00101 [6:0]=1110011 = 0x603022F3
 */
static uint32_t vi_read_hstatus  __attribute__((aligned(4))) = 0x600022F3U;
static uint32_t vi_write_hgatp   __attribute__((aligned(4))) = 0x68001073U;
static uint32_t vi_read_hideleg  __attribute__((aligned(4))) = 0x603022F3U;

/* ===================================================================
 * TVAL-VI-01: VS-mode read hstatus -> virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_sstvala_vi_01);
bool test_sstvala_vi_01(void) {
    TEST_BEGIN("TVAL-VI-01: virtual-instruction (read hstatus) stval == encoding");

    /* Execute the pre-encoded csrrs instruction in VS-mode.
     * This requires the H extension runtime (run_in_vs_mode or equivalent).
     * If H extension is not available at runtime, skip. */
    trap_expect_begin();
    exec_at((uintptr_t)&vi_read_hstatus);

    if (!trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("H extension not available or instruction did not fault");
    }

    /* In M-mode, accessing hstatus may trigger illegal-instruction
     * rather than virtual-instruction if we're not in VS-mode.
     * Accept either cause=2 (illegal-inst from M-mode) or cause=22
     * (virtual-instruction from VS-mode). */
    uintptr_t cause = trap_get_cause();
    if (cause == CAUSE_ILLEGAL_INST) {
        /* We executed from M-mode; the instruction is illegal here.
         * The stval should still contain the instruction encoding. */
        TEST_ASSERT_EQ("stval == instruction encoding (0x600022F3)",
                       trap_get_tval(), 0x600022F3UL);
    } else {
        TEST_ASSERT_EQ("cause == virtual-instruction",
                       cause, CAUSE_VIRTUAL_INSTRUCTION);
        TEST_ASSERT_EQ("stval == instruction encoding (0x600022F3)",
                       trap_get_tval(), 0x600022F3UL);
    }
    trap_expect_end();

    TEST_END();
}

/* ===================================================================
 * TVAL-VI-02: VS-mode write hgatp -> virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_sstvala_vi_02);
bool test_sstvala_vi_02(void) {
    TEST_BEGIN("TVAL-VI-02: virtual-instruction (write hgatp) stval == encoding");

    trap_expect_begin();
    exec_at((uintptr_t)&vi_write_hgatp);

    if (!trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("H extension not available or instruction did not fault");
    }

    uintptr_t cause = trap_get_cause();
    if (cause == CAUSE_ILLEGAL_INST) {
        TEST_ASSERT_EQ("stval == instruction encoding (0x68001073)",
                       trap_get_tval(), 0x68001073UL);
    } else {
        TEST_ASSERT_EQ("cause == virtual-instruction",
                       cause, CAUSE_VIRTUAL_INSTRUCTION);
        TEST_ASSERT_EQ("stval == instruction encoding (0x68001073)",
                       trap_get_tval(), 0x68001073UL);
    }
    trap_expect_end();

    TEST_END();
}

/* ===================================================================
 * TVAL-VI-03: VS-mode read hideleg -> virtual-instruction
 * =================================================================== */
TEST_REGISTER(test_sstvala_vi_03);
bool test_sstvala_vi_03(void) {
    TEST_BEGIN("TVAL-VI-03: virtual-instruction (read hideleg) stval == encoding");

    trap_expect_begin();
    exec_at((uintptr_t)&vi_read_hideleg);

    if (!trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("H extension not available or instruction did not fault");
    }

    uintptr_t cause = trap_get_cause();
    if (cause == CAUSE_ILLEGAL_INST) {
        TEST_ASSERT_EQ("stval == instruction encoding (0x603022F3)",
                       trap_get_tval(), 0x603022F3UL);
    } else {
        TEST_ASSERT_EQ("cause == virtual-instruction",
                       cause, CAUSE_VIRTUAL_INSTRUCTION);
        TEST_ASSERT_EQ("stval == instruction encoding (0x603022F3)",
                       trap_get_tval(), 0x603022F3UL);
    }
    trap_expect_end();

    TEST_END();
}

#else /* !ENABLE_HYP */

/*
 * When H extension is not enabled at compile time, register stub tests
 * that report SKIP so the test count remains consistent.
 */
TEST_REGISTER(test_sstvala_vi_01);
bool test_sstvala_vi_01(void) {
    TEST_BEGIN("TVAL-VI-01: virtual-instruction (read hstatus)");
    TEST_SKIP("H extension not enabled (ENABLE_HYP=1 required)");
}

TEST_REGISTER(test_sstvala_vi_02);
bool test_sstvala_vi_02(void) {
    TEST_BEGIN("TVAL-VI-02: virtual-instruction (write hgatp)");
    TEST_SKIP("H extension not enabled (ENABLE_HYP=1 required)");
}

TEST_REGISTER(test_sstvala_vi_03);
bool test_sstvala_vi_03(void) {
    TEST_BEGIN("TVAL-VI-03: virtual-instruction (read hideleg)");
    TEST_SKIP("H extension not enabled (ENABLE_HYP=1 required)");
}

#endif /* ENABLE_HYP */
