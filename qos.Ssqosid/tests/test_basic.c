/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_basic.c - Group 1: srmcfg Register Basic Read/Write and Field Behavior
 *
 * SRMCFG-01 ~ SRMCFG-07
 * Verifies srmcfg CSR accessibility, WARL field behavior, and WPRI fields.
 */

/* ------------------------------------------------------------------ */
/* SRMCFG-01: M-mode read/write srmcfg                                */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_01);
bool test_srmcfg_01(void) {
    TEST_BEGIN("SRMCFG-01: M-mode read/write srmcfg");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

    /* Write a test pattern to RCID and MCID fields */
    uintptr_t test_val = (0x123UL << SRMCFG_RCID_SHIFT) |
                         (0x456UL << SRMCFG_MCID_SHIFT);
    M_EXPECT_NO_TRAP(srmcfg_write(test_val));

    uintptr_t readback = srmcfg_read();

    /* WARL fields are 12-bit wide; any 12-bit value is legal.
     * A WARL field must hold the written value (or an implementation-
     * defined legal value) and read back consistently. */
    TEST_ASSERT_EQ("RCID readback matches written value",
                   srmcfg_get_rcid(readback), 0x123UL);
    TEST_ASSERT_EQ("MCID readback matches written value",
                   srmcfg_get_mcid(readback), 0x456UL);

    /* Verify write-0 works (suggested reset value) */
    srmcfg_write(0);
    uintptr_t v0 = srmcfg_read();
    TEST_ASSERT_EQ("RCID=0 after write 0", srmcfg_get_rcid(v0), 0);
    TEST_ASSERT_EQ("MCID=0 after write 0", srmcfg_get_mcid(v0), 0);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-02: HS-mode read/write srmcfg                               */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_02);
bool test_srmcfg_02(void) {
    TEST_BEGIN("SRMCFG-02: HS-mode read/write srmcfg");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

    /* If Smstateen is present, ensure bit 55 is set */
    if (has_smstateen()) {
        mstateen0_set(MSTATEEN0_BIT55);
    }

    SSQOSID_TEST_SMODE_ALLOWED("HS-mode read srmcfg", srmcfg_read());
    SSQOSID_TEST_SMODE_ALLOWED("HS-mode write srmcfg", srmcfg_write(0));

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-03: RCID field WARL behavior                                */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_03);
bool test_srmcfg_03(void) {
    TEST_BEGIN("SRMCFG-03: RCID field WARL behavior");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

    /* Write RCID = 0xFFF (all ones) - must hold */
    srmcfg_write(0xFFFUL << SRMCFG_RCID_SHIFT);
    uintptr_t v1 = srmcfg_read();
    TEST_ASSERT_EQ("RCID=0xFFF readback matches",
                   srmcfg_get_rcid(v1), 0xFFFUL);

    /* Write RCID = 0 - must hold */
    srmcfg_write(0);
    uintptr_t v2 = srmcfg_read();
    TEST_ASSERT_EQ("RCID=0 readback is 0", srmcfg_get_rcid(v2), 0);

    /* Walking-1: each individual bit must be writable and readable */
    for (int i = 0; i < 12; i++) {
        uintptr_t pattern = (1UL << i) << SRMCFG_RCID_SHIFT;
        srmcfg_write(pattern);
        uintptr_t v = srmcfg_read();
        TEST_ASSERT_EQ("RCID walking-1 bit held",
                       srmcfg_get_rcid(v), (1UL << i));
    }

    srmcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-04: MCID field WARL behavior                                */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_04);
bool test_srmcfg_04(void) {
    TEST_BEGIN("SRMCFG-04: MCID field WARL behavior");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

    /* Write MCID = 0xFFF (all ones) - must hold */
    srmcfg_write(0xFFFUL << SRMCFG_MCID_SHIFT);
    uintptr_t v1 = srmcfg_read();
    TEST_ASSERT_EQ("MCID=0xFFF readback matches",
                   srmcfg_get_mcid(v1), 0xFFFUL);

    /* Write MCID = 0 - must hold */
    srmcfg_write(0);
    uintptr_t v2 = srmcfg_read();
    TEST_ASSERT_EQ("MCID=0 readback is 0", srmcfg_get_mcid(v2), 0);

    /* Walking-1: each individual bit must be writable and readable */
    for (int i = 0; i < 12; i++) {
        uintptr_t pattern = (1UL << i) << SRMCFG_MCID_SHIFT;
        srmcfg_write(pattern);
        uintptr_t v = srmcfg_read();
        TEST_ASSERT_EQ("MCID walking-1 bit held",
                       srmcfg_get_mcid(v), (1UL << i));
    }

    srmcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-05: WPRI fields read as zero (SXLEN=64)                     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_05);
