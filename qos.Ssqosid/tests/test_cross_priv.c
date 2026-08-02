/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_cross_priv.c - Group 2: srmcfg Cross-Privilege Mode Applicability
 *
 * SRMCFG-08 ~ SRMCFG-10
 * Verifies that srmcfg configuration persists across privilege mode
 * transitions (the RCID/MCID apply to all privilege modes by default).
 */

/* Trampoline: read srmcfg in S-mode, return the value */
static uintptr_t _smode_read_srmcfg(uintptr_t arg) {
    (void)arg;
    return srmcfg_read();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-08: HS-mode configuration persists after S-mode execution   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_08);
bool test_srmcfg_08(void) {
    TEST_BEGIN("SRMCFG-08: HS-mode config persists after S-mode");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

    /* If Smstateen is present, ensure bit 55 is set */
    if (has_smstateen()) {
        mstateen0_set(MSTATEEN0_BIT55);
    }

    /* Write a known value in M-mode (acting as HS-mode context) */
    uintptr_t test_val = (0x042UL << SRMCFG_RCID_SHIFT) |
                         (0x084UL << SRMCFG_MCID_SHIFT);
    srmcfg_write(test_val);

    /* Read from S-mode via trampoline, get actual value */
    uintptr_t s_read = run_in_priv(PRIV_S, _smode_read_srmcfg, 0);
    TEST_ASSERT_EQ("S-mode reads same RCID",
                   srmcfg_get_rcid(s_read), srmcfg_get_rcid(test_val));
    TEST_ASSERT_EQ("S-mode reads same MCID",
                   srmcfg_get_mcid(s_read), srmcfg_get_mcid(test_val));

    srmcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-09: M-mode configuration visible from HS-mode               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_09);
bool test_srmcfg_09(void) {
    TEST_BEGIN("SRMCFG-09: M-mode config visible from HS-mode");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

    /* If Smstateen is present, ensure bit 55 is set */
    if (has_smstateen()) {
        mstateen0_set(MSTATEEN0_BIT55);
    }

    /* Write a known value in M-mode */
    uintptr_t test_val = (0x155UL << SRMCFG_RCID_SHIFT) |
                         (0x2AAUL << SRMCFG_MCID_SHIFT);
    srmcfg_write(test_val);

    /* Read from S-mode (HS-mode) via trampoline, get actual value */
    uintptr_t s_read = run_in_priv(PRIV_S, _smode_read_srmcfg, 0);
    TEST_ASSERT_EQ("HS-mode reads same RCID as M-mode wrote",
                   srmcfg_get_rcid(s_read), srmcfg_get_rcid(test_val));
    TEST_ASSERT_EQ("HS-mode reads same MCID as M-mode wrote",
                   srmcfg_get_mcid(s_read), srmcfg_get_mcid(test_val));

    srmcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-10: Context switch scenario                                  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_10);
bool test_srmcfg_10(void) {
    TEST_BEGIN("SRMCFG-10: Context switch scenario");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

    /* Simulate context A: write value A */
    uintptr_t val_a = (0x100UL << SRMCFG_RCID_SHIFT) |
                      (0x200UL << SRMCFG_MCID_SHIFT);
    srmcfg_write(val_a);
    uintptr_t read_a = srmcfg_read();

    /* Simulate context switch: save A, write B */
    uintptr_t val_b = (0x300UL << SRMCFG_RCID_SHIFT) |
                      (0x400UL << SRMCFG_MCID_SHIFT);
    srmcfg_write(val_b);
    uintptr_t read_b = srmcfg_read();

    /* Verify B is active */
    TEST_ASSERT_EQ("Context B RCID active",
                   srmcfg_get_rcid(read_b), srmcfg_get_rcid(val_b));
    TEST_ASSERT_EQ("Context B MCID active",
                   srmcfg_get_mcid(read_b), srmcfg_get_mcid(val_b));

    /* Simulate context restore: write back A */
    srmcfg_write(val_a);
    uintptr_t read_restored = srmcfg_read();

    TEST_ASSERT_EQ("Restored context A RCID",
                   srmcfg_get_rcid(read_restored), srmcfg_get_rcid(val_a));
    TEST_ASSERT_EQ("Restored context A MCID",
                   srmcfg_get_mcid(read_restored), srmcfg_get_mcid(val_a));

    (void)read_a;
    srmcfg_write(0);
    TEST_END();
}
