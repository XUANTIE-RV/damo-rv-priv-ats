/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_ppn_subst.c - Group 2: PPN Substitution Semantics Verification
 *
 * Tests PPN-01 through PPN-05
 *
 * Verifies that NAPOT PTE address translation correctly substitutes
 * ppn[i][napot_bits-1:0] with vpn[i][napot_bits-1:0].
 */

/* ===================================================================
 * PPN-01: PPN substitution basic verification
 * =================================================================== */
TEST_REGISTER(test_ppn_subst_basic);
bool test_ppn_subst_basic(void) {
    TEST_BEGIN("PPN-01: PPN substitution within 64 KiB NAPOT region");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* 64 KiB NAPOT identity mapping */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Verify each 4 KiB page maps to distinct physical location */
    uintptr_t result = vm_run_in_smode(&ctx, smode_ppn_subst_verify,
                                        test_va);

    if (result != 0) {
        unsigned phase = (result >> 8) & 0xF;
        unsigned page_idx = result & 0xFF;
        const char *phase_name =
            (phase == 1) ? "write" :
            (phase == 2) ? "readback" :
            (phase == 3) ? "verify" : "unknown";
        printf("  [DIAG] PPN subst failed: phase=%s, page_idx=%u (offset=0x%x)\n",
               phase_name, page_idx, page_idx * 0x1000);
        printf("  [DIAG] This may indicate the hardware does not correctly\n"
               "         substitute ppn[0][3:0] with vpn[0][3:0] for NAPOT PTEs.\n");
    }

    TEST_ASSERT("PPN substitution correct for all 16 pages", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PPN-02: Region offset 0x0 translation verification
 * =================================================================== */
TEST_REGISTER(test_ppn_subst_offset_0);
bool test_ppn_subst_offset_0(void) {
    TEST_BEGIN("PPN-02: NAPOT region offset 0x0 translation");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write,
                                        test_va + 0x0);
    TEST_ASSERT("offset 0x0 translates correctly", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PPN-03: Region offset 0x4000 translation verification
 * =================================================================== */
TEST_REGISTER(test_ppn_subst_offset_4000);
bool test_ppn_subst_offset_4000(void) {
    TEST_BEGIN("PPN-03: NAPOT region offset 0x4000 translation");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write,
                                        test_va + 0x4000);
    TEST_ASSERT("offset 0x4000 translates correctly", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PPN-04: Region offset 0xF000 translation verification
 * =================================================================== */
TEST_REGISTER(test_ppn_subst_offset_F000);
bool test_ppn_subst_offset_F000(void) {
    TEST_BEGIN("PPN-04: NAPOT region offset 0xF000 translation");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t napot_pte = napot_make_pte(test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, smode_read_write,
                                        test_va + 0xF000);
    TEST_ASSERT("offset 0xF000 translates correctly", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PPN-05: Non-identity mapping PPN substitution
 *
 * Map VA = NAPOT_TEST_REGION_0 to PA = NAPOT_TEST_REGION_1
 * (different VA and PA), verify translation at each offset.
 * =================================================================== */
TEST_REGISTER(test_ppn_subst_non_identity);
bool test_ppn_subst_non_identity(void) {
    TEST_BEGIN("PPN-05: Non-identity NAPOT mapping PPN substitution");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map VA = region0 to PA = region1 */
    uintptr_t test_va = NAPOT_TEST_REGION_0;
    uintptr_t test_pa = NAPOT_TEST_REGION_1;
    uintptr_t napot_pte = napot_make_pte(test_pa,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* Write via VA, then verify via direct PA access after VM off */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);

    if (result != 0) {
        printf("  [DIAG] non-identity NAPOT write failed: cause=0x%lx\n",
               (unsigned long)result);
        printf("  [DIAG] VA=0x%lx -> PA=0x%lx (NAPOT 64KiB)\n",
               (unsigned long)test_va, (unsigned long)test_pa);
    }
    TEST_ASSERT("non-identity NAPOT write succeeds", result == 0);

    /* Check physical memory at PA */
    volatile uintptr_t *pa_ptr = (volatile uintptr_t *)test_pa;
    if (*pa_ptr != MAGIC_WRITE) {
        printf("  [DIAG] PA data mismatch: expected=0x%lx, got=0x%lx\n",
               (unsigned long)MAGIC_WRITE, (unsigned long)*pa_ptr);
        /* Also check if data landed at VA (identity) instead of PA */
        volatile uintptr_t *va_ptr = (volatile uintptr_t *)test_va;
        if (*va_ptr == MAGIC_WRITE) {
            printf("  [DIAG] Data found at VA(0x%lx) instead of PA(0x%lx)\n",
                   (unsigned long)test_va, (unsigned long)test_pa);
            printf("  [DIAG] -> Hardware likely ignores NAPOT PPN and uses"
                   " VA-based PPN directly.\n");
        }
    }
    TEST_ASSERT("data appears at correct PA",
                *pa_ptr == MAGIC_WRITE);

    pt_pool_reset();
    TEST_END();
}
