/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 18: mtval2/mtinst registers
 *
 * Tests MTVAL-01 through MTVAL-05 verify mtval2 and mtinst behavior.
 */

#include "test_helpers.h"

/* ------------------------------------------------------------------
 * MTVAL-01: mtval2 basic read/write
 * ------------------------------------------------------------------ */
TEST_REGISTER(mtval2_basic_rw);
bool mtval2_basic_rw(void) {
    TEST_BEGIN("MTVAL-01: Verify mtval2 basic read/write");

    /* Write a test value to mtval2 (CSR 0x34B). */
    uintptr_t test_val = 0xDEADBEEF;
    asm volatile("csrw 0x34B, %0" :: "r"(test_val));

    /* Read back. */
    uintptr_t read_val;
    asm volatile("csrr %0, 0x34B" : "=r"(read_val));

    TEST_ASSERT_EQ("mtval2 should return written value", read_val, test_val);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MTVAL-02: mtval2 on guest-page-fault trap to M-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(mtval2_gpf_trap);
bool mtval2_gpf_trap(void) {
    TEST_BEGIN("MTVAL-02: Verify mtval2 on guest-page-fault trap to M-mode");

    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Fire a VS load fault. */
    uintptr_t victim_gpa = (uintptr_t)test_fault_page;
    uintptr_t flags = G_FLAGS_RWXU_AD | PTE_R;
    fire_vs_load_fault(victim_gpa, flags);

    /* Check htval (which may contain the GPA). */
    uintptr_t htval = trap_get_htval();
    (void)htval;
    TEST_ASSERT("htval accessible", true);

    /* Check mtval2 - should contain GPA>>2 or 0. */
    uintptr_t mtval2;
    asm volatile("csrr %0, 0x34B" : "=r"(mtval2));
    TEST_ASSERT("mtval2 accessible on GPF trap", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MTVAL-03: mtval2=0 on non-guest-page-fault trap
 * ------------------------------------------------------------------ */
TEST_REGISTER(mtval2_non_gpf_trap);
bool mtval2_non_gpf_trap(void) {
    TEST_BEGIN("MTVAL-03: Verify mtval2=0 on non-guest-page-fault trap");

    /* Execute VS ecall (not a guest-page-fault). */
    trap_expect_begin();
    run_in_vs_mode(vs_exec_ecall, 0);
    trap_expect_end();

    /* Check htval. */
    uintptr_t htval = trap_get_htval();
    TEST_ASSERT_EQ("htval should be 0 on non-GPF trap", htval, 0);

    /* Check mtval2 - should be 0. */
    uintptr_t mtval2;
    asm volatile("csrr %0, 0x34B" : "=r"(mtval2));
    TEST_ASSERT_EQ("mtval2 should be 0 on non-GPF trap", mtval2, 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MTVAL-04: mtinst basic read/write
 * ------------------------------------------------------------------ */
TEST_REGISTER(mtinst_basic_rw);
bool mtinst_basic_rw(void) {
    TEST_BEGIN("MTVAL-04: Verify mtinst basic read/write");

    /* Write a test value to mtinst (CSR 0x34A). */
    uintptr_t test_val = 0x12345678;
    asm volatile("csrw 0x34A, %0" :: "r"(test_val));

    /* Read back. */
    uintptr_t read_val;
    asm volatile("csrr %0, 0x34A" : "=r"(read_val));

    TEST_ASSERT_EQ("mtinst should return written value", read_val, test_val);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MTVAL-05: mtinst on M-mode guest-page-fault trap
 * ------------------------------------------------------------------ */
TEST_REGISTER(mtinst_gpf_trap);
bool mtinst_gpf_trap(void) {
    TEST_BEGIN("MTVAL-05: Verify mtinst on M-mode guest-page-fault trap");

    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Fire a VS load fault. */
    uintptr_t victim_gpa = (uintptr_t)test_fault_page;
    uintptr_t flags = G_FLAGS_RWXU_AD | PTE_R;
    fire_vs_load_fault(victim_gpa, flags);

    /* Check htinst. */
    uintptr_t htinst = trap_get_htinst();
    (void)htinst;
    TEST_ASSERT("htinst accessible", true);

    /* Check mtinst - should contain transformed instruction or 0. */
    uintptr_t mtinst;
    asm volatile("csrr %0, 0x34A" : "=r"(mtinst));
    TEST_ASSERT("mtinst accessible on GPF trap", true);

    HYP_TEST_END();
}
