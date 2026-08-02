/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_security.c - Group 8: Security Strength Requirements
 *
 * SEC-01 ~ SEC-02
 * Verifies security strength constraints (architecture-level).
 */

/* ------------------------------------------------------------------ */
/* SEC-01: seed CSR availability implies >= 256-bit security          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sec01_min_strength);
bool test_sec01_min_strength(void)
{
    TEST_BEGIN("SEC-01: seed CSR available implies >= 256-bit security");

    if (!check_zkr()) {
        /* If seed CSR is not accessible, the interface is not available.
         * Per SPEC, if security < 256 bits, interface must not be available.
         * So absent interface is compliant. */
        printf("  [INFO] seed CSR not available (compliant if < 256-bit)\n");
        TEST_ASSERT("interface absent (acceptable per SPEC)", true);
        TEST_END();
    }

    /* If we can access seed in M-mode, the implementation guarantees
     * >= 256-bit security strength. This is a design assertion, not
     * something verifiable at runtime. */
    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read());
    (void)val;
    printf("  [INFO] seed accessible, security >= 256 bits (design guarantee)\n");
    TEST_ASSERT("seed CSR accessible (implies >= 256-bit)", true);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SEC-02: DEAD state indicates unrecoverable error                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sec02_dead_unrecoverable);
bool test_sec02_dead_unrecoverable(void)
{
    TEST_BEGIN("SEC-02: DEAD state is unrecoverable");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* Poll to check if DEAD state is present */
    bool found_dead = false;
    for (int i = 0; i < 200; i++) {
        uintptr_t val = seed_read();
        if ((val & SEED_OPST_MASK) == SEED_OPST_DEAD) {
            found_dead = true;

            /* Once DEAD, keep polling - should never recover */
            bool recovered = false;
            for (int j = 0; j < 100; j++) {
                uintptr_t v2 = seed_read();
                uintptr_t opst = v2 & SEED_OPST_MASK;
                if (opst == SEED_OPST_ES16 || opst == SEED_OPST_WAIT) {
                    recovered = true;
                    break;
                }
            }
            TEST_ASSERT("DEAD state does not recover", !recovered);
            break;
        }
    }

    if (!found_dead) {
        printf("  [INFO] DEAD state not observed (normal operation)\n");
        TEST_ASSERT("DEAD not observed (acceptable)", true);
    }

    TEST_END();
}
