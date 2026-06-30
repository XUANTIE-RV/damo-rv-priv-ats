/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pbmt_pagesz.c - Group 5: PBMT and Different Page Sizes
 *
 * Tests PGSZ-01 through PGSZ-06
 *
 * Verifies PBMT attributes work correctly on 4KB, 2MB, and 1GB pages.
 */

/* ===================================================================
 * PGSZ-01: 4KB page + PBMT=NC
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_4kb);
bool test_pbmt_nc_4kb(void) {
    TEST_BEGIN("PGSZ-01: 4KB page + PBMT=NC load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("4KB + PBMT=NC works", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PGSZ-02: 4KB page + PBMT=IO
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_4kb);
bool test_pbmt_io_4kb(void) {
    TEST_BEGIN("PGSZ-02: 4KB page + PBMT=IO load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("4KB + PBMT=IO works", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PGSZ-03: 2MB megapage + PBMT=NC
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_2mb_megapage);
bool test_pbmt_nc_2mb_megapage(void) {
    TEST_BEGIN("PGSZ-03: 2MB megapage + PBMT=NC load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map 2MB megapage test area: PBMT=NC */
    uintptr_t test_va = (uintptr_t)test_data_area & ~(PAGE_SIZE_2M - 1);
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("2MB megapage + PBMT=NC works", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PGSZ-04: 2MB megapage + PBMT=IO
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_2mb_megapage);
bool test_pbmt_io_2mb_megapage(void) {
    TEST_BEGIN("PGSZ-04: 2MB megapage + PBMT=IO load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_data_area & ~(PAGE_SIZE_2M - 1);
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("2MB megapage + PBMT=IO works", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PGSZ-05/06 helper: 1GB gigapage PBMT data-only test
 *
 * Design rationale:
 *   PBMT=IO designates a page as strongly-ordered, non-cacheable,
 *   non-idempotent I/O memory. Per the RISC-V Svpbmt spec,
 *   instruction fetch from an IO page is implementation-defined
 *   and is typically not supported (the implementation may raise
 *   an instruction access fault).
 *
 *   The previous implementation mapped the entire 1GB region
 *   containing code, stack and data with the test PBMT attribute,
 *   which fails on implementations that disallow fetch from IO
 *   pages (which is correct hardware behavior, not a bug).
 *
 *   Fix: keep code identity-mapped with PMA so the CPU can fetch
 *   normally, and add a SECOND 1GB gigapage at a high VA whose
 *   PA points back to the page containing test_data_area. Only
 *   data load/store is performed on that PBMT-tagged page; no
 *   instruction fetch occurs there.
 *
 *   This still validates the spec intent: the hardware must
 *   correctly decode the PBMT field at the level-2 (1GB) PTE.
 *
 * VA layout (Sv39, 39-bit VA space):
 *   [PLATFORM_MEM_BASE .. +1GB)  - code/data identity mapping (PMA)
 *   PBMT_TEST_GIGAPAGE_VA        - 1GB-aligned high VA
 *                                  PA = test_data_area's 1GB-aligned base
 *                                  PBMT = NC or IO (under test)
 * =================================================================== */

/* Pick a 1GB-aligned VA that does NOT overlap the code region.
 * Sv39 supports a 39-bit VA space (max ~256 GB usable per half).
 * 4 GB (0x100000000) is safely above any practical code/data
 * placement and is naturally 1 GB aligned. */
#define PBMT_TEST_GIGAPAGE_VA  0x100000000UL

static void run_1gb_gigapage_pbmt_test(uintptr_t pbmt_attr) {
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* Step 1: identity-map code/data region with PMA so the CPU
     * can fetch instructions normally during the S-mode test. */
    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping (PMA) setup", ret == 0);

    /* Step 2: add a separate 1GB gigapage at a distinct high VA,
     * with PA pointing to the 1GB-aligned base containing
     * test_data_area. Only data accesses target this VA. */
    uintptr_t data_pa_base = (uintptr_t)test_data_area
                             & ~(PAGE_SIZE_1G - 1);
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | pbmt_attr;
    ret = pt_map_page(&ctx, PBMT_TEST_GIGAPAGE_VA, data_pa_base,
                      pte_flags, PT_LEVEL_1G);
    TEST_ASSERT("1GB gigapage data mapping setup", ret == 0);

    /* Compute the VA inside the 1GB gigapage that aliases
     * test_data_area's PA. */
    uintptr_t data_offset = (uintptr_t)test_data_area - data_pa_base;

    /* Ensure test_data_area fits within the 1GB region.
     * test_smode_read_write accesses ptr[0] and ptr[1] (2 * uintptr_t). */
    TEST_ASSERT("test_data_area fits in 1GB region",
        data_offset + 2 * sizeof(uintptr_t) < PAGE_SIZE_1G);
    uintptr_t test_va = PBMT_TEST_GIGAPAGE_VA + data_offset;

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);

    if (pbmt_attr == PBMT_NC)
        TEST_ASSERT("1GB gigapage + PBMT=NC works", result == 0);
    else
        TEST_ASSERT("1GB gigapage + PBMT=IO works", result == 0);

    pt_pool_reset();
}

/* ===================================================================
 * PGSZ-05: 1GB gigapage + PBMT=NC (data-only access)
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_1gb_gigapage);
bool test_pbmt_nc_1gb_gigapage(void) {
    TEST_BEGIN("PGSZ-05: 1GB gigapage + PBMT=NC load/store");
    run_1gb_gigapage_pbmt_test(PBMT_NC);
    TEST_END();
}

/* ===================================================================
 * PGSZ-06: 1GB gigapage + PBMT=IO (data-only access)
 *
 * Note: instruction fetch is NOT performed on the IO-tagged page,
 * because PBMT=IO semantics forbid speculative/idempotent access
 * and most implementations (e.g. C908V2) reject instruction fetch
 * on such pages. Only load/store is exercised here.
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_1gb_gigapage);
bool test_pbmt_io_1gb_gigapage(void) {
    TEST_BEGIN("PGSZ-06: 1GB gigapage + PBMT=IO load/store");
    run_1gb_gigapage_pbmt_test(PBMT_IO);
    TEST_END();
}
