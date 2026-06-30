/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_csr_adue.c - Group 1: menvcfg.ADUE field control
 *
 * Per SPEC/svadu.adoc:6 (norm:Svadu_hw_update_a_d_bits) and
 * SPEC/machine.adoc:2247 (norm:menvcfg_adue_rdonly0):
 *   When Svadu is implemented, menvcfg.ADUE (bit 61) must be writable.
 *   When Svadu is not implemented, ADUE is read-only zero.
 *
 * Tests:
 *   SVADU-CSR-01: ADUE writable 0 -> 1
 *   SVADU-CSR-02: ADUE writable 1 -> 0
 *   SVADU-CSR-03: toggling ADUE does not disturb other menvcfg fields
 *   SVADU-CSR-04: record and print menvcfg.ADUE reset value (no hard assert)
 *
 * These tests do NOT call detect_svadu() themselves: CSR-01 acts as the
 * "hard" implementation probe; CSR-03 only runs meaningfully if ADUE is
 * writable, so it guards with get_menvcfg_adue() readback.
 */

TEST_REGISTER(test_svadu_csr01);
bool test_svadu_csr01(void) {
    TEST_BEGIN("SVADU-CSR-01: menvcfg.ADUE writable 0->1");

    /* Clear first to avoid relying on reset value */
    set_menvcfg_adue(0);
    TEST_ASSERT("ADUE cleared to 0 before write", get_menvcfg_adue() == 0);

    set_menvcfg_adue(1);
    /* If Svadu is not implemented, ADUE is read-only zero: this reads back 0
     * and the assertion fails, which itself is the evidence of non-impl. */
    TEST_ASSERT("Platform does not implement Svadu: ADUE reads back as 1 after write", get_menvcfg_adue() == 1);

    /* Restore to 0 for subsequent tests */
    set_menvcfg_adue(0);
    TEST_END();
}

TEST_REGISTER(test_svadu_csr02);
bool test_svadu_csr02(void) {
    TEST_BEGIN("SVADU-CSR-02: menvcfg.ADUE writable 1->0");
    SVADU_REQUIRED_OR_SKIP();

    set_menvcfg_adue(1);
    TEST_ASSERT("ADUE set to 1 precondition", get_menvcfg_adue() == 1);

    set_menvcfg_adue(0);
    TEST_ASSERT("ADUE cleared to 0 after write", get_menvcfg_adue() == 0);

    TEST_END();
}

TEST_REGISTER(test_svadu_csr03);
bool test_svadu_csr03(void) {
    TEST_BEGIN("SVADU-CSR-03: toggling ADUE does not disturb other menvcfg bits");
    SVADU_REQUIRED_OR_SKIP();

    /* Mask of the ADUE bit; all other bits should stay unchanged. */
    const uintptr_t ADUE_MASK = MENVCFG_ADUE;

    /* Baseline: ensure ADUE=0 and snapshot the rest of the register. */
    set_menvcfg_adue(0);
    if (menvcfg_read() & ADUE_MASK) {
        TEST_SKIP("Cannot clear ADUE (unexpected); skip");
    }
    uintptr_t baseline = menvcfg_read() & ~ADUE_MASK;

    /* Toggle ADUE to 1 */
    set_menvcfg_adue(1);
    uintptr_t after_set = menvcfg_read() & ~ADUE_MASK;
    TEST_ASSERT("other menvcfg bits unchanged after ADUE 0->1",
                after_set == baseline);

    /* Toggle back to 0 */
    set_menvcfg_adue(0);
    uintptr_t after_clear = menvcfg_read() & ~ADUE_MASK;
    TEST_ASSERT("other menvcfg bits unchanged after ADUE 1->0",
                after_clear == baseline);

    TEST_END();
}

TEST_REGISTER(test_svadu_csr04);
bool test_svadu_csr04(void) {
    TEST_BEGIN("SVADU-CSR-04: record menvcfg.ADUE reset value");

    /* g_menvcfg_reset_value is captured in main.c before any test ran,
     * so it reflects the true reset value (not modified by CSR-01~03). */
    int reset_adue = (g_menvcfg_reset_value & MENVCFG_ADUE) ? 1 : 0;
    printf("    [INFO] menvcfg reset value = 0x%lx, ADUE bit = %d\n",
           (unsigned long)g_menvcfg_reset_value, reset_adue);

    /* Implementation-defined; do not hard-assert. Always passes. */
    TEST_ASSERT("reset value recorded", 1);
    TEST_END();
}
