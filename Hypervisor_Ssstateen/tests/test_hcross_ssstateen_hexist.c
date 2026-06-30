/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 8.1: hstateen CSR Existence and Accessibility
 *
 * Extracted from Ssstateen/tests/test_hexist.c (Group 7).
 * ID mapping: SS-HEXIST-01~06 -> HCROSS-SSSTA-01~06
 *
 * Spec anchors:
 *   norm:hstateen_rv64_csrs           — hstateen0-3 exist with H ext
 *   norm:stateen_rv32_upper_bits_csrs — RV32 hstateen0h-3h
 *   norm:hstateen_encoding            — encoding same as mstateen
 *
 * 6 tests: HCROSS-SSSTA-01 ~ HCROSS-SSSTA-06
 * =================================================================== */

/* ---- HCROSS-SSSTA-01: hstateen0 readable in HS-mode ---- */

TEST_REGISTER(test_hcross_sssta_01);
bool test_hcross_sssta_01(void)
{
    TEST_BEGIN("HCROSS-SSSTA-01: hstateen0 readable in HS-mode");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bit63(0, true);

    uintptr_t val = hstateen_read(0);
    (void)val;
    TEST_ASSERT("hstateen0 readable without trap", 1);

    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-02: hstateen0 writable in HS-mode ---- */

TEST_REGISTER(test_hcross_sssta_02);
bool test_hcross_sssta_02(void)
{
    TEST_BEGIN("HCROSS-SSSTA-02: hstateen0 writable in HS-mode");
    REQUIRE_H_EXT();

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bit63(0, true);

    uintptr_t saved = hstateen_read(0);
    hstateen_write(0, saved);
    TEST_ASSERT("hstateen0 writable without trap", 1);

    hstateen_write(0, saved);
    mstateen_write(0, saved_mstateen0);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-03: hstateen1 readable/writable in HS-mode ---- */

TEST_REGISTER(test_hcross_sssta_03);
bool test_hcross_sssta_03(void)
{
    TEST_BEGIN("HCROSS-SSSTA-03: hstateen1 readable/writable in HS-mode");
    REQUIRE_H_EXT();

    uintptr_t saved = mstateen_read(1);
    mstateen_set_bit63(1, true);

    if (!(mstateen_read(1) & STATEEN_BIT63))
    {
        mstateen_write(1, saved);
        TEST_SKIP("mstateen1 bit 63 not writable (hstateen1 not accessible)");
    }

    uintptr_t val = hstateen_read(1);
    hstateen_write(1, val);
    TEST_ASSERT("hstateen1 readable/writable", 1);

    mstateen_write(1, saved);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-04: hstateen2 readable/writable in HS-mode ---- */

TEST_REGISTER(test_hcross_sssta_04);
bool test_hcross_sssta_04(void)
{
    TEST_BEGIN("HCROSS-SSSTA-04: hstateen2 readable/writable in HS-mode");
    REQUIRE_H_EXT();

    uintptr_t saved = mstateen_read(2);
    mstateen_set_bit63(2, true);

    if (!(mstateen_read(2) & STATEEN_BIT63))
    {
        mstateen_write(2, saved);
        TEST_SKIP("mstateen2 bit 63 not writable (hstateen2 not accessible)");
    }

    uintptr_t val = hstateen_read(2);
    hstateen_write(2, val);
    TEST_ASSERT("hstateen2 readable/writable", 1);

    mstateen_write(2, saved);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-05: hstateen3 readable/writable in HS-mode ---- */

TEST_REGISTER(test_hcross_sssta_05);
bool test_hcross_sssta_05(void)
{
    TEST_BEGIN("HCROSS-SSSTA-05: hstateen3 readable/writable in HS-mode");
    REQUIRE_H_EXT();

    uintptr_t saved = mstateen_read(3);
    mstateen_set_bit63(3, true);

    if (!(mstateen_read(3) & STATEEN_BIT63))
    {
        mstateen_write(3, saved);
        TEST_SKIP("mstateen3 bit 63 not writable (hstateen3 not accessible)");
    }

    uintptr_t val = hstateen_read(3);
    hstateen_write(3, val);
    TEST_ASSERT("hstateen3 readable/writable", 1);

    mstateen_write(3, saved);
    HYP_TEST_END();
}

/* ---- HCROSS-SSSTA-06: hstateen0h readable/writable (RV32 only) ---- */

TEST_REGISTER(test_hcross_sssta_06);
bool test_hcross_sssta_06(void)
{
    TEST_BEGIN("HCROSS-SSSTA-06: hstateen0h readable/writable (RV32)");
    REQUIRE_H_EXT();

#if __riscv_xlen == 32
    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bit63(0, true);

    /* hstateen0h is CSR 0x61C */
    uintptr_t val;
    M_EXPECT_NO_TRAP({
        asm volatile("csrr %0, 0x61C" : "=r"(val) :: "memory");
    });
    M_EXPECT_NO_TRAP({
        asm volatile("csrw 0x61C, %0" :: "r"(val) : "memory");
    });

    mstateen_write(0, saved_mstateen0);
#else
    TEST_SKIP("RV32-only test");
#endif

    HYP_TEST_END();
}
