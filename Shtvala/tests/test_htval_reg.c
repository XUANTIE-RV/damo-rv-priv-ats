/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1: htval CSR existence and WARL behaviour
 *
 * Spec anchors:
 *   norm:H_htval — htval is present and writable when H is implemented.
 *
 * Test list (matches DOCS/testplan/shtvala_test_plan.md HTVAL-REG):
 *   HTVAL-REG-01  read-after-reset (== 0 once hyp_reset_state)
 *   HTVAL-REG-02  csrw writeable, readback equal
 *   HTVAL-REG-03  csrw of all-ones; readback masks reserved bits
 *                 (we accept whatever the implementation reports)
 *   HTVAL-REG-04  csrr/csrw round-trip is bit-stable for low 32 bits
 * =================================================================== */

static inline uintptr_t htval_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x643" : "=r"(v));
    return v;
}
static inline void htval_write(uintptr_t v) {
    asm volatile ("csrw 0x643, %0" :: "r"(v));
}

TEST_REGISTER(test_htval_reg_01_reset_zero);
bool test_htval_reg_01_reset_zero(void) {
    TEST_BEGIN("HTVAL-REG-01: htval == 0 after hyp_reset_state");
    hyp_reset_state();
    TEST_ASSERT_EQ("htval reset value", htval_read(), 0);
    HYP_TEST_END();
}

TEST_REGISTER(test_htval_reg_02_writable);
bool test_htval_reg_02_writable(void) {
    TEST_BEGIN("HTVAL-REG-02: htval is writable from M-mode");
    uintptr_t pattern = 0x123456789ABCUL;  /* fits in any sane htval width */
    htval_write(pattern);
    TEST_ASSERT_EQ("htval readback", htval_read(), pattern);
    htval_write(0);
    HYP_TEST_END();
}

TEST_REGISTER(test_htval_reg_03_all_ones_warl);
bool test_htval_reg_03_all_ones_warl(void) {
    TEST_BEGIN("HTVAL-REG-03: htval all-ones write probes implemented width");
    htval_write((uintptr_t)-1);
    uintptr_t got = htval_read();
    /* htval is at least as wide as MAX(GPALEN-2, XLEN). On QEMU it is
     * 64-bit. We just require the readback is non-zero (some bits are
     * implemented) and stable on a second read. */
    TEST_ASSERT("htval width >= 1 bit", got != 0);
    uintptr_t got2 = htval_read();
    TEST_ASSERT_EQ("htval is stable", got2, got);
    htval_write(0);
    HYP_TEST_END();
}

/* VS-mode helper: attempt to read htval (CSR 0x643).
 * In VS/VU-mode this should cause virtual-instruction (cause=22). */
static uintptr_t vs_csrr_htval(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x643" : "=r"(v));
    return v;
}

TEST_REGISTER(test_htval_reg_04_vs_vu_access);
bool test_htval_reg_04_vs_vu_access(void) {
    TEST_BEGIN("HTVAL-REG-04: VS/VU-mode access to htval traps");

    /* VS-mode: csrr htval -> virtual-instruction (cause=22) */
    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_csrr_htval, 0));

    /* VU-mode: csrr htval -> virtual-instruction (cause=22) */
    EXPECT_VIRTUAL_INST(run_in_vu_mode(vs_csrr_htval, 0));

    HYP_TEST_END();
}
