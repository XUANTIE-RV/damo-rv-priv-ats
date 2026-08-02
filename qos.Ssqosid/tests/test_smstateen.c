/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_smstateen.c - Group 3: Smstateen Gating Access Control
 *
 * SRMCFG-11 ~ SRMCFG-18
 * Verifies mstateen0 bit 55 gating of srmcfg access.
 * These tests only run when Smstateen is implemented.
 */

/* ------------------------------------------------------------------ */
/* SRMCFG-11: mstateen0[55]=0 HS-mode read srmcfg -> illegal-inst    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_11);
bool test_srmcfg_11(void) {
    TEST_BEGIN("SRMCFG-11: mstateen0[55]=0 HS-mode read -> illegal-inst");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!has_smstateen()) TEST_SKIP("Smstateen not implemented");

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_BIT55);

    SSQOSID_TEST_SMODE_BLOCKED("HS-mode read srmcfg blocked",
                               srmcfg_read());

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-12: mstateen0[55]=0 HS-mode write srmcfg -> illegal-inst   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_12);
bool test_srmcfg_12(void) {
    TEST_BEGIN("SRMCFG-12: mstateen0[55]=0 HS-mode write -> illegal-inst");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!has_smstateen()) TEST_SKIP("Smstateen not implemented");

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_BIT55);

    SSQOSID_TEST_SMODE_BLOCKED("HS-mode write srmcfg blocked",
                               srmcfg_write(0));

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-13: mstateen0[55]=0 S-mode read srmcfg -> illegal-inst     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_13);
bool test_srmcfg_13(void) {
    TEST_BEGIN("SRMCFG-13: mstateen0[55]=0 S-mode read -> illegal-inst");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!has_smstateen()) TEST_SKIP("Smstateen not implemented");

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_BIT55);

    /* S-mode (non-hypervisor) access should also be blocked */
    SSQOSID_TEST_SMODE_BLOCKED("S-mode read srmcfg blocked",
                               srmcfg_read());

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-14: mstateen0[55]=0 U-mode read srmcfg -> illegal-inst     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_14);
bool test_srmcfg_14(void) {
    TEST_BEGIN("SRMCFG-14: mstateen0[55]=0 U-mode read -> illegal-inst");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!has_smstateen()) TEST_SKIP("Smstateen not implemented");

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_BIT55);

    SSQOSID_TEST_UMODE_BLOCKED("U-mode read srmcfg blocked",
                               srmcfg_read());

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-15: mstateen0[55]=1 HS-mode normal access                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_15);
bool test_srmcfg_15(void) {
    TEST_BEGIN("SRMCFG-15: mstateen0[55]=1 HS-mode normal access");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!has_smstateen()) TEST_SKIP("Smstateen not implemented");

    uintptr_t orig = mstateen0_read();
    mstateen0_set(MSTATEEN0_BIT55);

    SSQOSID_TEST_SMODE_ALLOWED("HS-mode read srmcfg allowed",
                               srmcfg_read());
    SSQOSID_TEST_SMODE_ALLOWED("HS-mode write srmcfg allowed",
                               srmcfg_write(0));

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-16: mstateen0[55]=1 S-mode normal access                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_16);
bool test_srmcfg_16(void) {
    TEST_BEGIN("SRMCFG-16: mstateen0[55]=1 S-mode normal access");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!has_smstateen()) TEST_SKIP("Smstateen not implemented");

    uintptr_t orig = mstateen0_read();
    mstateen0_set(MSTATEEN0_BIT55);

    /* In non-hypervisor context, S-mode should access srmcfg normally
     * when mstateen0[55]=1 */
    SSQOSID_TEST_SMODE_ALLOWED("S-mode read srmcfg allowed",
                               srmcfg_read());
    SSQOSID_TEST_SMODE_ALLOWED("S-mode write srmcfg allowed",
                               srmcfg_write(0));

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-17: M-mode not restricted by mstateen0[55]                  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_17);
bool test_srmcfg_17(void) {
    TEST_BEGIN("SRMCFG-17: M-mode not restricted by mstateen0[55]");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!has_smstateen()) TEST_SKIP("Smstateen not implemented");

    uintptr_t orig = mstateen0_read();
    mstateen0_clear(MSTATEEN0_BIT55);

    /* M-mode should still access srmcfg regardless of mstateen0[55] */
    M_EXPECT_NO_TRAP({
        uintptr_t v = srmcfg_read();
        (void)v;
    });

    M_EXPECT_NO_TRAP(srmcfg_write(0));

    mstateen0_write(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-18: mstateen0[55] writability verification                  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_18);
bool test_srmcfg_18(void) {
    TEST_BEGIN("SRMCFG-18: mstateen0[55] writability");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!has_smstateen()) TEST_SKIP("Smstateen not implemented");

    uintptr_t orig = mstateen0_read();

    /* Write 1, read back */
    mstateen0_set(MSTATEEN0_BIT55);
    uintptr_t v1 = mstateen0_read();
    TEST_ASSERT("mstateen0[55] can be set to 1",
                (v1 & MSTATEEN0_BIT55) != 0);

    /* Write 0, read back */
    mstateen0_clear(MSTATEEN0_BIT55);
    uintptr_t v0 = mstateen0_read();
    TEST_ASSERT("mstateen0[55] can be cleared to 0",
                (v0 & MSTATEEN0_BIT55) == 0);

    mstateen0_write(orig);
    TEST_END();
}
