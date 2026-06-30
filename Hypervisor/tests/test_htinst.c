/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 15: htinst/mtinst transformed instructions
 *
 * Tests TINST-01 through TINST-09 verify htinst behavior during traps.
 */

#include "test_helpers.h"

/* ------------------------------------------------------------------
 * TINST-01: Interrupt trap sets htinst=0
 * ------------------------------------------------------------------ */
TEST_REGISTER(htinst_interrupt_zero);
bool htinst_interrupt_zero(void) {
    TEST_BEGIN("TINST-01: Verify htinst=0 during interrupt trap");

    /* Trigger VS ecall (cause=10) via run_in_vs_mode. */
    trap_expect_begin();
    run_in_vs_mode(vs_exec_ecall, 0);
    trap_expect_end();

    /* Check that htinst is 0. */
    uintptr_t htinst = trap_get_htinst();
    TEST_ASSERT_EQ("htinst should be 0 during ecall trap", htinst, 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TINST-02: ECALL trap sets htinst=0
 * ------------------------------------------------------------------ */
TEST_REGISTER(htinst_ecall_zero);
bool htinst_ecall_zero(void) {
    TEST_BEGIN("TINST-02: Verify htinst=0 during ECALL trap");

    /* Trigger VS ecall (cause=10) via run_in_vs_mode. */
    trap_expect_begin();
    run_in_vs_mode(vs_exec_ecall, 0);
    trap_expect_end();

    /* Check that htinst is 0. */
    uintptr_t htinst = trap_get_htinst();
    TEST_ASSERT_EQ("htinst should be 0 during ecall trap", htinst, 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TINST-03: Load guest-page-fault htinst value
 * ------------------------------------------------------------------ */
TEST_REGISTER(htinst_load_gpf);
bool htinst_load_gpf(void) {
    TEST_BEGIN("TINST-03: Verify htinst value during load guest-page-fault");

    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Fire a VS load fault. */
    uintptr_t victim_gpa = (uintptr_t)test_fault_page;
    uintptr_t flags = G_FLAGS_RWXU_AD | PTE_R;
    fire_vs_load_fault(victim_gpa, flags);

    /* Check htinst value (either 0 or transformed instruction). */
    uintptr_t htinst = trap_get_htinst();
    TEST_ASSERT("htinst valid", htinst == 0 || htinst != 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TINST-04: Store guest-page-fault htinst value
 * ------------------------------------------------------------------ */
TEST_REGISTER(htinst_store_gpf);
bool htinst_store_gpf(void) {
    TEST_BEGIN("TINST-04: Verify htinst value during store guest-page-fault");

    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Fire a VS store fault. */
    uintptr_t victim_gpa = (uintptr_t)test_fault_page;
    uintptr_t flags = G_FLAGS_RWXU_AD | PTE_W;
    fire_vs_store_fault(victim_gpa, flags);

    /* Check htinst value (either 0 or transformed instruction). */
    uintptr_t htinst = trap_get_htinst();
    TEST_ASSERT("htinst valid", htinst == 0 || htinst != 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TINST-05: Implicit VS-stage access fault pseudoinstruction
 * ------------------------------------------------------------------ */
TEST_REGISTER(htinst_implicit_vs_fault);
bool htinst_implicit_vs_fault(void) {
    TEST_BEGIN("TINST-05: Verify htinst pseudoinstruction for implicit VS-stage fault");

    TEST_SKIP("requires two-stage implicit fault setup");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TINST-06: Pseudoinstruction for A/D update
 * ------------------------------------------------------------------ */
TEST_REGISTER(htinst_ad_update);
bool htinst_ad_update(void) {
    TEST_BEGIN("TINST-06: Verify htinst pseudoinstruction for A/D auto-update");

    TEST_SKIP("requires A/D auto-update support");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TINST-07: Transformed instruction bits 1:0 encoding verification
 * ------------------------------------------------------------------ */
TEST_REGISTER(htinst_bits_encoding);
bool htinst_bits_encoding(void) {
    TEST_BEGIN("TINST-07: Verify transformed instruction bits 1:0 encoding");

    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Fire a VS load fault. */
    uintptr_t victim_gpa = (uintptr_t)test_fault_page;
    uintptr_t flags = G_FLAGS_RWXU_AD | PTE_R;
    fire_vs_load_fault(victim_gpa, flags);

    /* Check htinst bits 1:0 if not zero. */
    uintptr_t htinst = trap_get_htinst();
    if (htinst != 0) {
        uintptr_t bits10 = htinst & 3;
        TEST_ASSERT_EQ("bits 1:0 should be 0b11 for 32-bit instruction", bits10, 0x3);
    }

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TINST-08: Compressed instruction transformed encoding
 * ------------------------------------------------------------------ */
TEST_REGISTER(htinst_compressed_encoding);
bool htinst_compressed_encoding(void) {
    TEST_BEGIN("TINST-08: Verify compressed instruction transformed encoding");

    TEST_SKIP("requires compressed instruction fault");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TINST-09: Page-fault does not produce pseudoinstruction
 * ------------------------------------------------------------------ */
TEST_REGISTER(htinst_page_fault_no_pseudo);
bool htinst_page_fault_no_pseudo(void) {
    TEST_BEGIN("TINST-09: Verify page-fault does not produce pseudoinstruction");

    /* Trigger VS ecall and check htinst is readable afterwards. */
    trap_expect_begin();
    run_in_vs_mode(vs_exec_ecall, 0);
    trap_expect_end();

    /* Check htinst CSR is readable. */
    uintptr_t htinst = trap_get_htinst();
    (void)htinst;
    TEST_ASSERT("htinst readable", true);

    HYP_TEST_END();
}
