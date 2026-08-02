/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_virt.c - Group 4: Virtualization Mode (V=1) Access Exceptions
 *
 * SRMCFG-19 ~ SRMCFG-24
 * Verifies that srmcfg access from VS/VU-mode (V=1) triggers
 * virtual-instruction exception when mstateen0[55]=1 or Smstateen
 * is not implemented.
 */

#ifdef ENABLE_HYP

/* Trampoline: read srmcfg in VS-mode */
static uintptr_t _vs_read_srmcfg(uintptr_t arg) {
    (void)arg;
    srmcfg_read();
    return 0;
}

/* Trampoline: write srmcfg in VS-mode */
static uintptr_t _vs_write_srmcfg(uintptr_t arg) {
    (void)arg;
    srmcfg_write(0);
    return 0;
}

/* Trampoline: read srmcfg in VU-mode */
static uintptr_t _vu_read_srmcfg(uintptr_t arg) {
    (void)arg;
    srmcfg_read();
    return 0;
}

#endif /* ENABLE_HYP */

/* ------------------------------------------------------------------ */
/* SRMCFG-19: V=1 VS-mode read srmcfg -> virtual-instruction          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_19);
bool test_srmcfg_19(void) {
    TEST_BEGIN("SRMCFG-19: VS-mode read srmcfg -> virtual-instruction");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

#ifdef ENABLE_HYP
    /* Ensure mstateen0[55]=1 if Smstateen is present */
    if (has_smstateen()) {
        mstateen0_set(MSTATEEN0_BIT55);
    }

    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_read_srmcfg, 0));

    HYP_TEST_END();
#else
    TEST_SKIP("ENABLE_HYP not compiled");
#endif
}

/* ------------------------------------------------------------------ */
/* SRMCFG-20: V=1 VS-mode write srmcfg -> virtual-instruction         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_20);
bool test_srmcfg_20(void) {
    TEST_BEGIN("SRMCFG-20: VS-mode write srmcfg -> virtual-instruction");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

#ifdef ENABLE_HYP
    /* Ensure mstateen0[55]=1 if Smstateen is present */
    if (has_smstateen()) {
        mstateen0_set(MSTATEEN0_BIT55);
    }

    EXPECT_VIRTUAL_INST(run_in_vs_mode(_vs_write_srmcfg, 0));

    HYP_TEST_END();
#else
    TEST_SKIP("ENABLE_HYP not compiled");
#endif
}

/* ------------------------------------------------------------------ */
/* SRMCFG-21: V=1 VU-mode access srmcfg -> virtual-instruction        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_21);
bool test_srmcfg_21(void) {
    TEST_BEGIN("SRMCFG-21: VU-mode access srmcfg -> virtual-instruction");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

#ifdef ENABLE_HYP
    /* Ensure mstateen0[55]=1 if Smstateen is present */
    if (has_smstateen()) {
        mstateen0_set(MSTATEEN0_BIT55);
    }

    /* VU-mode access to srmcfg (S-level CSR, address 0x181).
     * Per Ssqosid spec: "when V=1 raise virtual-instruction".
     * Per standard CSR rules: U accessing S-CSR -> illegal-instruction.
     * The priority between these two rules is ambiguous in the spec.
     * Accept both cause=2 and cause=22 as conforming. */
    trap_expect_begin();
    run_in_vu_mode(_vu_read_srmcfg, 0);
    TEST_ASSERT("VU-mode srmcfg access trapped", trap_was_triggered());
    if (trap_was_triggered()) {
        uintptr_t c = trap_get_cause();
        TEST_ASSERT("cause is 2 (illegal) or 22 (virtual-inst)",
                    c == CAUSE_ILLEGAL_INST ||
                    c == CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();

    HYP_TEST_END();
#else
    TEST_SKIP("ENABLE_HYP not compiled");
#endif
}

/* ------------------------------------------------------------------ */
/* SRMCFG-22: V=0 HS-mode normal access srmcfg                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_22);
bool test_srmcfg_22(void) {
    TEST_BEGIN("SRMCFG-22: V=0 HS-mode normal access srmcfg");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

#ifdef ENABLE_HYP
    /* Ensure mstateen0[55]=1 if Smstateen is present */
    if (has_smstateen()) {
        mstateen0_set(MSTATEEN0_BIT55);
    }

    /* V=0, HS-mode (S-mode in framework) should access normally */
    SSQOSID_TEST_SMODE_ALLOWED("HS-mode (V=0) read srmcfg",
                               srmcfg_read());
    SSQOSID_TEST_SMODE_ALLOWED("HS-mode (V=0) write srmcfg",
                               srmcfg_write(0));

    HYP_TEST_END();
#else
    TEST_SKIP("ENABLE_HYP not compiled");
#endif
}

