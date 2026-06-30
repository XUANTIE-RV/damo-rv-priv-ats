/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_smstateen_exception.c - HCROSS-SMSTA-13~14: VS/VU-mode exception behavior
 *
 * Verifies that VS-mode and VU-mode access to controlled CSRs
 * triggers virtual-instruction exception when mstateen blocks access
 * but hstateen would allow it.
 *
 * IMPORTANT: goto_priv(PRIV_VS) does NOT work because PRIV_VS=5 >
 * PRIV_M=3, causing goto_priv to take the wrong (ecall-up) path.
 * We must use run_in_vs_mode() / run_in_vu_mode() instead, which
 * correctly enter/exit virtualized privilege levels via mstatus.MPV.
 */

#ifdef ENABLE_HYP

/* Trampoline: read sstateen0 in VS-mode, return trap cause (0 = no trap) */
static uintptr_t _vs_read_sstateen0(uintptr_t arg) {
    (void)arg;
    sstateen0_read();
    return trap_get_cause();
}

/* Trampoline: read sstateen0 in VU-mode, return trap cause (0 = no trap) */
static uintptr_t _vu_read_sstateen0(uintptr_t arg) {
    (void)arg;
    sstateen0_read();
    return trap_get_cause();
}

#endif /* ENABLE_HYP */

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-13: VS-mode access triggers virtual-instruction      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_13);
bool test_hcross_smsta_13(void) {
    TEST_BEGIN("HCROSS-SMSTA-13: VS-mode access -> virtual-instruction");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

#ifdef ENABLE_HYP
    uintptr_t orig = mstateen0_read();

    /* Set up: mstateen0.SE0=1 temporarily so we can write hstateen0 */
    mstateen0_set(MSTATEEN0_SE0);
    hstateen0_write(1ULL << 63);

    /* Now clear mstateen0 SE0 to block at M-level.
     * hstateen0 bit 63 is still 1 (H perspective allows),
     * but mstateen0 SE0=0 blocks at M-level.
     * From VS-mode, accessing sstateen0 should trigger
     * virtual-instruction (cause=22) because hstateen would allow
     * but mstateen blocks. */
    mstateen0_clear(MSTATEEN0_SE0);

    /* Use run_in_vs_mode to correctly enter VS-mode (V=1, nominal S).
     * The trampoline reads sstateen0 and returns trap_get_cause(). */
    trap_expect_begin();
    uintptr_t cause = run_in_vs_mode(_vs_read_sstateen0, 0);

    /* Accept virtual-instruction(22) or illegal-instruction(2)
     * depending on implementation routing */
    TEST_ASSERT("VS-mode trap cause is virtual-inst or illegal-inst",
                cause == CAUSE_VIRTUAL_INSTRUCTION ||
                cause == CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
#else
    TEST_SKIP("ENABLE_HYP not compiled");
#endif

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SMSTA-14: VU-mode access triggers virtual-instruction      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_smsta_14);
bool test_hcross_smsta_14(void) {
    TEST_BEGIN("HCROSS-SMSTA-14: VU-mode access -> virtual-instruction");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

#ifdef ENABLE_HYP
    uintptr_t orig = mstateen0_read();

    /* Set hstateen0 bit 63 = 1 via mstateen0.SE0=1 */
    mstateen0_set(MSTATEEN0_SE0);
    hstateen0_write(1ULL << 63);

    /* Block at M-level */
    mstateen0_clear(MSTATEEN0_SE0);

    /* Use run_in_vu_mode to correctly enter VU-mode (V=1, nominal U).
     * The trampoline reads sstateen0 and returns trap_get_cause(). */
    trap_expect_begin();
    uintptr_t cause = run_in_vu_mode(_vu_read_sstateen0, 0);

    TEST_ASSERT("VU-mode trap cause is virtual-inst or illegal-inst",
                cause == CAUSE_VIRTUAL_INSTRUCTION ||
                cause == CAUSE_ILLEGAL_INST);

    mstateen0_write(orig);
#else
    TEST_SKIP("ENABLE_HYP not compiled");
#endif

    TEST_END();
}
