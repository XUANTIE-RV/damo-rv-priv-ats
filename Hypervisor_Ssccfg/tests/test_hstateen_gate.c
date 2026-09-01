/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hstateen_gate.c - Group 11.5: hstateen0 bit 60 gating of
 * VS-mode indirect counter access (Ssccfg perspective)
 *
 * Tests HCROSS-SSCCFG-22 through HCROSS-SSCCFG-24
 * (migrated from SSCFG-STA-04~06 in Ssccfg_test_plan.md).
 *
 * norm:hstateen0_csrind_op:
 *   hstateen0 bit 60 (CSRIND) controls VS-mode access to
 *   siselect/sireg* (really vsiselect/vsireg*). With
 *   mstateen0[60]=1 and hstateen0[60]=0, VS-mode access raises a
 *   virtual-instruction exception.
 *
 * See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 11.
 */

#ifdef ENABLE_HYP

/* Preconditions: H extension + Smstateen with writable gating bits.
 * Returns false (with reason) when the test must be skipped. */
static bool hstateen_preconditions(const char **skip_reason)
{
    if (!HAS_H_EXT()) {
        *skip_reason = "H extension not supported";
        return false;
    }
    if (!platform_has_smstateen()) {
        *skip_reason = "Smstateen not supported";
        return false;
    }

    /* mstateen0[60] must be settable (grant HS-mode access) */
    mstateen0_set(STATEEN0_CSRIND);
    if ((mstateen0_read() & STATEEN0_CSRIND) == 0) {
        *skip_reason = "mstateen0 bit 60 not writable";
        return false;
    }

    /* hstateen0[60] must be writable for the gating tests */
    uintptr_t orig = hstateen0_read();
    hstateen0_write(orig & ~STATEEN0_CSRIND);
    bool clear_ok = ((hstateen0_read() & STATEEN0_CSRIND) == 0);
    hstateen0_write(orig);
    if (!clear_ok) {
        *skip_reason = "hstateen0 bit 60 not writable";
        return false;
    }
    return true;
}

/* Use a select value OUTSIDE the delegated range so the Ssccfg CDE
 * rules (Group 11.4) do not interfere with the state-enable gate. */
#define HSTA_TEST_SELECT  0x60

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-22: hstateen0[60]=0 blocks VS write to siselect     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_22_hsta0_block_siselect);
bool test_hcross_ssccfg_22_hsta0_block_siselect(void) {
    TEST_BEGIN("HCROSS-SSCCFG-22: hstateen0[60]=0 blocks VS siselect");
    const char *skip;
    if (!hstateen_preconditions(&skip)) TEST_SKIP(skip);

    uintptr_t orig_hsta = hstateen0_read();
    hstateen0_clear(STATEEN0_CSRIND);

    TEST_VS_MODE_TRAP("VS siselect write virtual-inst",
                      _vs_write_siselect, HSTA_TEST_SELECT,
                      CAUSE_VIRTUAL_INSTRUCTION);

    hstateen0_write(orig_hsta);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-23: hstateen0[60]=0 blocks VS read of sireg         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_23_hsta0_block_sireg);
bool test_hcross_ssccfg_23_hsta0_block_sireg(void) {
    TEST_BEGIN("HCROSS-SSCCFG-23: hstateen0[60]=0 blocks VS sireg");
    const char *skip;
    if (!hstateen_preconditions(&skip)) TEST_SKIP(skip);

    uintptr_t orig_hsta = hstateen0_read();
    hstateen0_clear(STATEEN0_CSRIND);

    TEST_VS_MODE_TRAP("VS sireg read virtual-inst",
                      _vs_read_sireg, 0, CAUSE_VIRTUAL_INSTRUCTION);

    hstateen0_write(orig_hsta);
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSCCFG-24: hstateen0[60]=1 allows VS-mode access           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_ssccfg_24_hsta1_allow);
bool test_hcross_ssccfg_24_hsta1_allow(void) {
    TEST_BEGIN("HCROSS-SSCCFG-24: hstateen0[60]=1 allows VS access");
    const char *skip;
    if (!hstateen_preconditions(&skip)) TEST_SKIP(skip);

    /* Disable CDE and use a non-delegated select value so that the
     * access is gated by hstateen0 alone (no CDE interference). */
    uintptr_t orig_cde = menvcfg_read();
    menvcfg_write(orig_cde & ~MENVCFG_CDE);
    uintptr_t orig_hsta = hstateen0_read();
    hstateen0_set(STATEEN0_CSRIND);

    trap_expect_begin();
    uintptr_t rb = run_in_vs_mode(_vs_write_read_siselect, HSTA_TEST_SELECT);
    bool trapped = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("VS siselect access no trap with hstateen0[60]=1", !trapped);

    /* The write must have been remapped to vsiselect */
    uintptr_t vsisel = vsiselect_read();
    TEST_ASSERT_EQ("VS write remapped to vsiselect",
                   vsisel, (uintptr_t)HSTA_TEST_SELECT);
    (void)rb;

    hstateen0_write(orig_hsta);
    menvcfg_write(orig_cde);
    HYP_TEST_END();
}

#endif /* ENABLE_HYP */