/* ------------------------------------------------------------------ */
/* SRMCFG-23: mstateen0[55]=0 V=1 access -> illegal-instruction       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_23);
bool test_srmcfg_23(void) {
    TEST_BEGIN("SRMCFG-23: mstateen0[55]=0 VS-mode -> illegal-inst");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!has_smstateen()) TEST_SKIP("Smstateen not implemented");

#ifdef ENABLE_HYP
    uintptr_t orig = mstateen0_read();

    /* Clear bit 55: mstateen0 gating takes priority over V=1 rule */
    mstateen0_clear(MSTATEEN0_BIT55);

    /* VS-mode access should trigger illegal-instruction (cause=2)
     * because mstateen0[55]=0 blocks before V=1 rule applies */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_srmcfg, 0);
    TEST_ASSERT("trap triggered with mstateen0[55]=0",
                trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INST);
    }
    trap_expect_end();

    mstateen0_write(orig);
    HYP_TEST_END();
#else
    TEST_SKIP("ENABLE_HYP not compiled");
#endif
}

/* ------------------------------------------------------------------ */
/* SRMCFG-24: virtual-instruction trap stval/htinst values             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_srmcfg_24);
bool test_srmcfg_24(void) {
    TEST_BEGIN("SRMCFG-24: virtual-inst trap stval/htinst values");

    if (!has_ssqosid()) TEST_SKIP("Ssqosid not implemented");
    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

#ifdef ENABLE_HYP
    /* Ensure mstateen0[55]=1 if Smstateen is present */
    if (has_smstateen()) {
        mstateen0_set(MSTATEEN0_BIT55);
    }

    /* Trigger virtual-instruction from VS-mode */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_srmcfg, 0);
    TEST_ASSERT("virtual-inst trap triggered", trap_was_triggered());

    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

        /* stval: per H-spec, virtual-instruction trap writes stval
         * the same as illegal-instruction: either 0 or the faulting
         * instruction encoding. If non-zero, it must be a SYSTEM
         * instruction (opcode[6:0]=0x73) with CSR funct3. */
        uintptr_t tval = trap_get_tval();
        if (tval != 0) {
            TEST_ASSERT_EQ("stval opcode is SYSTEM (0x73)",
                           tval & 0x7F, 0x73);
            TEST_ASSERT("stval funct3 is CSR access (non-zero)",
                        (tval >> 12) & 0x7);
        } else {
            TEST_ASSERT("stval=0 is valid", true);
        }

        /* htinst: per H-spec, written with 0 or a transformed
         * instruction. For CSR access traps, 0 is the most common
         * conforming value (no standard pseudoinstruction defined
         * for CSR access). Accept 0 or a SYSTEM instruction. */
        uintptr_t htinst = trap_get_htinst();
        if (htinst != 0) {
            TEST_ASSERT_EQ("htinst opcode is SYSTEM (0x73)",
                           htinst & 0x7F, 0x73);
        } else {
            TEST_ASSERT("htinst=0 is valid", true);
        }
    }
    trap_expect_end();

    HYP_TEST_END();
#else
    TEST_SKIP("ENABLE_HYP not compiled");
#endif
}