bool test_srmcfg_05(void) {
    TEST_BEGIN("SRMCFG-05: WPRI fields read as zero (SXLEN=64)");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

#if __riscv_xlen == 64
    /* Write all ones, WPRI fields should read back as zero */
    srmcfg_write(~0UL);
    uintptr_t v = srmcfg_read();

    TEST_ASSERT_EQ("WPRI bits[15:12] read as zero",
                   v & SRMCFG_WPRI_LO_MASK, 0);
    TEST_ASSERT_EQ("WPRI bits[63:28] read as zero",
                   v & SRMCFG_WPRI_HI_MASK, 0);
#else
    TEST_SKIP("SXLEN=64 specific test");
#endif

    srmcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-06: WPRI fields read as zero (SXLEN=32)                     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_06);
bool test_srmcfg_06(void) {
    TEST_BEGIN("SRMCFG-06: WPRI fields read as zero (SXLEN=32)");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

#if __riscv_xlen == 32
    /* Write all ones, WPRI fields should read back as zero */
    srmcfg_write(~0UL);
    uintptr_t v = srmcfg_read();

    TEST_ASSERT_EQ("WPRI bits[15:12] read as zero",
                   v & SRMCFG_WPRI_LO_MASK, 0);
    TEST_ASSERT_EQ("WPRI bits[31:28] read as zero",
                   v & SRMCFG_WPRI_HI_MASK, 0);
#else
    TEST_SKIP("SXLEN=32 specific test");
#endif

    srmcfg_write(0);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SRMCFG-07: RCID and MCID field independence                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_07);
bool test_srmcfg_07(void) {
    TEST_BEGIN("SRMCFG-07: RCID and MCID field independence");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");

    /* Step 1: Write both fields with known values */
    uintptr_t rcid_a = 0xABC;
    uintptr_t mcid_a = 0xDEF;
    srmcfg_write((rcid_a << SRMCFG_RCID_SHIFT) |
                 (mcid_a << SRMCFG_MCID_SHIFT));
    uintptr_t v1 = srmcfg_read();
    TEST_ASSERT_EQ("initial RCID held", srmcfg_get_rcid(v1), rcid_a);
    TEST_ASSERT_EQ("initial MCID held", srmcfg_get_mcid(v1), mcid_a);

    /* Step 2: Read-modify-write only RCID, preserve MCID */
    uintptr_t rcid_b = 0x111;
    uintptr_t cur = srmcfg_read();
    uintptr_t new_val = (rcid_b << SRMCFG_RCID_SHIFT) |
                        (cur & (SRMCFG_MCID_MASK << SRMCFG_MCID_SHIFT));
    srmcfg_write(new_val);
    uintptr_t v2 = srmcfg_read();
    TEST_ASSERT_EQ("RCID updated to new value",
                   srmcfg_get_rcid(v2), rcid_b);
    TEST_ASSERT_EQ("MCID preserved after RCID change",
                   srmcfg_get_mcid(v2), srmcfg_get_mcid(v1));

    /* Step 3: Read-modify-write only MCID, preserve RCID */
    uintptr_t mcid_b = 0x222;
    cur = srmcfg_read();
    new_val = (mcid_b << SRMCFG_MCID_SHIFT) |
              (cur & (SRMCFG_RCID_MASK << SRMCFG_RCID_SHIFT));
    srmcfg_write(new_val);
    uintptr_t v3 = srmcfg_read();
    TEST_ASSERT_EQ("MCID updated to new value",
                   srmcfg_get_mcid(v3), mcid_b);
    TEST_ASSERT_EQ("RCID preserved after MCID change",
                   srmcfg_get_rcid(v3), rcid_b);
    srmcfg_write(0);
    TEST_END();
}
