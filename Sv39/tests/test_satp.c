/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_satp.c - Group 11: satp CSR Control
 *
 * Shared by Sv39/Sv48/Sv57; per-mode test IDs, function names, the
 * report mode name and the mode-switch target are derived from
 * SUITE_SATP_MODE in test_helpers.h. ID mapping (vm_test_plan.md):
 *   enable-VM: SATP-02 (Sv39) / SATP-03 (Sv48) / SATP-04 (Sv57)
 *   switch:    SATP-05 (Sv39 -> Sv48) / SATP-06 (Sv48 -> Sv57)
 * SATP-01/07/09 are identical across modes. Sv57 is the highest mode,
 * so it has no mode-switch test (compiled out with #if below).
 *
 * Whether a mode-specific case runs is decided by the compile-time
 * REQUIRE_SATP_MODE gate (platform config declaration), NOT by a
 * runtime probe of satp.MODE.
 */

SV_REGISTER(satp01);
bool SVFN(satp01)(void) {
    TEST_BEGIN("SATP-01: MODE=Bare disables VM");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SUITE_SATP_MODE);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Write satp with MODE=Bare (0) */
    CSRW(satp, 0);
    vm_sfence_vma(0, 0);

    /* Verify satp.MODE is Bare */
    uintptr_t satp_val = CSRR(satp);
    uintptr_t mode = satp_val >> SATP_MODE_SHIFT;
    TEST_ASSERT("satp.MODE is Bare", mode == SATP_MODE_BARE);

    pt_pool_reset();
    TEST_END();
}

SV_REGISTER(SFX_SATP_ENABLE);
bool SVFN(SFX_SATP_ENABLE)(void) {
    TEST_BEGIN(ID_SATP_ENABLE ": MODE=" SUITE_MODE_NAME " enables VM");
    REQUIRE_SATP_MODE(SUITE_SATP_MODE);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SUITE_SATP_MODE);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test_data_area (in the separate VM test region) */
    uintptr_t data_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, data_va, data_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Enable VM and verify it works */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT(SUITE_MODE_NAME " VM enabled, read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* Sv57 is the highest supported mode: there is no higher mode to switch
 * to, so the mode-switch test exists only for Sv39 (-> Sv48) and Sv48
 * (-> Sv57). */
#if SUITE_SATP_MODE != SATP_MODE_SV57
SV_REGISTER(SFX_SATP_SWITCH);
bool SVFN(SFX_SATP_SWITCH)(void) {
    TEST_BEGIN(ID_SATP_SWITCH ": Mode switch " SUITE_MODE_NAME
               " -> " SUITE_NEXT_MODE_NAME);
    REQUIRE_SATP_MODE(SUITE_SATP_MODE);
    REQUIRE_SATP_MODE(SUITE_NEXT_MODE);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SUITE_SATP_MODE);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G, flags, PT_LEVEL_1G);

    /* Current-mode test */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT(SUITE_MODE_NAME " read/write succeeds", result == 0);

    /* Switch to the next higher mode */
    vm_switch_mode(&ctx, SUITE_NEXT_MODE);

    /* Next-mode test */
    result = vm_run_in_smode(&ctx, test_smode_read_write,
                              (uintptr_t)test_data_area);
    TEST_ASSERT(SUITE_NEXT_MODE_NAME " read/write succeeds after switch",
                result == 0);

    pt_pool_reset();
    TEST_END();
}
#endif /* SUITE_SATP_MODE != SATP_MODE_SV57 */

SV_REGISTER(satp07);
bool SVFN(satp07)(void) {
    TEST_BEGIN("SATP-07: ASID basic functionality");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SUITE_SATP_MODE);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test_data_area (in the separate VM test region) */
    uintptr_t data_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, data_va, data_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Set ASID to a non-zero value and verify it's retained */
    unsigned test_asid = 42;
    uintptr_t root_ppn = (uintptr_t)ctx.root_pt >> PAGE_SHIFT;
    uintptr_t satp_val = MAKE_SATP(SUITE_SATP_MODE, test_asid, root_ppn);
    CSRW(satp, satp_val);
    vm_sfence_vma(0, 0);

    uintptr_t readback = CSRR(satp);
    uintptr_t asid_readback = (readback >> SATP_ASID_SHIFT) & 0xFFFF;

    /* ASID field is WARL, so the value may be masked.
     * At minimum, if ASID is supported, the value should be retained.
     * If ASID width is 0, readback will be 0. */
    TEST_ASSERT("ASID is WARL (readback is 0 or matches)",
                asid_readback == 0 || asid_readback == test_asid);

    /* Verify VM still works with ASID set */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("VM works with ASID set", result == 0);

    pt_pool_reset();
    TEST_END();
}

SV_REGISTER(satp09);
bool SVFN(satp09)(void) {
    TEST_BEGIN("SATP-09: Unsupported MODE value (WARL behavior)");

    /*
     * Write an unsupported MODE value (e.g., MODE=15) to satp.
     * Since satp.MODE is WARL, the hardware should either:
     *   - Ignore the write (retain previous value), or
     *   - Set MODE to a supported value (e.g., Bare)
     * The key invariant: the readback MODE must be a supported value.
     */
    uintptr_t satp_before = CSRR(satp);

    /* Try writing MODE=15 (unsupported) */
    uintptr_t bad_satp = (15UL << SATP_MODE_SHIFT);
    CSRW(satp, bad_satp);

    uintptr_t satp_after = CSRR(satp);
    uintptr_t mode_after = satp_after >> SATP_MODE_SHIFT;

    /* MODE must be a supported value */
    bool mode_valid = (mode_after == SATP_MODE_BARE ||
                       mode_after == SATP_MODE_SV39 ||
                       mode_after == SATP_MODE_SV48 ||
                       mode_after == SATP_MODE_SV57);
    TEST_ASSERT("unsupported MODE: readback is valid", mode_valid);

    /* Restore previous satp */
    CSRW(satp, satp_before);
    vm_sfence_vma(0, 0);

    TEST_END();
}
