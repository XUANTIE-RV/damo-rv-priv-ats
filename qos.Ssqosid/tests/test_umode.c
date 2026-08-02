/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_umode.c - Group 5: U-mode Access Control
 *
 * SRMCFG-25 ~ SRMCFG-26
 * Verifies that U-mode cannot access srmcfg (S-level CSR) and
 * S-mode can access it normally.
 */

/* ------------------------------------------------------------------ */
/* SRMCFG-25: U-mode access srmcfg -> illegal-instruction             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_25);
bool test_srmcfg_25(void) {
    TEST_BEGIN("SRMCFG-25: U-mode access srmcfg -> illegal-instruction");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

    /* If Smstateen is present, ensure bit 55 is set so the test
     * isolates the privilege-level check (not mstateen gating) */
    if (has_smstateen()) {
        mstateen0_set(MSTATEEN0_BIT55);
    }

    /* srmcfg is an S-level CSR (address 0x181). U-mode access to
     * S-level CSRs always triggers illegal-instruction. */
    SSQOSID_TEST_UMODE_BLOCKED("U-mode read srmcfg -> illegal-inst",
                               srmcfg_read());

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-26: S-mode normal access srmcfg                             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_26);
bool test_srmcfg_26(void) {
    TEST_BEGIN("SRMCFG-26: S-mode normal access srmcfg");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

    /* If Smstateen is present, ensure bit 55 is set */
    if (has_smstateen()) {
        mstateen0_set(MSTATEEN0_BIT55);
    }

    /* S-mode should access srmcfg normally */
    SSQOSID_TEST_SMODE_ALLOWED("S-mode read srmcfg", srmcfg_read());
    SSQOSID_TEST_SMODE_ALLOWED("S-mode write srmcfg", srmcfg_write(0));

    TEST_END();
}
