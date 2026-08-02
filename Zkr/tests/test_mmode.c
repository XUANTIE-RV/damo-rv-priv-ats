/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mmode.c - Group 4: M-mode Access Control
 *
 * MACC-01 ~ MACC-03
 * Verifies that seed CSR is unconditionally available in M-mode.
 */

/* ------------------------------------------------------------------ */
/* MACC-01: M-mode csrrw always available (SSEED=0, USEED=0)          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_macc01_always_avail_00);
bool test_macc01_always_avail_00(void)
{
    TEST_BEGIN("MACC-01: M-mode csrrw available (SSEED=0, USEED=0)");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* Clear SSEED and USEED */
    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED | MSECCFG_USEED);

    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read());
    printf("  [INFO] seed = 0x%lx (SSEED=0, USEED=0)\n", (unsigned long)val);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MACC-02: M-mode csrrw always available (SSEED=1, USEED=1)          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_macc02_always_avail_11);
bool test_macc02_always_avail_11(void)
{
    TEST_BEGIN("MACC-02: M-mode csrrw available (SSEED=1, USEED=1)");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* Set SSEED and USEED */
    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED | MSECCFG_USEED);

    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read());
    printf("  [INFO] seed = 0x%lx (SSEED=1, USEED=1)\n", (unsigned long)val);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* MACC-03: M-mode read-only access still triggers exception          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_macc03_ro_still_traps);
bool test_macc03_ro_still_traps(void)
{
    TEST_BEGIN("MACC-03: M-mode read-only access still triggers exception");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* Even in M-mode, csrrs rd, seed, x0 must trap */
    M_EXPECT_TRAP(CAUSE_ILLEGAL_INST, {
        (void)seed_read_ro_csrrs();
    });

    TEST_END();
}
