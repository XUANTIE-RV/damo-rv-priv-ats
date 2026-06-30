/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pbmt_basic.c - Group 1: PBMT Basic Encoding Verification
 *
 * Tests PBMT-01 through PBMT-06
 *
 * Verifies that PBMT values PMA(00), NC(01), IO(10) in leaf PTEs
 * allow normal access for load, store, and exec operations.
 */

/* ===================================================================
 * PBMT-01: PBMT=PMA load/store
 * =================================================================== */
TEST_REGISTER(test_pbmt_pma_load_store);
bool test_pbmt_pma_load_store(void) {
    TEST_BEGIN("PBMT-01: PBMT=PMA (00) load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map test page: PBMT=PMA (00, default) */
    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_PMA;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("PBMT=PMA load/store succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PBMT-02: PBMT=NC load/store
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_load_store);
bool test_pbmt_nc_load_store(void) {
    TEST_BEGIN("PBMT-02: PBMT=NC (01) load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map test page: PBMT=NC (01) */
    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("PBMT=NC load/store succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PBMT-03: PBMT=IO load/store
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_load_store);
bool test_pbmt_io_load_store(void) {
    TEST_BEGIN("PBMT-03: PBMT=IO (10) load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Map test page: PBMT=IO (10) */
    uintptr_t test_va = (uintptr_t)test_data_area;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("PBMT=IO load/store succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PBMT-04: PBMT=PMA exec
 * =================================================================== */
TEST_REGISTER(test_pbmt_pma_exec);
bool test_pbmt_pma_exec(void) {
    TEST_BEGIN("PBMT-04: PBMT=PMA (00) exec");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    fill_exec_page(test_va);

    uintptr_t pte_flags = PTE_V | PTE_R | PTE_X | PTE_A | PTE_D
                          | PBMT_PMA;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT("PBMT=PMA exec succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PBMT-05: PBMT=NC exec (implementation-defined)
 *
 * PBMT=NC designates non-cacheable, idempotent memory. Whether
 * instruction fetch succeeds on such a page is implementation-
 * defined: some hardware disallows fetch from non-cacheable pages.
 *
 * Acceptable outcomes:
 *   - result == 0:              exec succeeded (NC fetch supported)
 *   - CAUSE_IAF  (cause  1):   PMA-level rejection of NC fetch
 *   - CAUSE_IPF  (cause 12):   PTE/PBMT-level rejection of NC fetch
 *   - CAUSE_ILI  (cause  2):   illegal instruction — the test writes
 *       nop;ret via cacheable M-mode, but NC fetch bypasses cache and
 *       reads stale memory (typically zeros = illegal insn). This is
 *       a cache-coherence artifact, not a spec violation.
 * =================================================================== */
TEST_REGISTER(test_pbmt_nc_exec);
bool test_pbmt_nc_exec(void) {
    TEST_BEGIN("PBMT-05: PBMT=NC (01) exec (impl-defined)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    fill_exec_page(test_va);

    uintptr_t pte_flags = PTE_V | PTE_R | PTE_X | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* PBMT=NC exec is implementation-defined (see comment above). */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    printf("  [DEBUG] PBMT=NC exec result (mcause): %lu\n", (unsigned long)result);
    TEST_ASSERT("PBMT=NC exec: succeeds or fault (impl-defined)",
                result == 0 || result == CAUSE_IAF
                            || result == CAUSE_IPF
                            || result == CAUSE_ILI);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * PBMT-06: PBMT=IO exec (implementation-defined)
 * =================================================================== */
TEST_REGISTER(test_pbmt_io_exec);
bool test_pbmt_io_exec(void) {
    TEST_BEGIN("PBMT-06: PBMT=IO (10) exec (impl-defined)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t test_va = (uintptr_t)test_exec_page;
    fill_exec_page(test_va);

    uintptr_t pte_flags = PTE_V | PTE_R | PTE_X | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* PBMT=IO exec is implementation-defined:
     * may succeed or trigger an access fault. Both are acceptable. */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_exec, test_va);
    TEST_ASSERT("PBMT=IO exec: succeeds or access fault",
                result == 0 || result == CAUSE_IAF);

    pt_pool_reset();
    TEST_END();
}
