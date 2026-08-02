/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mseccfg.c - Group 7: mseccfg SSEED/USEED Field Properties
 *
 * MSECFG-01 ~ MSECFG-06
 * Verifies WARL attributes and implementation detection of
 * SSEED/USEED fields in mseccfg.
 */

/* ------------------------------------------------------------------ */
/* MSECFG-01: SSEED field write and readback                          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_msecfg01_sseed_rw);
bool test_msecfg01_sseed_rw(void)
{
    TEST_BEGIN("MSECFG-01: SSEED field write and readback");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t orig = mseccfg_read_zkr();

    /* Write SSEED=1 */
    mseccfg_set_bits(MSECCFG_SSEED);
    uintptr_t val1 = mseccfg_read_zkr();

    /* Write SSEED=0 */
    mseccfg_clear_bits(MSECCFG_SSEED);
    uintptr_t val0 = mseccfg_read_zkr();

    mseccfg_write_zkr(orig);

    if (val1 & MSECCFG_SSEED) {
        printf("  [INFO] SSEED writable: set=1 read=1\n");
        TEST_ASSERT("SSEED=1 reads back 1", true);
        TEST_ASSERT_EQ("SSEED=0 reads back 0",
                       val0 & MSECCFG_SSEED, 0UL);
    } else {
        printf("  [INFO] SSEED is read-only zero\n");
        TEST_ASSERT_EQ("SSEED read-only zero", val1 & MSECCFG_SSEED, 0UL);
    }

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSECFG-02: USEED field write and readback                          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_msecfg02_useed_rw);
bool test_msecfg02_useed_rw(void)
{
    TEST_BEGIN("MSECFG-02: USEED field write and readback");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t orig = mseccfg_read_zkr();

    /* Write USEED=1 */
    mseccfg_set_bits(MSECCFG_USEED);
    uintptr_t val1 = mseccfg_read_zkr();

    /* Write USEED=0 */
    mseccfg_clear_bits(MSECCFG_USEED);
    uintptr_t val0 = mseccfg_read_zkr();

    mseccfg_write_zkr(orig);

    if (val1 & MSECCFG_USEED) {
        printf("  [INFO] USEED writable: set=1 read=1\n");
        TEST_ASSERT("USEED=1 reads back 1", true);
        TEST_ASSERT_EQ("USEED=0 reads back 0",
                       val0 & MSECCFG_USEED, 0UL);
    } else {
        printf("  [INFO] USEED is read-only zero\n");
        TEST_ASSERT_EQ("USEED read-only zero", val1 & MSECCFG_USEED, 0UL);
    }

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSECFG-03: SSEED/USEED initial value detection                     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_msecfg03_initial_values);
bool test_msecfg03_initial_values(void)
{
    TEST_BEGIN("MSECFG-03: SSEED/USEED initial value detection");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* Note: reset_state() may have modified mseccfg already.
     * We just record the current values as informational. */
    uintptr_t val = mseccfg_read_zkr();
    unsigned int sseed = (val & MSECCFG_SSEED) ? 1 : 0;
    unsigned int useed = (val & MSECCFG_USEED) ? 1 : 0;

    printf("  [INFO] current SSEED=%u, USEED=%u\n", sseed, useed);
    /* Reset values are implementation-defined; just verify readable */
    TEST_ASSERT("mseccfg readable", true);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSECFG-04: SSEED=0 blocks S-mode access                            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_msecfg04_sseed0_blocks);
bool test_msecfg04_sseed0_blocks(void)
{
    TEST_BEGIN("MSECFG-04: SSEED=0 blocks S-mode seed access");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!sseed_writable()) {
        TEST_SKIP("SSEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED);

    goto_priv(PRIV_S);
    PRIV_DO(SMODE_SEED_CSRRW());
    goto_priv(PRIV_M);
    CHECK_TRAP("S-mode blocked with SSEED=0", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSECFG-05: USEED=0 blocks U-mode access                            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_msecfg05_useed0_blocks);
bool test_msecfg05_useed0_blocks(void)
{
    TEST_BEGIN("MSECFG-05: USEED=0 blocks U-mode seed access");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!useed_writable()) {
        TEST_SKIP("USEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_USEED);

    goto_priv(PRIV_U);
    PRIV_DO(UMODE_SEED_CSRRW());
    goto_priv(PRIV_M);
    CHECK_TRAP("U-mode blocked with USEED=0", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MSECFG-06: SSEED/USEED do not affect other mseccfg fields          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_msecfg06_no_side_effects);
bool test_msecfg06_no_side_effects(void)
{
    TEST_BEGIN("MSECFG-06: SSEED/USEED do not affect other mseccfg fields");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t orig = mseccfg_read_zkr();

    /* Mask out SSEED/USEED bits to get "other fields" */
    uintptr_t other_mask = ~(MSECCFG_SSEED | MSECCFG_USEED);
    uintptr_t other_before = orig & other_mask;

    /* Toggle SSEED and USEED */
    mseccfg_set_bits(MSECCFG_SSEED | MSECCFG_USEED);
    uintptr_t after_set = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED | MSECCFG_USEED);
    uintptr_t after_clear = mseccfg_read_zkr();

    mseccfg_write_zkr(orig);

    /* Other fields should remain unchanged */
    TEST_ASSERT_EQ("other fields unchanged after set",
                   after_set & other_mask, other_before);
    TEST_ASSERT_EQ("other fields unchanged after clear",
                   after_clear & other_mask, other_before);

    TEST_END();
}
