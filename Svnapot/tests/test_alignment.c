/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_alignment.c - Group 13: NAPOT Region Boundary Alignment
 *
 * Tests ALIGN-01 through ALIGN-02
 *
 * Verifies NAPOT PTE translation behavior with respect to
 * PPN substitution semantics when accessing at various offsets.
 */

/* ===================================================================
 * ALIGN-01: VA non-64KiB-aligned access within NAPOT region
 *
 * NAPOT PTE installed at 64 KiB aligned base, accessing at
 * offset 0x5000 should correctly translate via PPN substitution.
 * =================================================================== */
TEST_REGISTER(test_napot_unaligned_va);
bool test_napot_unaligned_va(void) {
    TEST_BEGIN("ALIGN-01: NAPOT PTE with non-64KiB-aligned VA access");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Install NAPOT PTE at 64 KiB aligned base */
    uintptr_t napot_base = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(napot_base,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, napot_base, napot_pte);

    /* Access at offset 0x5000 within the NAPOT region.
     * vpn[0] low 4 bits = 5 -> PA offset = 0x5000 via PPN substitution */
    uintptr_t test_va = napot_base + 0x5000;
    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, test_va);
    TEST_ASSERT("non-aligned VA within NAPOT region translates correctly",
                result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ALIGN-02: PA non-64KiB-aligned NAPOT translation
 *
 * The NAPOT PTE's ppn[0] low 4 bits are the encoding (1000), and
 * during translation these bits are replaced by vpn[0] low bits.
 * This test verifies that accessing different offsets within the
 * region correctly maps to different physical pages within the
 * PA's 64 KiB aligned region.
 * =================================================================== */
TEST_REGISTER(test_napot_pa_alignment);
bool test_napot_pa_alignment(void) {
    TEST_BEGIN("ALIGN-02: NAPOT PA alignment and PPN substitution");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Use identity mapping: VA = PA = NAPOT_TEST_REGION_0 */
    uintptr_t napot_base = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(napot_base,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, napot_base, napot_pte);

    /* Verify multiple boundary offsets translate correctly */
    uintptr_t offsets[] = {0x0000, 0x1000, 0x7000, 0x8000, 0xF000};
    int n = sizeof(offsets) / sizeof(offsets[0]);
    bool all_pass = true;

    for (int i = 0; i < n; i++) {
        uintptr_t addr = napot_base + offsets[i];
        uintptr_t result = vm_run_in_smode(&ctx, smode_read_write, addr);
        if (result != 0) {
            printf("  offset 0x%lx failed: result=0x%lx\n",
                   (unsigned long)offsets[i], (unsigned long)result);
            all_pass = false;
        }
    }
    TEST_ASSERT("all boundary offsets translate correctly", all_pass);

    pt_pool_reset();
    TEST_END();
}
