/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 8.3: hstateen VS-mode sstateen ROZ propagation
 *
 * Extracted from Ssstateen/tests/test_vsprop.c (Group 9).
 * ID mapping: SS-VSPROP-01~05 -> HCROSS-SSSTA-16~20
 *
 * Spec anchor:
 *   norm:sstateen_vsmode_access_roz
 *     — hstateen bit=0 => VS-mode sees sstateen bit as ROZ
 *
 * 5 tests: HCROSS-SSSTA-16 ~ HCROSS-SSSTA-20
 * =================================================================== */

/* VS-mode helper: write then read sstateen0 */
static uintptr_t _vs_write_read_sstateen0(uintptr_t write_val)
{
    asm volatile("csrw 0x10C, %0" :: "r"(write_val));
    uintptr_t rb;
    asm volatile("csrr %0, 0x10C" : "=r"(rb));
    return rb;
}

/* ---- HCROSS-SSSTA-16: hstateen0.C=0 propagates to VS sstateen0.C ---- */

TEST_REGISTER(test_hcross_sssta_16);
bool test_hcross_sssta_16(void)
{
    TEST_BEGIN("HCROSS-SSSTA-16: hstateen0.C=0 => VS sstateen0.C is ROZ");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_SE0 | STATEEN0_C);

    uintptr_t saved_hstateen0 = hstateen_read(0);
    /* Set bit63=1 (allow VS access to sstateen0), but C=0 */
    hstateen_write(0, (saved_hstateen0 | STATEEN_BIT63) & ~STATEEN0_C);

    /* Probe: is C bit meaningful? */
    if (!mstateen0_bit_writable(STATEEN0_C))
    {
        hstateen_write(0, saved_hstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("C bit not writable in mstateen0");
    }

    /* VS-mode: write sstateen0.C=1, read back => should be 0 */
    trap_expect_begin();
    uintptr_t vs_rb = run_in_vs_mode(_vs_write_read_sstateen0, STATEEN0_C);
    bool no_trap = !trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VS-mode access sstateen0 OK (bit63=1)", no_trap);
    if (no_trap)
    {
        TEST_ASSERT("sstateen0.C is ROZ in VS (hstateen0.C=0)",
                    (vs_rb & STATEEN0_C) == 0);
    }

    hstateen_write(0, saved_hstateen0);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-17: hstateen0.C=1 releases VS propagation ---- */

TEST_REGISTER(test_hcross_sssta_17);
bool test_hcross_sssta_17(void)
{
    TEST_BEGIN("HCROSS-SSSTA-17: hstateen0.C=1 => VS sstateen0.C writable");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_SE0 | STATEEN0_C);

    uintptr_t saved_hstateen0 = hstateen_read(0);
    hstateen_write(0, saved_hstateen0 | STATEEN_BIT63 | STATEEN0_C);

    if (!mstateen0_bit_writable(STATEEN0_C))
    {
        hstateen_write(0, saved_hstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("C bit not writable in mstateen0");
    }

    /* Verify hstateen0.C actually took */
    if (!(hstateen_read(0) & STATEEN0_C))
    {
        hstateen_write(0, saved_hstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("hstateen0.C not writable");
    }

    /* VS-mode: write sstateen0.C=1, read back => should be 1 */
    trap_expect_begin();
    uintptr_t vs_rb = run_in_vs_mode(_vs_write_read_sstateen0, STATEEN0_C);
    bool no_trap = !trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VS-mode access sstateen0 OK", no_trap);
    if (no_trap)
    {
        TEST_ASSERT("sstateen0.C writable in VS (hstateen0.C=1)",
                    (vs_rb & STATEEN0_C) != 0);
    }

    hstateen_write(0, saved_hstateen0);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-18: hstateen0.JVT=0 propagates to VS sstateen0.JVT ---- */

TEST_REGISTER(test_hcross_sssta_18);
bool test_hcross_sssta_18(void)
{
    TEST_BEGIN("HCROSS-SSSTA-18: hstateen0.JVT=0 => VS sstateen0.JVT ROZ");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_SE0 | STATEEN0_JVT);

    uintptr_t saved_hstateen0 = hstateen_read(0);
    hstateen_write(0, (saved_hstateen0 | STATEEN_BIT63) & ~STATEEN0_JVT);

    if (!mstateen0_bit_writable(STATEEN0_JVT))
    {
        hstateen_write(0, saved_hstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("JVT bit not writable in mstateen0");
    }

    trap_expect_begin();
    uintptr_t vs_rb = run_in_vs_mode(_vs_write_read_sstateen0, STATEEN0_JVT);
    bool no_trap = !trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VS-mode access sstateen0 OK", no_trap);
    if (no_trap)
    {
        TEST_ASSERT("sstateen0.JVT is ROZ in VS (hstateen0.JVT=0)",
                    (vs_rb & STATEEN0_JVT) == 0);
    }

    hstateen_write(0, saved_hstateen0);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-19: hstateen0 multiple bits propagate ---- */

TEST_REGISTER(test_hcross_sssta_19);
bool test_hcross_sssta_19(void)
{
    TEST_BEGIN("HCROSS-SSSTA-19: hstateen0 multiple bits propagate to VS");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, ~0UL);  /* Open all mstateen0 bits */

    uintptr_t saved_hstateen0 = hstateen_read(0);
    /* Set bit63=1 but clear C, FCSR, JVT */
    uintptr_t func_bits = STATEEN0_C | STATEEN0_FCSR | STATEEN0_JVT;
    hstateen_write(0, (hstateen_read(0) | STATEEN_BIT63) & ~func_bits);

    /* VS-mode: write all func bits, read back */
    trap_expect_begin();
    uintptr_t vs_rb = run_in_vs_mode(_vs_write_read_sstateen0, func_bits);
    bool no_trap = !trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VS-mode access sstateen0 OK", no_trap);
    if (no_trap)
    {
        /* Each cleared hstateen0 bit should be ROZ in VS sstateen0 */
        uintptr_t leaked = vs_rb & func_bits;
        printf("  VS sstateen0 readback: 0x%lx, func_bits: 0x%lx, "
               "leaked: 0x%lx\n",
               (unsigned long)vs_rb, (unsigned long)func_bits,
               (unsigned long)leaked);
        TEST_ASSERT("cleared hstateen0 bits are ROZ in VS sstateen0",
                    leaked == 0);
    }

    hstateen_write(0, saved_hstateen0);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-20: hstateen0 bit 0->1 releases propagation ---- */

TEST_REGISTER(test_hcross_sssta_20);
bool test_hcross_sssta_20(void)
{
    TEST_BEGIN("HCROSS-SSSTA-20: hstateen0.C toggle 0->1 releases VS ROZ");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_write(0, saved_mstateen0 | STATEEN0_SE0 | STATEEN0_C);

    if (!mstateen0_bit_writable(STATEEN0_C))
    {
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("C bit not writable");
    }

    uintptr_t saved_hstateen0 = hstateen_read(0);

    /* Phase 1: hstateen0.C=0 => VS sees ROZ */
    hstateen_write(0, (saved_hstateen0 | STATEEN_BIT63) & ~STATEEN0_C);

    trap_expect_begin();
    uintptr_t vs_rb1 = run_in_vs_mode(_vs_write_read_sstateen0, STATEEN0_C);
    bool ok1 = !trap_was_triggered();
    trap_expect_end();

    if (ok1)
    {
        TEST_ASSERT("Phase 1: C is ROZ in VS",
                    (vs_rb1 & STATEEN0_C) == 0);
    }

    /* Phase 2: hstateen0.C=1 => VS can write */
    hstateen_set_bits(0, STATEEN0_C);

    if (!(hstateen_read(0) & STATEEN0_C))
    {
        hstateen_write(0, saved_hstateen0);
        mstateen_write(0, saved_mstateen0);
        TEST_SKIP("hstateen0.C not writable");
    }

    trap_expect_begin();
    uintptr_t vs_rb2 = run_in_vs_mode(_vs_write_read_sstateen0, STATEEN0_C);
    bool ok2 = !trap_was_triggered();
    trap_expect_end();

    if (ok2)
    {
        TEST_ASSERT("Phase 2: C is writable in VS",
                    (vs_rb2 & STATEEN0_C) != 0);
    }

    hstateen_write(0, saved_hstateen0);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}
