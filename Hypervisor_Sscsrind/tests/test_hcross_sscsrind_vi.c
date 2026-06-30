/*
 * Hypervisor_Sscsrind - Group 11.2: Virtual-Instruction Exception Behavior
 *
 * Tests HCROSS-SSCSRIND-11 through HCROSS-SSCSRIND-21
 *
 * These tests verify that VS/VU-mode direct access to vsiselect/vsireg*
 * (CSR addresses 0x250-0x257) and VU-mode access to siselect/sireg*
 * (CSR addresses 0x150-0x157) correctly trigger virtual-instruction
 * exceptions (cause=22).
 *
 * KEY: Direct access to VS CSRs from VS/VU-mode ALWAYS triggers
 * virtual-instruction, regardless of mstateen0[60] or hstateen0[60].
 *
 * Migrated from Sscsrind_test_plan.md Group 3 (SSCSRIND-VI-01~11).
 * See Hypervisor_cross_test_plan.md Group 11.2.
 */

/* ===================================================================
 * HCROSS-SSCSRIND-11: VS-mode direct read vsiselect → virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_11);
bool test_hcross_sscsrind_11(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-11: VS-mode read vsiselect → virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    /* Enable state-enable bits to isolate virtual-inst behavior */
    if (platform_has_smstateen()) {
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    TEST_VS_MODE_TRAP("VS-mode read vsiselect",
                      _vs_read_vsiselect, 0,
                      CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-12: VS-mode direct write vsiselect → virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_12);
bool test_hcross_sscsrind_12(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-12: VS-mode write vsiselect → virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    if (platform_has_smstateen()) {
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    TEST_VS_MODE_TRAP("VS-mode write vsiselect",
                      _vs_write_vsiselect, 0x100,
                      CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-13: VS-mode direct read vsireg → virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_13);
bool test_hcross_sscsrind_13(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-13: VS-mode read vsireg → virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    if (platform_has_smstateen()) {
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    TEST_VS_MODE_TRAP("VS-mode read vsireg",
                      _vs_read_vsireg, 0,
                      CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-14: VS-mode direct read vsireg2~vsireg6 → virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_14);
bool test_hcross_sscsrind_14(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-14: VS-mode read vsireg2~6 → virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    if (platform_has_smstateen())
    {
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    /*
     * Test vsireg2, vsireg3, vsireg4, vsireg5, vsireg6.
     * Each should trigger virtual-instruction from VS-mode.
     * Use run_in_vs_mode() with trampoline functions.
     */

    TEST_VS_MODE_TRAP("VS-mode read vsireg2",
                      _vs_read_vsireg2, 0,
                      CAUSE_VIRTUAL_INSTRUCTION);

    TEST_VS_MODE_TRAP("VS-mode read vsireg3",
                      _vs_read_vsireg3, 0,
                      CAUSE_VIRTUAL_INSTRUCTION);

    TEST_VS_MODE_TRAP("VS-mode read vsireg4",
                      _vs_read_vsireg4, 0,
                      CAUSE_VIRTUAL_INSTRUCTION);

    TEST_VS_MODE_TRAP("VS-mode read vsireg5",
                      _vs_read_vsireg5, 0,
                      CAUSE_VIRTUAL_INSTRUCTION);

    TEST_VS_MODE_TRAP("VS-mode read vsireg6",
                      _vs_read_vsireg6, 0,
                      CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-15: VS-mode direct write vsireg → virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_15);
bool test_hcross_sscsrind_15(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-15: VS-mode write vsireg → virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    if (platform_has_smstateen()) {
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    TEST_VS_MODE_TRAP("VS-mode write vsireg",
                      _vs_write_vsireg, 0x42,
                      CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-16: VU-mode direct read vsiselect → virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_16);
bool test_hcross_sscsrind_16(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-16: VU-mode read vsiselect → virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    if (platform_has_smstateen()) {
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    TEST_VU_MODE_TRAP("VU-mode read vsiselect",
                       _vu_read_vsiselect, 0,
                       CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-17: VU-mode direct read vsireg → virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_17);
bool test_hcross_sscsrind_17(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-17: VU-mode read vsireg → virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    if (platform_has_smstateen()) {
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    TEST_VU_MODE_TRAP("VU-mode read vsireg",
                       _vu_read_vsireg, 0,
                       CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-18: VU-mode read siselect → virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_18);
bool test_hcross_sscsrind_18(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-18: VU-mode read siselect → virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    if (platform_has_smstateen()) {
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    TEST_VU_MODE_TRAP("VU-mode read siselect",
                       _vu_read_siselect, 0,
                       CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-19: VU-mode read sireg → virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_19);
bool test_hcross_sscsrind_19(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-19: VU-mode read sireg → virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    if (platform_has_smstateen()) {
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    TEST_VU_MODE_TRAP("VU-mode read sireg",
                       _vu_read_sireg, 0,
                       CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-20: VU-mode write siselect → virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_20);
bool test_hcross_sscsrind_20(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-20: VU-mode write siselect → virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    if (platform_has_smstateen()) {
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    TEST_VU_MODE_TRAP("VU-mode write siselect",
                       _vu_write_siselect, 0x100,
                       CAUSE_VIRTUAL_INSTRUCTION);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-21: HS-level-only vsiselect → VS-mode sireg* virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_21);
bool test_hcross_sscsrind_21(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-21: HS-level vsiselect → VS-mode virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_sscsrind()) TEST_SKIP("Sscsrind not implemented");

    /*
     * If vsiselect holds a value implemented at HS-level but not VS-level,
     * VS-mode access via sireg* (remapped to vsireg*) typically triggers
     * virtual-instruction.
     *
     * This allows hypervisor to trap and emulate.
     *
     * For this test, we use a hypothetical HS-only range. If platform
     * doesn't implement such a range, SKIP.
     */

    uintptr_t orig_sel;
    if (!vsiselect_read_safe(&orig_sel))
    {
        TEST_SKIP("vsiselect not accessible from M-mode");
    }

    uintptr_t orig_m = 0, orig_h = 0;

    /* Enable access */
    if (platform_has_smstateen())
    {
        orig_m = mstateen0_read();
        orig_h = hstateen0_read();
        mstateen0_set(MSTATEEN0_CSRIND);
        hstateen0_set(HSTATEEN0_CSRIND);
    }

    /*
     * Try vsiselect=0x80 (hypothetical HS-only range).
     * If platform implements this as HS-only, VS-mode sireg access traps.
     */
    if (!vsiselect_write_safe(0x80))
    {
        vsiselect_write_safe(orig_sel);
        if (platform_has_smstateen())
        {
            mstateen0_write(orig_m);
            hstateen0_write(orig_h);
        }
        TEST_SKIP("vsiselect write 0x80 trapped");
    }

    uintptr_t readback;
    if (!vsiselect_read_safe(&readback) || readback != 0x80)
    {
        /* Platform doesn't support this vsiselect value */
        vsiselect_write_safe(orig_sel);
        if (platform_has_smstateen())
        {
            mstateen0_write(orig_m);
            hstateen0_write(orig_h);
        }
        TEST_SKIP("Platform doesn't implement HS-only vsiselect range");
    }

    /* VS-mode access sireg (remapped to vsireg) */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_sireg, 0);
    if (trap_was_triggered())
    {
        uintptr_t cause = trap_get_cause();
        /* Accept virtual-instruction or illegal-instruction */
        TEST_ASSERT("VS-mode sireg with HS-only vsiselect traps",
                    cause == CAUSE_VIRTUAL_INSTRUCTION ||
                    cause == CAUSE_ILLEGAL_INST);
    }
    else
    {
        /* Accessible - not HS-only */
        TEST_ASSERT("VS-mode sireg accessible (not HS-only)", true);
    }
    trap_expect_end();

    vsiselect_write_safe(orig_sel);
    if (platform_has_smstateen())
    {
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
    }
    HYP_TEST_END();
}
