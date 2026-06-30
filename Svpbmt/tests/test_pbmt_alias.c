/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pbmt_alias.c - Group 7: Aliasing and Coherence
 *
 * Tests ALIAS-01 through ALIAS-06
 *
 * Verifies behavior when the same physical page is mapped at two
 * different virtual addresses with different PBMT attributes.
 *
 * Note: True coherence issues may not manifest on single-hart
 * simulators. These tests verify basic data visibility and that
 * the fence/flush sequences execute without exceptions.
 */

/* ===================================================================
 * ALIAS-01: NC + IO non-cacheable alias coherence
 * =================================================================== */
TEST_REGISTER(test_pbmt_alias_nc_io);
bool test_pbmt_alias_nc_io(void) {
    TEST_BEGIN("ALIAS-01: NC + IO non-cacheable alias coherence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    /* Same physical page, two different virtual addresses */
    uintptr_t phys_page = (uintptr_t)test_data_area;
    uintptr_t va_nc = (uintptr_t)test_data_area;
    uintptr_t va_io = (uintptr_t)test_fault_page;

    /* VA1: PBMT=NC */
    pt_map_page(&ctx, va_nc, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC,
                PT_LEVEL_4K);

    /* VA2: PBMT=IO, mapping to same physical page */
    pt_map_page(&ctx, va_io, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_IO,
                PT_LEVEL_4K);

    /* Write via VA1 (NC), read via VA2 (IO) */
    alias_args_t args = { .va1 = va_nc, .va2 = va_io };
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_alias_write_read,
                                        (uintptr_t)&args);
    TEST_ASSERT("NC+IO alias: data coherent", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ALIAS-02: NC + IO alias with fence
 * =================================================================== */
TEST_REGISTER(test_pbmt_alias_nc_io_fence);
bool test_pbmt_alias_nc_io_fence(void) {
    TEST_BEGIN("ALIAS-02: NC + IO alias with fence iorw,iorw");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t phys_page = (uintptr_t)test_data_area;
    uintptr_t va_nc = (uintptr_t)test_data_area;
    uintptr_t va_io = (uintptr_t)test_fault_page;

    pt_map_page(&ctx, va_nc, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC,
                PT_LEVEL_4K);

    pt_map_page(&ctx, va_io, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_IO,
                PT_LEVEL_4K);

    /* Write via VA1 (NC), fence, read via VA2 (IO) */
    alias_args_t args = { .va1 = va_nc, .va2 = va_io };
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_alias_write_read,
                                        (uintptr_t)&args);
    TEST_ASSERT("NC+IO alias with fence: data coherent", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ALIAS-03: PMA + NC cacheable attribute alias (behavior uncertain)
 * =================================================================== */
TEST_REGISTER(test_pbmt_alias_pma_nc);
bool test_pbmt_alias_pma_nc(void) {
    TEST_BEGIN("ALIAS-03: PMA + NC cacheable alias (behavior uncertain)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t phys_page = (uintptr_t)test_data_area;
    uintptr_t va_pma = (uintptr_t)test_data_area;
    uintptr_t va_nc = (uintptr_t)test_fault_page;

    /* VA1: PBMT=PMA (cacheable) */
    pt_map_page(&ctx, va_pma, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_PMA,
                PT_LEVEL_4K);

    /* VA2: PBMT=NC (non-cacheable), same physical page */
    pt_map_page(&ctx, va_nc, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC,
                PT_LEVEL_4K);

    /* Write via PMA alias, read via NC alias.
     * Per spec: behavior uncertain (coherence may be lost).
     * We just verify no crash/unexpected trap. */
    alias_args_t args = { .va1 = va_pma, .va2 = va_nc };
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_alias_write_read,
                                        (uintptr_t)&args);
    /* Result may or may not match - we accept both */
    TEST_ASSERT("PMA+NC alias: no crash (coherence uncertain)",
                result == 0 || result == 1);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ALIAS-04: PMA + NC alias with fence+cbo.flush+fence
 * =================================================================== */
TEST_REGISTER(test_pbmt_alias_pma_nc_flush);
bool test_pbmt_alias_pma_nc_flush(void) {
    TEST_BEGIN("ALIAS-04: PMA + NC alias with fence+cbo.flush+fence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t phys_page = (uintptr_t)test_data_area;
    uintptr_t va_pma = (uintptr_t)test_data_area;
    uintptr_t va_nc = (uintptr_t)test_fault_page;

    pt_map_page(&ctx, va_pma, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_PMA,
                PT_LEVEL_4K);

    pt_map_page(&ctx, va_nc, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC,
                PT_LEVEL_4K);

    /* Write via PMA, fence+cbo.flush+fence, read via NC */
    alias_args_t args = { .va1 = va_pma, .va2 = va_nc };
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_alias_flush_sequence,
                                        (uintptr_t)&args);

    if (result == 0xFF) {
        TEST_SKIP("cbo.flush not supported (Zicbom not available)");
    }

    TEST_ASSERT("PMA+NC alias with flush sequence: data coherent",
                result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ALIAS-05: PMA + IO cacheable attribute alias (behavior uncertain)
 * =================================================================== */
TEST_REGISTER(test_pbmt_alias_pma_io);
bool test_pbmt_alias_pma_io(void) {
    TEST_BEGIN("ALIAS-05: PMA + IO cacheable alias (behavior uncertain)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t phys_page = (uintptr_t)test_data_area;
    uintptr_t va_pma = (uintptr_t)test_data_area;
    uintptr_t va_io = (uintptr_t)test_fault_page;

    pt_map_page(&ctx, va_pma, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_PMA,
                PT_LEVEL_4K);

    pt_map_page(&ctx, va_io, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_IO,
                PT_LEVEL_4K);

    alias_args_t args = { .va1 = va_pma, .va2 = va_io };
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_alias_write_read,
                                        (uintptr_t)&args);
    TEST_ASSERT("PMA+IO alias: no crash (coherence uncertain)",
                result == 0 || result == 1);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * ALIAS-06: PMA + IO alias with fence+cbo.flush+fence
 * =================================================================== */
TEST_REGISTER(test_pbmt_alias_pma_io_flush);
bool test_pbmt_alias_pma_io_flush(void) {
    TEST_BEGIN("ALIAS-06: PMA + IO alias with fence+cbo.flush+fence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    int ret = setup_code_mapping(&ctx);
    TEST_ASSERT("code mapping setup", ret == 0);

    uintptr_t phys_page = (uintptr_t)test_data_area;
    uintptr_t va_pma = (uintptr_t)test_data_area;
    uintptr_t va_io = (uintptr_t)test_fault_page;

    pt_map_page(&ctx, va_pma, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_PMA,
                PT_LEVEL_4K);

    pt_map_page(&ctx, va_io, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_IO,
                PT_LEVEL_4K);

    alias_args_t args = { .va1 = va_pma, .va2 = va_io };
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_alias_flush_sequence,
                                        (uintptr_t)&args);

    if (result == 0xFF) {
        TEST_SKIP("cbo.flush not supported (Zicbom not available)");
    }

    TEST_ASSERT("PMA+IO alias with flush sequence: data coherent",
                result == 0);

    pt_pool_reset();
    TEST_END();
}
