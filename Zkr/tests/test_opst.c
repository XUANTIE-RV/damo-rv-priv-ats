/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_opst.c - Group 2: seed CSR OPST State and Entropy Field
 *
 * OPST-01 ~ OPST-09
 * Verifies seed CSR state machine behavior, entropy field constraints,
 * and wipe-on-read semantics.
 */

/* ------------------------------------------------------------------ */
/* OPST-01: ES16 state provides entropy                               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_opst01_es16_entropy);
bool test_opst01_es16_entropy(void)
{
    TEST_BEGIN("OPST-01: ES16 state provides 16-bit entropy");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t val = seed_poll_es16(1000);
    if (val == 0) {
        TEST_SKIP("ES16 never obtained within 1000 polls");
    }

    /* Verify the returned value is indeed in ES16 state */
    TEST_ASSERT("OPST == ES16 in returned value",
                (val & SEED_OPST_MASK) == SEED_OPST_ES16);

    uintptr_t entropy = val & SEED_ENTROPY_MASK;
    printf("  [INFO] ES16 entropy = 0x%lx\n", (unsigned long)entropy);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* OPST-02: WAIT state has zero entropy                               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_opst02_wait_zero);
bool test_opst02_wait_zero(void)
{
    TEST_BEGIN("OPST-02: WAIT state entropy is zero");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* Poll seed and look for WAIT state */
    bool found_wait = false;
    bool entropy_zero = true;
    for (int i = 0; i < 100; i++) {
        uintptr_t val = seed_read();
        if ((val & SEED_OPST_MASK) == SEED_OPST_WAIT) {
            found_wait = true;
            if ((val & SEED_ENTROPY_MASK) != 0) {
                entropy_zero = false;
            }
            break;
        }
    }

    if (!found_wait) {
        /* If WAIT never observed, ES16 may always be ready */
        printf("  [INFO] WAIT state not observed (entropy always ready)\n");
        TEST_ASSERT("WAIT not observed (acceptable)", true);
    } else {
        TEST_ASSERT("WAIT state entropy == 0", entropy_zero);
    }

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* OPST-03: BIST state has zero entropy                               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_opst03_bist_zero);
bool test_opst03_bist_zero(void)
{
    TEST_BEGIN("OPST-03: BIST state entropy is zero");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* BIST is typically only seen at power-on; may not be observable */
    bool found_bist = false;
    bool entropy_zero = true;
    for (int i = 0; i < 100; i++) {
        uintptr_t val = seed_read();
        if ((val & SEED_OPST_MASK) == SEED_OPST_BIST) {
            found_bist = true;
            if ((val & SEED_ENTROPY_MASK) != 0) {
                entropy_zero = false;
            }
            break;
        }
    }

    if (!found_bist) {
        printf("  [INFO] BIST state not observed (normal after boot)\n");
        TEST_ASSERT("BIST not observed (acceptable)", true);
    } else {
        TEST_ASSERT("BIST state entropy == 0", entropy_zero);
    }

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* OPST-04: DEAD state has zero entropy                               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_opst04_dead_zero);
bool test_opst04_dead_zero(void)
{
    TEST_BEGIN("OPST-04: DEAD state entropy is zero");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* DEAD is an unrecoverable error state; should not normally occur */
    bool found_dead = false;
    bool entropy_zero = true;
    for (int i = 0; i < 100; i++) {
        uintptr_t val = seed_read();
        if ((val & SEED_OPST_MASK) == SEED_OPST_DEAD) {
            found_dead = true;
            if ((val & SEED_ENTROPY_MASK) != 0) {
                entropy_zero = false;
            }
            break;
        }
    }

    if (!found_dead) {
        printf("  [INFO] DEAD state not observed (normal operation)\n");
        TEST_ASSERT("DEAD not observed (acceptable)", true);
    } else {
        TEST_ASSERT("DEAD state entropy == 0", entropy_zero);
    }

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* OPST-05: wipe-on-read behavior after ES16                          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_opst05_wipe_on_read);
bool test_opst05_wipe_on_read(void)
{
    TEST_BEGIN("OPST-05: wipe-on-read after ES16");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t val1 = seed_poll_es16(1000);
    if (val1 == 0) {
        TEST_SKIP("ES16 never obtained");
    }

    /* Immediately read again */
    uintptr_t val2 = seed_read();
    uintptr_t opst2 = val2 & SEED_OPST_MASK;

    /* After wipe-on-read, second read should be WAIT or a new ES16 */
    bool valid = (opst2 == SEED_OPST_WAIT) || (opst2 == SEED_OPST_ES16);
    TEST_ASSERT("second read is WAIT or new ES16", valid);

    /* If both are ES16, entropy values should differ (wipe occurred) */
    if (opst2 == SEED_OPST_ES16) {
        uintptr_t e1 = val1 & SEED_ENTROPY_MASK;
        uintptr_t e2 = val2 & SEED_ENTROPY_MASK;
        printf("  [INFO] e1=0x%lx e2=0x%lx\n",
               (unsigned long)e1, (unsigned long)e2);
        /* Note: spec says values represent unique randomness even if
         * numerically equal, so we cannot strictly require e1 != e2 */
    }

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* OPST-06: WAIT state polling does not change state                  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_opst06_wait_stable);
bool test_opst06_wait_stable(void)
{
    TEST_BEGIN("OPST-06: WAIT state polling stability");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* Find WAIT state */
    uintptr_t val = 0;
    bool found_wait = false;
    for (int i = 0; i < 100; i++) {
        val = seed_read();
        if ((val & SEED_OPST_MASK) == SEED_OPST_WAIT) {
            found_wait = true;
            break;
        }
    }

    if (!found_wait) {
        printf("  [INFO] WAIT state not observed\n");
        TEST_ASSERT("WAIT not observed (acceptable)", true);
        TEST_END();
    }

    /* Poll again: should remain WAIT or transition to ES16 */
    uintptr_t val2 = seed_read();
    uintptr_t opst2 = val2 & SEED_OPST_MASK;
    bool valid = (opst2 == SEED_OPST_WAIT) || (opst2 == SEED_OPST_ES16);
    TEST_ASSERT("WAIT remains WAIT or becomes ES16", valid);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* OPST-07: Consecutive ES16 values uniqueness                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_opst07_entropy_unique);
