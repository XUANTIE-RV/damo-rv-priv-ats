/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 8.4: hstateen0 Functional Bit Controls
 *
 * Extracted from Ssstateen/tests/test_hfunc.c (Group 10).
 * ID mapping:
 *   SS-HSE0-01~03    -> HCROSS-SSSTA-21~23
 *   SS-HENVCFG-01~03 -> HCROSS-SSSTA-24~26
 *   SS-HCSRIND-01~03 -> HCROSS-SSSTA-27~29
 *   SS-HIMSIC-01~03  -> HCROSS-SSSTA-30~32
 *   SS-HAIA-01~03    -> HCROSS-SSSTA-33~35
 *   SS-HCTX-01~03    -> HCROSS-SSSTA-36~38
 *
 * Spec anchors:
 *   norm:hstateen0_SE0_op      — SE0 (bit 63) controls VS sstateen0
 *   norm:hstateen0_envcfg_op   — ENVCFG (bit 62) controls VS senvcfg
 *   norm:hstateen0_csrind_op   — CSRIND (bit 60) controls VS siselect/sireg
 *   norm:hstateen0_imsic_op    — IMSIC (bit 58) controls guest IMSIC
 *   norm:hstateen0_aia_op      — AIA (bit 59) controls Ssaia state
 *   norm:hstateen0_context_op  — CONTEXT (bit 57) controls VS scontext
 *
 * 18 tests: HCROSS-SSSTA-21 ~ HCROSS-SSSTA-38
 * =================================================================== */

/* ===== 8.4.1 SE0 bit (bit 63) — VS sstateen0 access ===== */

/* HCROSS-SSSTA-21: hstateen0.SE0=0 blocks VS read sstateen0 */
TEST_REGISTER(test_hcross_sssta_21);
bool test_hcross_sssta_21(void)
{
    TEST_BEGIN("HCROSS-SSSTA-21: hstateen0.SE0=0 blocks VS read sstateen0");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_set_bit63(0, true);
    uintptr_t saved_h = hstateen_read(0);

    hstateen_clear_bits(0, STATEEN_BIT63);

    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_sstateen0, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-22: hstateen0.SE0=0 blocks VS write sstateen0 */
TEST_REGISTER(test_hcross_sssta_22);
bool test_hcross_sssta_22(void)
{
    TEST_BEGIN("HCROSS-SSSTA-22: hstateen0.SE0=0 blocks VS write sstateen0");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_set_bit63(0, true);
    uintptr_t saved_h = hstateen_read(0);

    hstateen_clear_bits(0, STATEEN_BIT63);

    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_write_sstateen0, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-23: hstateen0.SE0=1 allows VS sstateen0 access */
TEST_REGISTER(test_hcross_sssta_23);
bool test_hcross_sssta_23(void)
{
    TEST_BEGIN("HCROSS-SSSTA-23: hstateen0.SE0=1 allows VS sstateen0 access");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_set_bit63(0, true);
    uintptr_t saved_h = hstateen_read(0);

    hstateen_set_bits(0, STATEEN_BIT63);

    VS_EXPECT_NO_TRAP(run_in_vs_mode(_vs_read_sstateen0, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* ===== 8.4.2 ENVCFG bit (bit 62) — VS senvcfg access ===== */

/* HCROSS-SSSTA-24: hstateen0.ENVCFG=0 blocks VS read senvcfg */
TEST_REGISTER(test_hcross_sssta_24);
bool test_hcross_sssta_24(void)
{
    TEST_BEGIN("HCROSS-SSSTA-24: hstateen0.ENVCFG=0 blocks VS read senvcfg");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_ENVCFG);

    uintptr_t saved_h = hstateen_read(0);

    /* Probe ENVCFG writability */
    hstateen_set_bits(0, STATEEN0_ENVCFG | STATEEN_BIT63);
    if (!(hstateen_read(0) & STATEEN0_ENVCFG))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.ENVCFG not writable");
    }

    /* Clear ENVCFG, keep bit63=1 */
    hstateen_clear_bits(0, STATEEN0_ENVCFG);

    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_senvcfg, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-25: hstateen0.ENVCFG=0 blocks VS write senvcfg */
TEST_REGISTER(test_hcross_sssta_25);
bool test_hcross_sssta_25(void)
{
    TEST_BEGIN("HCROSS-SSSTA-25: hstateen0.ENVCFG=0 blocks VS write senvcfg");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_ENVCFG);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63);

    if (!hstateen0_bit_writable(STATEEN0_ENVCFG))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.ENVCFG not writable");
    }

    hstateen_clear_bits(0, STATEEN0_ENVCFG);

    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_write_senvcfg, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-26: hstateen0.ENVCFG=1 allows VS senvcfg */
TEST_REGISTER(test_hcross_sssta_26);
bool test_hcross_sssta_26(void)
{
    TEST_BEGIN("HCROSS-SSSTA-26: hstateen0.ENVCFG=1 allows VS senvcfg");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_ENVCFG);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63 | STATEEN0_ENVCFG);

    if (!(hstateen_read(0) & STATEEN0_ENVCFG))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.ENVCFG not writable");
    }

    VS_EXPECT_NO_TRAP(run_in_vs_mode(_vs_read_senvcfg, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* ===== 8.4.3 CSRIND bit (bit 60) — VS siselect/sireg access ===== */

/* HCROSS-SSSTA-27: hstateen0.CSRIND=0 blocks VS read siselect */
TEST_REGISTER(test_hcross_sssta_27);
bool test_hcross_sssta_27(void)
{
    TEST_BEGIN("HCROSS-SSSTA-27: hstateen0.CSRIND=0 blocks VS read siselect");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_CSRIND);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63);

    if (!hstateen0_bit_writable(STATEEN0_CSRIND))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.CSRIND not writable (Sscsrind not implemented)");
    }

    hstateen_clear_bits(0, STATEEN0_CSRIND);

    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_siselect, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-28: hstateen0.CSRIND=0 blocks VS read sireg */
