/*
 * Hypervisor_Sscsrind - Group 11.3: State-Enable Access Control
 *
 * Tests HCROSS-SSCSRIND-22 through HCROSS-SSCSRIND-27
 *
 * These tests verify mstateen0[60] and hstateen0[60] control behavior
 * for VS/VU-mode access to siselect/sireg* (remapped to vsiselect/vsireg*).
 *
 * KEY: When mstateen0[60]=1 but hstateen0[60]=0, VS/VU-mode access
 * triggers virtual-instruction (cause=22), NOT illegal-instruction (cause=2).
 * This allows hypervisor to trap and handle.
 *
 * Migrated from Sscsrind_test_plan.md Group 4.2/4.3 (SSCSRIND-STA-05~10).
 * See Hypervisor_cross_test_plan.md Group 11.3.
 */

/* ===================================================================
 * HCROSS-SSCSRIND-22: mstateen0[60]=0 blocks HS-mode read vsiselect
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_22);
bool test_hcross_sscsrind_22(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-22: mstateen0[60]=0 blocks HS-mode vsiselect");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");

    uintptr_t orig_m = mstateen0_read();

    /* Clear mstateen0[60] (CSRIND) */
    mstateen0_clear(MSTATEEN0_CSRIND);

    /*
     * HS-mode (V=0) access vsiselect.
     * In Hypervisor mode, S-mode with V=0 is HS-mode.
     */
    trap_expect_begin();
    goto_priv(PRIV_S);  /* HS-mode: S-mode with V=0 */
    asm volatile("csrr t0, 0x250" ::: "t0", "memory");  /* vsiselect */
    goto_priv(PRIV_M);
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("HS-mode vsiselect with mstateen0[60]=0",
                       trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    } else {
        TEST_ASSERT("HS-mode vsiselect should trap (illegal-inst)", false);
    }
    trap_expect_end();

    mstateen0_write(orig_m);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-23: mstateen0[60]=0 blocks HS-mode read vsireg
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_23);
bool test_hcross_sscsrind_23(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-23: mstateen0[60]=0 blocks HS-mode vsireg");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");

    uintptr_t orig_m = mstateen0_read();

    /* Clear mstateen0[60] */
    mstateen0_clear(MSTATEEN0_CSRIND);

    /* HS-mode access vsireg */
    trap_expect_begin();
    goto_priv(PRIV_S);
    asm volatile("csrr t0, 0x251" ::: "t0", "memory");  /* vsireg */
    goto_priv(PRIV_M);
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("HS-mode vsireg with mstateen0[60]=0",
                       trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    } else {
        TEST_ASSERT("HS-mode vsireg should trap (illegal-inst)", false);
    }
    trap_expect_end();

    mstateen0_write(orig_m);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-24: hstateen0[60]=0 + mstateen0[60]=1 → VS-mode virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_24);
bool test_hcross_sscsrind_24(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-24: hstateen0[60]=0 → VS-mode virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");

    uintptr_t orig_m = mstateen0_read();
    uintptr_t orig_h = hstateen0_read();

    /* mstateen0[60]=1, hstateen0[60]=0 */
    mstateen0_set(MSTATEEN0_CSRIND);
    hstateen0_clear(HSTATEEN0_CSRIND);

    /*
     * VS-mode reads siselect (remapped to vsiselect).
     * Should trap with virtual-instruction (cause=22).
     */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_siselect, 0);
    if (trap_was_triggered()) {
        uintptr_t cause = trap_get_cause();
        TEST_ASSERT_EQ("VS-mode siselect with hstateen0[60]=0",
                       cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    } else {
        TEST_ASSERT("VS-mode siselect should trap (virtual-inst)", false);
    }
    trap_expect_end();

    mstateen0_write(orig_m);
    hstateen0_write(orig_h);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-25: hstateen0[60]=0 + mstateen0[60]=1 → VS-mode sireg virtual-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_25);