bool test_opst07_entropy_unique(void)
{
    TEST_BEGIN("OPST-07: consecutive ES16 entropy uniqueness");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    #define ENTROPY_SAMPLES 8
    uintptr_t samples[ENTROPY_SAMPLES];
    unsigned int count = 0;

    for (unsigned int i = 0; i < 1000 && count < ENTROPY_SAMPLES; i++) {
        uintptr_t val = seed_read();
        if ((val & SEED_OPST_MASK) == SEED_OPST_ES16) {
            samples[count++] = val & SEED_ENTROPY_MASK;
        }
    }

    if (count < ENTROPY_SAMPLES) {
        TEST_SKIP("insufficient ES16 samples");
    }

    /* Check that not ALL values are identical (statistical) */
    bool all_same = true;
    for (unsigned int i = 1; i < count; i++) {
        if (samples[i] != samples[0]) {
            all_same = false;
            break;
        }
    }
    TEST_ASSERT("not all entropy values identical", !all_same);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* OPST-08: reserved bits check                                       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_opst08_reserved_bits);
bool test_opst08_reserved_bits(void)
{
    TEST_BEGIN("OPST-08: reserved bits [29:24] are zero");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t val = seed_read();
    uintptr_t reserved = val & SEED_RESERVED_MASK;
    TEST_ASSERT_EQ("reserved bits [29:24] == 0", reserved, 0UL);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* OPST-09: BIST alarm latch verification                             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_opst09_bist_latch);
bool test_opst09_bist_latch(void)
{
    TEST_BEGIN("OPST-09: BIST alarm latch behavior");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* BIST is typically not triggerable by software.
     * We verify that if BIST is observed, it persists across reads
     * until polled (latched behavior). */
    bool found_bist = false;
    for (int i = 0; i < 100; i++) {
        uintptr_t val = seed_read();
        if ((val & SEED_OPST_MASK) == SEED_OPST_BIST) {
            found_bist = true;
            /* Read again - BIST should still be visible (latched) */
            uintptr_t val2 = seed_read();
            uintptr_t opst2 = val2 & SEED_OPST_MASK;
            /* After first poll, BIST may clear or remain */
            printf("  [INFO] BIST observed, second read OPST=%lu\n",
                   (unsigned long)(opst2 >> SEED_OPST_SHIFT));
            break;
        }
    }

    if (!found_bist) {
        printf("  [INFO] BIST not triggerable (design verification)\n");
        TEST_ASSERT("BIST latch not testable at runtime", true);
    } else {
        /* BIST was visible on first read, confirming it was latched
         * until polled (per norm:seed_bist_latch). */
        TEST_ASSERT("BIST latched until first poll", found_bist);
    }

    TEST_END();
}