TEST_REGISTER(test_hcross_sssta_28);
bool test_hcross_sssta_28(void)
{
    TEST_BEGIN("HCROSS-SSSTA-28: hstateen0.CSRIND=0 blocks VS read sireg");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_CSRIND);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63);

    if (!hstateen0_bit_writable(STATEEN0_CSRIND))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.CSRIND not writable");
    }

    hstateen_clear_bits(0, STATEEN0_CSRIND);

    /* sireg is CSR 0x151 (maps to vsireg in VS-mode) */
    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_sireg, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-29: hstateen0.CSRIND=1 allows VS siselect/sireg */
TEST_REGISTER(test_hcross_sssta_29);
bool test_hcross_sssta_29(void)
{
    TEST_BEGIN("HCROSS-SSSTA-29: hstateen0.CSRIND=1 allows VS siselect/sireg");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_CSRIND);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63 | STATEEN0_CSRIND);

    if (!(hstateen_read(0) & STATEEN0_CSRIND))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.CSRIND not writable");
    }

    VS_EXPECT_NO_TRAP(run_in_vs_mode(_vs_read_siselect, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* ===== 8.4.4 IMSIC bit (bit 58) — Guest IMSIC control ===== */

/* HCROSS-SSSTA-30: hstateen0.IMSIC=0 blocks VS IMSIC access */
TEST_REGISTER(test_hcross_sssta_30);
bool test_hcross_sssta_30(void)
{
    TEST_BEGIN("HCROSS-SSSTA-30: hstateen0.IMSIC=0 blocks VS IMSIC access");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_IMSIC);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63);

    if (!hstateen0_bit_writable(STATEEN0_IMSIC))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.IMSIC not writable (Ssaia/IMSIC not implemented)");
    }

    hstateen_clear_bits(0, STATEEN0_IMSIC);

    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_stopei, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-31: hstateen0.IMSIC=1 allows VS IMSIC access */
TEST_REGISTER(test_hcross_sssta_31);
bool test_hcross_sssta_31(void)
{
    TEST_BEGIN("HCROSS-SSSTA-31: hstateen0.IMSIC=1 allows VS IMSIC access");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_IMSIC);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63 | STATEEN0_IMSIC);

    if (!(hstateen_read(0) & STATEEN0_IMSIC))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.IMSIC not writable");
    }

    /* VS-mode stopei access requires hstatus.VGEIN to be a valid
     * guest interrupt file number (non-zero). Per AIA spec: when
     * VGEIN is not an implemented guest external interrupt number,
     * VS-mode stopei access raises virtual-instruction exception.
     * This is independent of the stateen gate. */
    uintptr_t saved_hstatus;
    asm volatile("csrr %0, 0x600" : "=r"(saved_hstatus));

    /* Try setting VGEIN=1 */
    uintptr_t new_hs = (saved_hstatus & ~HSTATUS_VGEIN_MASK) |
                       (1UL << HSTATUS_VGEIN_SHIFT);
    asm volatile("csrw 0x600, %0" :: "r"(new_hs));
    uintptr_t rb_hs;
    asm volatile("csrr %0, 0x600" : "=r"(rb_hs));

    if (((rb_hs & HSTATUS_VGEIN_MASK) >> HSTATUS_VGEIN_SHIFT) == 0)
    {
        /* GEILEN=0, cannot configure valid VGEIN */
        asm volatile("csrw 0x600, %0" :: "r"(saved_hstatus));
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("No guest interrupt files (GEILEN=0), "
                   "cannot test stopei access");
    }

    /* With IMSIC=1 and valid VGEIN, VS-mode stopei access should
     * not trap due to stateen gating. */
    printf("  IMSIC=1, VGEIN=1; verifying VS-mode stopei not blocked\n");
    VS_EXPECT_NO_TRAP(run_in_vs_mode(_vs_read_stopei, 0));

    asm volatile("csrw 0x600, %0" :: "r"(saved_hstatus));
    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-32: hstateen0.IMSIC=0 equivalent to VGEIN=0 */
