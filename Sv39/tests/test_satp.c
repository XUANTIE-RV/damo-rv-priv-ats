/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_satp.c - Group 11: satp CSR Control (SATP-01/02/05/07/09)
 *
 * Tests:
 *   SATP-01: MODE=Bare disables VM
 *   SATP-02: MODE=Sv39 enables VM
 *   SATP-05: Mode switch Sv39 -> Sv48
 *   SATP-07: ASID basic functionality
 *   SATP-09: Unsupported MODE value (WARL behavior)
 */

TEST_REGISTER(test_sv39_satp01);
bool test_sv39_satp01(void) {
    TEST_BEGIN("SATP-01: MODE=Bare disables VM");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
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

TEST_REGISTER(test_sv39_satp02);
bool test_sv39_satp02(void) {
    TEST_BEGIN("SATP-02: MODE=Sv39 enables VM");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test_data_area (in the separate VM test region) */
    uintptr_t data_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, data_va, data_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Enable VM and verify it works */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("Sv39 VM enabled, read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv39_satp05);
bool test_sv39_satp05(void) {
    TEST_BEGIN("SATP-05: Mode switch Sv39 -> Sv48");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G, flags, PT_LEVEL_1G);

    /* Sv39 test */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("Sv39 read/write succeeds", result == 0);

    /* Switch to Sv48 */
    vm_switch_mode(&ctx, SATP_MODE_SV48);

    /* Sv48 test */
    result = vm_run_in_smode(&ctx, test_smode_read_write,
                              (uintptr_t)test_data_area);
    TEST_ASSERT("Sv48 read/write succeeds after switch", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv39_satp07);
bool test_sv39_satp07(void) {
    TEST_BEGIN("SATP-07: ASID basic functionality");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test_data_area (in the separate VM test region) */
    uintptr_t data_va = (uintptr_t)test_data_area;
    pt_map_page(&ctx, data_va, data_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* Set ASID to a non-zero value and verify it's retained */
    unsigned test_asid = 42;
    uintptr_t root_ppn = (uintptr_t)ctx.root_pt >> PAGE_SHIFT;
    uintptr_t satp_val = MAKE_SATP(SATP_MODE_SV39, test_asid, root_ppn);
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

TEST_REGISTER(test_sv39_satp09);
bool test_sv39_satp09(void) {
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