bool test_hcross_sscsrind_25(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-25: hstateen0[60]=0 → VS-mode sireg virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");

    uintptr_t orig_m = mstateen0_read();
    uintptr_t orig_h = hstateen0_read();

    /* mstateen0[60]=1, hstateen0[60]=0 */
    mstateen0_set(MSTATEEN0_CSRIND);
    hstateen0_clear(HSTATEEN0_CSRIND);

    /* VS-mode reads sireg (remapped to vsireg) */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_sireg, 0);
    if (trap_was_triggered()) {
        uintptr_t cause = trap_get_cause();
        TEST_ASSERT_EQ("VS-mode sireg with hstateen0[60]=0",
                       cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    } else {
        TEST_ASSERT("VS-mode sireg should trap (virtual-inst)", false);
    }
    trap_expect_end();

    mstateen0_write(orig_m);
    hstateen0_write(orig_h);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-26: hstateen0[60]=1 + mstateen0[60]=1 → VS-mode access OK
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_26);
bool test_hcross_sscsrind_26(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-26: hstateen0[60]=1 → VS-mode access OK");

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

    /* Enable access: mstateen0[60]=1, hstateen0[60]=1 */
    mstateen0_set(MSTATEEN0_CSRIND);
    hstateen0_set(HSTATEEN0_CSRIND);

    /* Set vsiselect to a legal value (0 = reserved but accessible) */
    if (!vsiselect_write_safe(0))
    {
        vsiselect_write_safe(orig_sel);
        mstateen0_write(orig_m);
        hstateen0_write(orig_h);
        TEST_SKIP("vsiselect write 0 trapped");
    }

    /*
     * VS-mode reads siselect (remapped to vsiselect).
     * Should NOT trap (access allowed).
     *
     * Note: sireg access with vsiselect=0 may still trap (UNSPECIFIED),
     * but siselect itself should be readable.
     */
    trap_expect_begin();
    uintptr_t val = run_in_vs_mode(_vs_read_siselect, 0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped)
    {
        TEST_ASSERT("VS-mode siselect should not trap (access allowed)", false);
    }
    else
    {
        TEST_ASSERT("VS-mode siselect accessible", true);
        (void)val;
    }

    vsiselect_write_safe(orig_sel);
    mstateen0_write(orig_m);
    hstateen0_write(orig_h);
    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSCSRIND-27: Exception type must be virtual-inst not illegal-inst
 * =================================================================== */
TEST_REGISTER(test_hcross_sscsrind_27);
bool test_hcross_sscsrind_27(void)
{
    TEST_BEGIN("HCROSS-SSCSRIND-27: Exception type virtual-inst not illegal-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");

    uintptr_t orig_m = mstateen0_read();
    uintptr_t orig_h = hstateen0_read();

    /*
     * KEY TEST: mstateen0[60]=1, hstateen0[60]=0
     *
     * VS-mode access siselect/sireg* should trigger:
     *   - virtual-instruction (cause=22), NOT
     *   - illegal-instruction (cause=2)
     *
     * This is critical: M-mode allows (mstateen=1), but HS-mode hypervisor
     * chooses not to allow (hstateen=0). Exception type reflects that
     * hypervisor needs to trap and handle.
     */

    mstateen0_set(MSTATEEN0_CSRIND);
    hstateen0_clear(HSTATEEN0_CSRIND);

    /* VS-mode reads siselect */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_siselect, 0);
    if (trap_was_triggered()) {
        uintptr_t cause = trap_get_cause();

        /*
         * CRITICAL: Must be virtual-instruction (22), NOT illegal-instruction (2).
         * This distinguishes hypervisor-controlled access from M-mode blocked.
         */
        TEST_ASSERT_EQ("Exception cause must be virtual-instruction (22)",
                       cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

        /* Explicitly verify it's NOT illegal-instruction */
        TEST_ASSERT("Exception cause must NOT be illegal-instruction (2)",
                    cause != (uintptr_t)CAUSE_ILLEGAL_INST);
    } else {
        TEST_ASSERT("VS-mode siselect should trap (virtual-inst)", false);
    }
    trap_expect_end();

    mstateen0_write(orig_m);
    hstateen0_write(orig_h);
    HYP_TEST_END();
}