TEST_REGISTER(test_hcross_sssta_32);
bool test_hcross_sssta_32(void)
{
    TEST_BEGIN("HCROSS-SSSTA-32: hstateen0.IMSIC=0 equivalent to VGEIN=0");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_IMSIC);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63);

    if (!hstateen0_bit_writable(STATEEN0_IMSIC))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.IMSIC not writable");
    }

    /* IMSIC=0: VS-mode has no access to guest IMSIC,
     * effectively same as hstatus.VGEIN=0. */
    hstateen_clear_bits(0, STATEEN0_IMSIC);

    /* Verify the bit is cleared */
    TEST_ASSERT("hstateen0.IMSIC == 0",
                (hstateen_read(0) & STATEEN0_IMSIC) == 0);

    /* VS-mode stopei access should trigger virtual-instruction */
    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_stopei, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* ===== 8.4.5 AIA bit (bit 59) — Ssaia remaining state ===== */

/* HCROSS-SSSTA-33: hstateen0.AIA=0 blocks VS Ssaia state */
TEST_REGISTER(test_hcross_sssta_33);
bool test_hcross_sssta_33(void)
{
    TEST_BEGIN("HCROSS-SSSTA-33: hstateen0.AIA=0 blocks VS Ssaia state");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_AIA);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63);

    if (!hstateen0_bit_writable(STATEEN0_AIA))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.AIA not writable (Ssaia not implemented)");
    }

    hstateen_clear_bits(0, STATEEN0_AIA);
    TEST_ASSERT("hstateen0.AIA cleared to 0",
                (hstateen_read(0) & STATEEN0_AIA) == 0);

#if __riscv_xlen == 32
    /* RV32: AIA controls sieh/siph access. With AIA=0, VS-mode
     * access to sieh should trigger virtual-instruction. */
    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_sieh, 0));
#else
    /* RV64: AIA controls Ssaia state not covered by CSRIND/IMSIC.
     * On typical RV64, CSRIND+IMSIC cover all main Ssaia supervisor
     * state (siselect/sireg* + stopei). The AIA bit may control
     * implementation-specific state; verified at register level. */
    printf("  AIA=0: on RV64, remaining Ssaia state is platform-specific\n");
#endif

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-34: hstateen0.AIA=1 allows VS Ssaia state */
TEST_REGISTER(test_hcross_sssta_34);
bool test_hcross_sssta_34(void)
{
    TEST_BEGIN("HCROSS-SSSTA-34: hstateen0.AIA=1 allows VS Ssaia state");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_AIA);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63 | STATEEN0_AIA);

    if (!(hstateen_read(0) & STATEEN0_AIA))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.AIA not writable");
    }

    TEST_ASSERT("hstateen0.AIA set to 1",
                (hstateen_read(0) & STATEEN0_AIA) != 0);

#if __riscv_xlen == 32
    /* RV32: AIA=1 allows VS-mode access to sieh/siph */
    VS_EXPECT_NO_TRAP(run_in_vs_mode(_vs_read_sieh, 0));
#else
    /* RV64: AIA controls Ssaia state not covered by CSRIND/IMSIC.
     * On typical RV64, no additional supervisor state remains. */
    printf("  AIA=1: on RV64, remaining Ssaia state is platform-specific\n");
