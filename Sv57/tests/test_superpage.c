/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_superpage.c - Group 8: Superpage Alignment (ALIGN-01~02)
 *                  + Group 9: Multi-level Page Table Walk (WALK-03/04/05)
 *
 * Tests:
 *   ALIGN-01: Misaligned 2MB megapage triggers page fault
 *   ALIGN-02: Misaligned 1GB gigapage triggers page fault
 *   WALK-03:  Sv57 five-level full walk (4KB pages)
 *   WALK-04:  Non-leaf PTE at last level triggers page fault
 *   WALK-05:  Intermediate leaf PTE (2MB megapage) works
 */

/* ===================================================================
 * Group 8: Superpage Alignment (ALIGN-01 ~ ALIGN-02)
 * =================================================================== */

TEST_REGISTER(test_sv57_align01);
bool test_sv57_align01(void) {
    TEST_BEGIN("ALIGN-01: Misaligned 2MB megapage triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /*
     * Construct a misaligned megapage:
     * Use a VA outside the 1GB code mapping region.
     * Map with PA that has ppn[0] != 0 (PA not 2MB-aligned).
     */
    uintptr_t test_va_2m = 0x40000000UL;  /* 1GB mark, different from code region */
    uintptr_t test_pa_misaligned = test_va_2m + PAGE_SIZE_4K;  /* ppn[0] != 0 */

    pt_map_page(&ctx, test_va_2m, test_pa_misaligned,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va_2m);
    TEST_ASSERT("misaligned 2MB megapage triggers page fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_align02);
bool test_sv57_align02(void) {
    TEST_BEGIN("ALIGN-02: Misaligned 1GB gigapage triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /*
     * Construct a misaligned gigapage:
     * Install a leaf PTE at gigapage level with ppn[1:0] != 0.
     * Use a VA in a different 1GB region than the code mapping.
     */
    uintptr_t test_va_1g = 0x40000000UL;  /* 1GB, different region */
    uintptr_t test_pa_misaligned = test_va_1g + PAGE_SIZE_2M;  /* ppn[0] != 0 */

    pt_map_page(&ctx, test_va_1g, test_pa_misaligned,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_1G);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va_1g);
    TEST_ASSERT("misaligned 1GB gigapage triggers page fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * Group 9: Multi-level Page Table Walk (WALK-03, WALK-04, WALK-05)
 * =================================================================== */

TEST_REGISTER(test_sv57_walk03);
bool test_sv57_walk03(void) {
    TEST_BEGIN("WALK-03: Sv57 five-level full walk (4KB pages)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);

    uintptr_t base = PLATFORM_MEM_BASE;
    uintptr_t size = 2 * PAGE_SIZE_2M;
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, size,
                                        flags, PT_LEVEL_4K);
    TEST_ASSERT("4KB identity mapping setup", ret == 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("five-level walk read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_walk04);
bool test_sv57_walk04(void) {
    TEST_BEGIN("WALK-04: Non-leaf PTE at last level triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /*
     * Install a non-leaf PTE (V=1, R=0, X=0) at L0 level.
     * The translation algorithm will try i=i-1, making i<0,
     * which triggers a page-fault.
     */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V,  /* non-leaf: V=1, R=0, W=0, X=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("non-leaf at L0 triggers page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

TEST_REGISTER(test_sv57_walk05);
bool test_sv57_walk05(void) {
    TEST_BEGIN("WALK-05: Intermediate leaf PTE (2MB megapage) works");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);

    uintptr_t base = PLATFORM_MEM_BASE;
    uintptr_t size = 16 * PAGE_SIZE_2M;
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, size,
                                        flags, PT_LEVEL_2M);
    TEST_ASSERT("2MB identity mapping setup", ret == 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("L1 leaf PTE (megapage) read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
