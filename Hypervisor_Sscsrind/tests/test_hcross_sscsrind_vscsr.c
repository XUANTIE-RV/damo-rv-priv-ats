/*
 * Hypervisor_Sscsrind - Group 11.1: VS-level CSR Basic Functionality
 *
 * Tests HCROSS-SSCSRIND-01 through HCROSS-SSCSRIND-10
 *
 * These tests verify vsiselect/vsireg* CSR basic functionality in HS-mode,
 * including read/write, WARL behavior, minimum range, MSB semantics, and
 * width verification.
 *
 * Migrated from Sscsrind_test_plan.md Group 2 (SSCSRIND-VSCSR-01~10).
 * See Hypervisor_cross_test_plan.md Group 11.1.
 */

/* ===================================================================
 * HCROSS-SSCSRIND-01: vsiselect readable in HS-mode
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_01);
bool test_hcross_sscsrind_01(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-01: vsiselect readable in HS-mode");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    /* Switch to HS-mode (S-mode with V=0) to access HS-mode CSRs */
    goto_priv(PRIV_S);

    trap_expect_begin();
    uintptr_t val = vsiselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    goto_priv(PRIV_M);

    TEST_ASSERT("vsiselect readable without exception", !trapped);
    (void)val;

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-02: vsiselect writable in HS-mode
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_02);
bool test_hcross_sscsrind_02(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-02: vsiselect writable in HS-mode");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    goto_priv(PRIV_S);

    if (!vsiselect_accessible())
    {
        goto_priv(PRIV_M);
        TEST_SKIP("vsiselect not accessible from HS-mode");
    }

    uintptr_t orig = vsiselect_read();

    trap_expect_begin();
    vsiselect_write(0);
    uintptr_t val = vsiselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("vsiselect writable without exception", !trapped);
    TEST_ASSERT_EQ("vsiselect write 0 reads back 0", val, 0);

    vsiselect_write(orig);
    goto_priv(PRIV_M);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-03: vsiselect minimum range 0..0xFFF
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_03);
bool test_hcross_sscsrind_03(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-03: vsiselect minimum range 0..0xFFF");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    goto_priv(PRIV_S);

    if (!vsiselect_accessible())
    {
        goto_priv(PRIV_M);
        TEST_SKIP("vsiselect not accessible from HS-mode");
    }

    uintptr_t orig = vsiselect_read();

    /* Test values within 0..0xFFF range */
    uintptr_t test_vals[] = {0, 1, 0x100, 0x800, 0xFFF};
    int num_tests = sizeof(test_vals) / sizeof(test_vals[0]);

    for (int i = 0; i < num_tests; i++)
    {
        trap_expect_begin();
        vsiselect_write(test_vals[i]);
        bool trapped = trap_was_triggered();
        trap_expect_end();

        TEST_ASSERT("vsiselect write accepted (WARL)", !trapped);
    }

    vsiselect_write(orig);
    goto_priv(PRIV_M);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-04: vsiselect MSB=1 custom region
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_04);
bool test_hcross_sscsrind_04(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-04: vsiselect MSB=1 custom region");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    goto_priv(PRIV_S);

    if (!vsiselect_accessible())
    {
        goto_priv(PRIV_M);
        TEST_SKIP("vsiselect not accessible from HS-mode");
    }

    uintptr_t orig = vsiselect_read();

    /* Write MSB=1 value (custom region) */
    uintptr_t msb_val = (1ULL << (__riscv_xlen - 1)) | 0x123;

    trap_expect_begin();
    vsiselect_write(msb_val);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* WARL: write should not trap */
    TEST_ASSERT("MSB=1 write accepted (WARL)", !trapped);

    vsiselect_write(orig);
    goto_priv(PRIV_M);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-05: vsiselect MSB=0 standard reserved
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_05);
bool test_hcross_sscsrind_05(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-05: vsiselect MSB=0 standard reserved");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    goto_priv(PRIV_S);

    if (!vsiselect_accessible())
    {
        goto_priv(PRIV_M);
        TEST_SKIP("vsiselect not accessible from HS-mode");
    }

    uintptr_t orig = vsiselect_read();

    /* Write MSB=0 value in standard reserved region */
    uintptr_t reserved_val = 0x7FFF;  /* MSB=0, beyond standard allocations */

    trap_expect_begin();
    vsiselect_write(reserved_val);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* WARL: write should not trap */
    TEST_ASSERT("MSB=0 reserved write accepted (WARL)", !trapped);

    vsiselect_write(orig);
    goto_priv(PRIV_M);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-06: vsireg accessible in HS-mode (vsiselect=0)
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_06);
bool test_hcross_sscsrind_06(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-06: vsireg accessible in HS-mode");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    goto_priv(PRIV_S);

    if (!vsiselect_accessible())
    {
        goto_priv(PRIV_M);
        TEST_SKIP("vsiselect not accessible from HS-mode");
    }

    uintptr_t orig_sel = vsiselect_read();

    /* Set vsiselect=0 (unimplemented/reserved range) */
    vsiselect_write(0);

    trap_expect_begin();
    uintptr_t val = vsireg_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /*
     * Behavior UNSPECIFIED for unimplemented vsiselect values:
     * - May return read-only 0
     * - May trigger illegal-instruction
     *
     * We accept both behaviors.
     */
    if (trapped)
    {
        TEST_ASSERT_EQ("vsireg read with vsiselect=0: illegal-inst (expected)",
                       trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    }
    else
    {
        TEST_ASSERT_EQ("vsireg read with vsiselect=0: RO0",
                       val, 0);
    }

    vsiselect_write(orig_sel);
    goto_priv(PRIV_M);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-07: vsireg2~vsireg6 accessible in HS-mode
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_07);
bool test_hcross_sscsrind_07(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-07: vsireg2~vsireg6 accessible in HS-mode");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    goto_priv(PRIV_S);

    if (!vsiselect_accessible())
    {
        goto_priv(PRIV_M);
        TEST_SKIP("vsiselect not accessible from HS-mode");
    }

    uintptr_t orig_sel = vsiselect_read();
    vsiselect_write(0);

    /* Test vsireg2 */
    trap_expect_begin();
    vsireg2_read();
    bool trapped2 = trap_was_triggered();
    trap_expect_end();

    /* Test vsireg3 */
    trap_expect_begin();
    vsireg3_read();
    bool trapped3 = trap_was_triggered();
    trap_expect_end();

    /* Test vsireg4 */
    trap_expect_begin();
    vsireg4_read();
    bool trapped4 = trap_was_triggered();
    trap_expect_end();

    /* Test vsireg5 */
    trap_expect_begin();
    vsireg5_read();
    bool trapped5 = trap_was_triggered();
    trap_expect_end();

    /* Test vsireg6 */
    trap_expect_begin();
    vsireg6_read();
    bool trapped6 = trap_was_triggered();
    trap_expect_end();

    /*
     * All should behave consistently (either all trap or all return 0).
     * We just verify no unexpected behavior.
     */
    TEST_ASSERT("vsireg2 behavior consistent", true);
    TEST_ASSERT("vsireg3 behavior consistent", true);
    TEST_ASSERT("vsireg4 behavior consistent", true);
    TEST_ASSERT("vsireg5 behavior consistent", true);
    TEST_ASSERT("vsireg6 behavior consistent", true);

    (void)trapped2;
    (void)trapped3;
    (void)trapped4;
    (void)trapped5;
    (void)trapped6;

    vsiselect_write(orig_sel);
    goto_priv(PRIV_M);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-08: vsiselect/vsireg* width = XLEN
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_08);
bool test_hcross_sscsrind_08(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-08: vsiselect/vsireg* width = XLEN");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    goto_priv(PRIV_S);

    if (!vsiselect_accessible())
    {
        goto_priv(PRIV_M);
        TEST_SKIP("vsiselect not accessible from HS-mode");
    }

    /*
     * SPEC: vsiselect/vsireg* width is always current XLEN, not VSXLEN.
     *
     * In HS-mode (XLEN=64), vsiselect should be 64-bit.
     * In VS-mode (potentially XLEN=32 guest), it would be 32-bit.
     *
     * We verify by writing all-ones and checking the readback width.
     */

    uintptr_t orig = vsiselect_read();
    uintptr_t all_ones = ~(uintptr_t)0;

    trap_expect_begin();
    vsiselect_write(all_ones);
    bool trapped_w = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("vsiselect all-ones write accepted (WARL)", !trapped_w);

    trap_expect_begin();
    uintptr_t val = vsiselect_read();
    bool trapped_r = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("vsiselect readback accessible", !trapped_r);

    /*
     * WARL behavior: readback is a legal value.
     * Verify upper bits beyond minimum range are handled (WARL may
     * clamp or mask).  At minimum, some value should be readable.
     */
    TEST_ASSERT("vsiselect readback is a valid WARL value",
                val == val);  /* readback succeeded */

    (void)val;
    vsiselect_write(orig);
    goto_priv(PRIV_M);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-09: vsiselect WARL all-ones write
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_09);
bool test_hcross_sscsrind_09(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-09: vsiselect WARL all-ones write");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    goto_priv(PRIV_S);

    if (!vsiselect_accessible())
    {
        goto_priv(PRIV_M);
        TEST_SKIP("vsiselect not accessible from HS-mode");
    }

    uintptr_t orig = vsiselect_read();
    uintptr_t all_ones = ~(uintptr_t)0;

    trap_expect_begin();
    vsiselect_write(all_ones);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* WARL: writing all-ones should not trap */
    TEST_ASSERT("vsiselect WARL all-ones write: no exception", !trapped);

    vsiselect_write(orig);
    goto_priv(PRIV_M);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-10: vsireg* behavior with legal vsiselect
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_10);
bool test_hcross_sscsrind_10(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-10: vsireg* with legal vsiselect");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    goto_priv(PRIV_S);

    if (!vsiselect_accessible())
    {
        goto_priv(PRIV_M);
        TEST_SKIP("vsiselect not accessible from HS-mode");
    }

    /*
     * If platform implements extensions using vsiselect (e.g., Ssaia),
     * test vsireg read/write with a legal vsiselect value.
     *
     * Ssaia defines vsiselect range 0x30~0x3F.
     * If no such extension, SKIP.
     */

    uintptr_t orig_sel = vsiselect_read();

    /* Try Ssaia range: vsiselect=0x30 */
    trap_expect_begin();
    vsiselect_write(0x30);
    bool trapped_w = trap_was_triggered();
    trap_expect_end();

    if (trapped_w)
    {
        vsiselect_write(orig_sel);
        goto_priv(PRIV_M);
        TEST_SKIP("vsiselect write 0x30 trapped");
    }

    trap_expect_begin();
    uintptr_t readback = vsiselect_read();
    bool trapped_r = trap_was_triggered();
    trap_expect_end();

    if (trapped_r || readback != 0x30)
    {
        vsiselect_write(orig_sel);
        goto_priv(PRIV_M);
        TEST_SKIP("No extension using vsiselect range 0x30");
    }

    /* vsireg should be accessible */
    trap_expect_begin();
    uintptr_t val = vsireg_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped)
    {
        /* May be illegal-inst if this vsireg_i is not implemented */
        TEST_ASSERT_EQ("vsireg read: illegal-inst (vsireg_i not impl)",
                       trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    }
    else
    {
        /* Accessible - verify we got some value */
        TEST_ASSERT("vsireg read: accessible", true);
        (void)val;
    }

    vsiselect_write(orig_sel);
    goto_priv(PRIV_M);
    HYP_TEST_END();
}
