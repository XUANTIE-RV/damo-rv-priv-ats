/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_exist.c - Group 1: mstateen CSR Existence and Accessibility
 *
 * MSTA-EXIST-01 ~ MSTA-EXIST-05
 * Verifies that mstateen0-3 CSRs exist and are accessible in M-mode.
 */

/* ------------------------------------------------------------------ */
/* MSTA-EXIST-01: mstateen0 readable and writable in M-mode           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0_exist);
bool test_mstateen0_exist(void) {
    TEST_BEGIN("MSTA-EXIST-01: mstateen0 readable and writable");

    uintptr_t orig = 0;
    M_EXPECT_NO_TRAP(orig = mstateen0_read());
    M_EXPECT_NO_TRAP(mstateen0_write(orig));

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-EXIST-02: mstateen1 readable and writable in M-mode           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen1_exist);
bool test_mstateen1_exist(void) {
    TEST_BEGIN("MSTA-EXIST-02: mstateen1 readable and writable");

    uintptr_t orig = 0;
    M_EXPECT_NO_TRAP(orig = mstateen1_read());
    M_EXPECT_NO_TRAP(mstateen1_write(orig));

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-EXIST-03: mstateen2 readable and writable in M-mode           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen2_exist);
bool test_mstateen2_exist(void) {
    TEST_BEGIN("MSTA-EXIST-03: mstateen2 readable and writable");

    uintptr_t orig = 0;
    M_EXPECT_NO_TRAP(orig = mstateen2_read());
    M_EXPECT_NO_TRAP(mstateen2_write(orig));

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-EXIST-04: mstateen3 readable and writable in M-mode           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen3_exist);
bool test_mstateen3_exist(void) {
    TEST_BEGIN("MSTA-EXIST-04: mstateen3 readable and writable");

    uintptr_t orig = 0;
    M_EXPECT_NO_TRAP(orig = mstateen3_read());
    M_EXPECT_NO_TRAP(mstateen3_write(orig));

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSTA-EXIST-05: mstateen0h readable and writable (RV32 only)        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_mstateen0h_exist);
bool test_mstateen0h_exist(void) {
    TEST_BEGIN("MSTA-EXIST-05: mstateen0h readable and writable (RV32)");

#if __riscv_xlen == 32
    uintptr_t orig = 0;
    M_EXPECT_NO_TRAP({
        asm volatile("csrr %0, 0x31C" : "=r"(orig) :: "memory");
    });
    M_EXPECT_NO_TRAP({
        asm volatile("csrw 0x31C, %0" :: "r"(orig) : "memory");
    });
#else
    TEST_SKIP("RV32-only test");
#endif

    TEST_END();
}
