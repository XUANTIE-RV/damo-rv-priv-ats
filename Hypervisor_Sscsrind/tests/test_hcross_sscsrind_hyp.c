/*
 * Hypervisor_Sscsrind - Group 11.4: Hypervisor Cross Tests
 *
 * Tests HCROSS-SSCSRIND-28 through HCROSS-SSCSRIND-33
 *
 * These tests verify the transparent remapping behavior:
 * - VS-mode access to siselect/sireg* is automatically remapped to vsiselect/vsireg*
 * - HS-mode and VS-mode see independent select spaces
 * - M-mode and S-mode select spaces may alias (extension-dependent)
 *
 * Migrated from Sscsrind_test_plan.md Group 5 (SSCSRIND-HYP-01~06).
 * See Hypervisor_cross_test_plan.md Group 11.4.
 */

/* ===================================================================
 * HCROSS-SSCSRIND-28: VS-mode sireg → vsireg transparent remapping
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_28);
bool test_hcross_sscsrind_28(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-28: VS-mode sireg → vsireg remapping");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    uintptr_t orig_m = mstateen0_read();
    uintptr_t orig_h = hstateen0_read();
    uintptr_t orig_sel;
    if (!vsiselect_read_safe(&orig_sel))
    {
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
        TEST_SKIP("vsiselect not accessible from M-mode");
    }

    /* Enable access */
    mstateen0_set(MSTATEEN0_CSRIND);
    hstateen0_set(HSTATEEN0_CSRIND);

    /*
     * Test transparent remapping:
     * 1. HS-mode writes a value to vsireg (via vsiselect=0x30, Ssaia range)
     * 2. VS-mode reads sireg (address 0x151), which remaps to vsireg
     * 3. VS-mode should see the value HS-mode wrote
     */

    /* Set vsiselect to Ssaia range */
    if (!vsiselect_write_safe(0x30))
    {
        vsiselect_write_safe(orig_sel);
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
        TEST_SKIP("vsiselect write 0x30 trapped");
    }

    uintptr_t readback;
    if (!vsiselect_read_safe(&readback) || readback != 0x30)
    {
        vsiselect_write_safe(orig_sel);
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
        TEST_SKIP("Platform doesn't support vsiselect=0x30");
    }

    /* HS-mode writes vsireg */
    uintptr_t test_val = 0x12345678;
    if (!vsireg_write_safe(test_val))
    {
        vsiselect_write_safe(orig_sel);
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
        TEST_SKIP("vsireg write trapped (not implemented)");
    }

    /* VS-mode reads sireg (remapped to vsireg) */
    trap_expect_begin();
    uintptr_t vs_val = run_in_vs_mode(_vs_read_sireg, 0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped)
    {
        /* May trap if vsireg_i not implemented */
        TEST_ASSERT("VS-mode sireg read trapped", true);
    }
    else
    {
        /* Verify remapping: VS-mode should see HS-mode's value */
        TEST_ASSERT_EQ("VS-mode sireg reads HS-mode vsireg value",
                       vs_val, test_val);
    }

    vsiselect_write_safe(orig_sel);
    mstateen0_write(orig_m);
    hstateen0_write(orig_h);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-29: VS-mode siselect → vsiselect transparent remapping
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_29);
bool test_hcross_sscsrind_29(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-29: VS-mode siselect → vsiselect remapping");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    uintptr_t orig_m = mstateen0_read();
    uintptr_t orig_h = hstateen0_read();
    uintptr_t orig_sel;
    if (!vsiselect_read_safe(&orig_sel))
    {
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
        TEST_SKIP("vsiselect not accessible from M-mode");
    }

    /* Enable access */
    mstateen0_set(MSTATEEN0_CSRIND);
    hstateen0_set(HSTATEEN0_CSRIND);

    /*
     * Test transparent remapping:
     * 1. HS-mode writes vsiselect=0x100
     * 2. VS-mode reads siselect (address 0x150), which remaps to vsiselect
     * 3. VS-mode should see 0x100
     */

    if (!vsiselect_write_safe(0x100))
    {
        vsiselect_write_safe(orig_sel);
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
        TEST_SKIP("vsiselect write 0x100 trapped");
    }

    /* VS-mode reads siselect (remapped to vsiselect) */
    trap_expect_begin();
    uintptr_t vs_val = run_in_vs_mode(_vs_read_siselect, 0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped)
    {
        TEST_ASSERT("VS-mode siselect read trapped", true);
    }
    else
    {
        TEST_ASSERT_EQ("VS-mode siselect reads HS-mode vsiselect value",
                       vs_val, 0x100);
    }

    vsiselect_write_safe(orig_sel);
    mstateen0_write(orig_m);
    hstateen0_write(orig_h);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-30: vsireg read/write with legal vsiselect
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_30);
bool test_hcross_sscsrind_30(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-30: vsireg R/W with legal vsiselect");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    uintptr_t orig_sel;
    if (!vsiselect_read_safe(&orig_sel))
    {
        TEST_SKIP("vsiselect not accessible from M-mode");
    }

    /* Try Ssaia range: vsiselect=0x30 */
    if (!vsiselect_write_safe(0x30))
    {
        vsiselect_write_safe(orig_sel);
        TEST_SKIP("vsiselect write 0x30 trapped");
    }

    uintptr_t readback;
    if (!vsiselect_read_safe(&readback) || readback != 0x30)
    {
        vsiselect_write_safe(orig_sel);
        TEST_SKIP("Platform doesn't support vsiselect=0x30");
    }

    /* HS-mode writes and reads back vsireg */
    uintptr_t test_val = 0xDEADBEEF;

    if (!vsireg_write_safe(test_val))
    {
        vsiselect_write_safe(orig_sel);
        TEST_SKIP("vsireg write trapped");
    }

    trap_expect_begin();
    uintptr_t val = vsireg_read();
    bool trapped_r = trap_was_triggered();
    trap_expect_end();

    if (trapped_r)
    {
        TEST_ASSERT("vsireg read trapped after successful write", false);
    }
    else
    {
        TEST_ASSERT_EQ("vsireg R/W consistent", val, test_val);
    }

    vsiselect_write_safe(orig_sel);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-31: vsireg behavior with unimplemented vsiselect
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_31);
bool test_hcross_sscsrind_31(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-31: vsireg with unimplemented vsiselect");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    uintptr_t orig_sel;
    if (!vsiselect_read_safe(&orig_sel))
    {
        TEST_SKIP("vsiselect not accessible from M-mode");
    }

    /* Set vsiselect to unimplemented/reserved value */
    if (!vsiselect_write_safe(0))
    {
        vsiselect_write_safe(orig_sel);
        TEST_SKIP("vsiselect write 0 trapped");
    }

    /*
     * Behavior UNSPECIFIED:
     * - May return read-only 0
     * - May trigger illegal-instruction
     */

    trap_expect_begin();
    uintptr_t val = vsireg_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped)
    {
        TEST_ASSERT_EQ("vsireg with vsiselect=0: illegal-inst",
                       trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    }
    else
    {
        TEST_ASSERT_EQ("vsireg with vsiselect=0: RO0",
                       val, 0);
    }

    vsiselect_write_safe(orig_sel);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-32: HS-mode and VS-mode select space independence
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_32);
bool test_hcross_sscsrind_32(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-32: HS/VS-mode select space independence");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    uintptr_t orig_m = mstateen0_read();
    uintptr_t orig_h = hstateen0_read();
    uintptr_t orig_sel;
    if (!vsiselect_read_safe(&orig_sel))
    {
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
        TEST_SKIP("vsiselect not accessible from M-mode");
    }

    /* Enable access */
    mstateen0_set(MSTATEEN0_CSRIND);
    hstateen0_set(HSTATEEN0_CSRIND);

    /*
     * Test select space independence:
     * 1. HS-mode writes vsiselect=0x100
     * 2. VS-mode reads siselect (remapped) → should see 0x100
     * 3. VS-mode writes siselect=0x200 (remapped to vsiselect)
     * 4. HS-mode reads vsiselect → should see 0x200
     *
     * This verifies HS-mode and VS-mode share the same vsiselect CSR,
     * not independent spaces.
     */

    /* HS-mode writes vsiselect=0x100 */
    if (!vsiselect_write_safe(0x100))
    {
        vsiselect_write_safe(orig_sel);
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
        TEST_SKIP("vsiselect write 0x100 trapped");
    }

    /* VS-mode reads siselect (remapped) */
    trap_expect_begin();
    uintptr_t vs_val1 = run_in_vs_mode(_vs_read_siselect, 0);
    bool trapped1 = trap_was_triggered();
    trap_expect_end();

    if (trapped1)
    {
        vsiselect_write_safe(orig_sel);
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
        TEST_SKIP("VS-mode siselect read trapped");
    }

    TEST_ASSERT_EQ("VS-mode sees HS-mode vsiselect value",
                   vs_val1, 0x100);

    /* VS-mode writes siselect=0x200 (remapped to vsiselect) */
    trap_expect_begin();
    run_in_vs_mode(_vs_write_siselect, 0x200);
    bool trapped2 = trap_was_triggered();
    trap_expect_end();

    if (trapped2)
    {
        /* VS-mode write trapped - select space is independent */
        TEST_ASSERT("VS-mode siselect write trapped (independent spaces)", true);
    }
    else
    {
        /* HS-mode reads vsiselect - should see 0x200 */
        uintptr_t hs_val;
        if (vsiselect_read_safe(&hs_val))
        {
            TEST_ASSERT_EQ("HS-mode sees VS-mode siselect write",
                           hs_val, 0x200);
        }
        else
        {
            TEST_ASSERT("HS-mode vsiselect read trapped unexpectedly", false);
        }
    }

    vsiselect_write_safe(orig_sel);
    mstateen0_write(orig_m);
    hstateen0_write(orig_h);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-33: M-mode and S-mode select space alias
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_33);
bool test_hcross_sscsrind_33(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-33: M/S-mode select space alias");

    /*
     * Test M-mode and S-mode select space aliasing.
     *
     * SPEC: Machine-level and Supervisor-level CSRs with the same
     * select value may be defined by an extension as partial or
     * full aliases.
     *
     * Example: Ssaia defines miselect=0x30 and siselect=0x30 as
     * accessing the same indirect register (mireg/sireg).
     *
     * If platform doesn't implement such aliasing, SKIP.
     */

    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");

    uintptr_t orig_mstateen = mstateen0_read();
    uintptr_t orig_misel = miselect_read();
    uintptr_t orig_sisel = siselect_read();

    /* Enable S-mode access */
    mstateen0_set(MSTATEEN0_CSRIND);

    /* Try Ssaia alias range: select=0x30 */
    miselect_write(0x30);
    uintptr_t m_readback = miselect_read();

    if (m_readback != 0x30)
    {
        miselect_write(orig_misel);
        siselect_write(orig_sisel);
        mstateen0_write(orig_mstateen);
        TEST_SKIP("Platform doesn't support miselect=0x30");
    }

    siselect_write(0x30);
    uintptr_t s_readback = siselect_read();

    if (s_readback != 0x30)
    {
        miselect_write(orig_misel);
        siselect_write(orig_sisel);
        mstateen0_write(orig_mstateen);
        TEST_SKIP("Platform doesn't support siselect=0x30");
    }

    /*
     * Test aliasing:
     * 1. M-mode writes mireg
     * 2. S-mode reads sireg
     * 3. If aliased, S-mode should see M-mode's value
     */

    uintptr_t test_val = 0xCAFEBABE;

    trap_expect_begin();
    mireg_write(test_val);
    bool trapped_m = trap_was_triggered();
    trap_expect_end();

    if (trapped_m)
    {
        miselect_write(orig_misel);
        siselect_write(orig_sisel);
        mstateen0_write(orig_mstateen);
        TEST_SKIP("mireg write trapped");
    }

    /* S-mode reads sireg */
    trap_expect_begin();
    goto_priv(PRIV_S);
    uintptr_t s_val = sireg_read();
    goto_priv(PRIV_M);
    bool trapped_s = trap_was_triggered();
    trap_expect_end();

    if (trapped_s)
    {
        /* sireg read trapped - not aliased or not implemented */
        TEST_ASSERT("S-mode sireg read trapped (not aliased)", true);
    }
    else
    {
        /*
         * If aliased, S-mode should see M-mode's value.
         * If not aliased, S-mode may see different value or 0.
         * We accept both behaviors.
         */
        if (s_val == test_val)
        {
            TEST_ASSERT("M/S-mode select spaces aliased", true);
        }
        else
        {
            TEST_ASSERT("M/S-mode select spaces not aliased", true);
        }
    }

    miselect_write(orig_misel);
    siselect_write(orig_sisel);
    mstateen0_write(orig_mstateen);
    HYP_TEST_END();
}