#endif

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-35: AIA=0 does not affect CSRIND/IMSIC gates */
TEST_REGISTER(test_hcross_sssta_35);
bool test_hcross_sssta_35(void)
{
    TEST_BEGIN("HCROSS-SSSTA-35: AIA=0 does not affect CSRIND/IMSIC gates");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_AIA |
                   STATEEN0_CSRIND | STATEEN0_IMSIC);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63 | STATEEN0_CSRIND | STATEEN0_IMSIC);

    /* Clear AIA only, keep CSRIND and IMSIC */
    hstateen_clear_bits(0, STATEEN0_AIA);

    bool csrind_set = (hstateen_read(0) & STATEEN0_CSRIND) != 0;
    bool imsic_set = (hstateen_read(0) & STATEEN0_IMSIC) != 0;

    if (!csrind_set && !imsic_set)
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("Neither CSRIND nor IMSIC writable");
    }

    /* AIA=0 should NOT affect CSRIND/IMSIC controlled state.
     * Verify at hstateen0 register level. */
    if (csrind_set)
    {
        TEST_ASSERT("CSRIND still 1 with AIA=0",
                    (hstateen_read(0) & STATEEN0_CSRIND) != 0);
    }
    if (imsic_set)
    {
        TEST_ASSERT("IMSIC still 1 with AIA=0",
                    (hstateen_read(0) & STATEEN0_IMSIC) != 0);
    }

    /* Verify VS-mode can still access CSRIND/IMSIC controlled
     * CSRs despite AIA=0, proving AIA does not gate them. */
    if (csrind_set)
    {
        VS_EXPECT_NO_TRAP(run_in_vs_mode(_vs_read_siselect, 0));
    }
    if (imsic_set)
    {
        /* stopei requires valid VGEIN per AIA spec. */
        uintptr_t saved_hstatus;
        asm volatile("csrr %0, 0x600" : "=r"(saved_hstatus));

        uintptr_t new_hs = (saved_hstatus & ~HSTATUS_VGEIN_MASK) |
                           (1UL << HSTATUS_VGEIN_SHIFT);
        asm volatile("csrw 0x600, %0" :: "r"(new_hs));
        uintptr_t rb_hs;
        asm volatile("csrr %0, 0x600" : "=r"(rb_hs));

        if (((rb_hs & HSTATUS_VGEIN_MASK) >> HSTATUS_VGEIN_SHIFT) != 0)
        {
            VS_EXPECT_NO_TRAP(run_in_vs_mode(_vs_read_stopei, 0));
        }
        else
        {
            printf("  stopei VS-mode access skipped: GEILEN=0\n");
        }

        asm volatile("csrw 0x600, %0" :: "r"(saved_hstatus));
    }

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* ===== 8.4.6 CONTEXT bit (bit 57) — VS scontext access ===== */

/* HCROSS-SSSTA-36: hstateen0.CONTEXT=0 blocks VS read scontext */
TEST_REGISTER(test_hcross_sssta_36);
bool test_hcross_sssta_36(void)
{
    TEST_BEGIN("HCROSS-SSSTA-36: hstateen0.CONTEXT=0 blocks VS read scontext");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_CONTEXT);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63);

    if (!hstateen0_bit_writable(STATEEN0_CONTEXT))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.CONTEXT not writable (Sdtrig not implemented)");
    }

    hstateen_clear_bits(0, STATEEN0_CONTEXT);

    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_scontext, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-37: hstateen0.CONTEXT=0 blocks VS write scontext */
TEST_REGISTER(test_hcross_sssta_37);
bool test_hcross_sssta_37(void)
{
    TEST_BEGIN("HCROSS-SSSTA-37: hstateen0.CONTEXT=0 blocks VS write scontext");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_CONTEXT);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63);

    if (!hstateen0_bit_writable(STATEEN0_CONTEXT))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.CONTEXT not writable");
    }

    hstateen_clear_bits(0, STATEEN0_CONTEXT);

    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_write_scontext, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}

/* HCROSS-SSSTA-38: hstateen0.CONTEXT=1 allows VS scontext */
TEST_REGISTER(test_hcross_sssta_38);
bool test_hcross_sssta_38(void)
{
    TEST_BEGIN("HCROSS-SSSTA-38: hstateen0.CONTEXT=1 allows VS scontext");
    REQUIRE_H_EXT();

    uintptr_t saved_m = mstateen_read(0);
    mstateen_write(0, saved_m | STATEEN0_SE0 | STATEEN0_CONTEXT);

    uintptr_t saved_h = hstateen_read(0);
    hstateen_set_bits(0, STATEEN_BIT63 | STATEEN0_CONTEXT);

    if (!(hstateen_read(0) & STATEEN0_CONTEXT))
    {
        hstateen_write(0, saved_h);
        mstateen_write(0, saved_m);
        TEST_SKIP("hstateen0.CONTEXT not writable");
    }

    VS_EXPECT_NO_TRAP(run_in_vs_mode(_vs_read_scontext, 0));

    hstateen_write(0, saved_h);
    mstateen_write(0, saved_m);
    HYP_TEST_END();
}
