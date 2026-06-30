/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_pagefault.c - Group 1: Page-Fault address-type exceptions
 *
 * Per Sstvala (SPEC/sstvala.adoc:4-6):
 *   For load/store/instruction page-fault, stval must contain the
 *   faulting virtual address.
 *
 * Tests:
 *   TVAL-LPF-01: load page-fault on unmapped VA
 *   TVAL-LPF-02: load on valid mapping (no fault, reverse check)
 *   TVAL-SPF-01: store page-fault on read-only page
 *   TVAL-SPF-02: store page-fault on unmapped VA
 *   TVAL-IPF-01: instruction page-fault on unmapped VA fetch
 *   TVAL-IPF-02: instruction page-fault on non-executable page fetch
 *   TVAL-LPF-03: load page-fault on non-canonical VA
 */

/* Unmapped VA used by TVAL-LPF-01 / TVAL-SPF-02 */
#define UNMAPPED_VA       0x40000000UL

/* Read-only page VA used by TVAL-SPF-01 */
#define READONLY_VA       ((uintptr_t)test_fault_page)

/* Non-canonical VA for Sv39 (bit 38 set but bits 63:39 are zero) */
#define NONCANONICAL_VA   0x4000000000UL

/* ===================================================================
 * TVAL-LPF-01: load page-fault on unmapped VA
 * =================================================================== */
TEST_REGISTER(test_sstvala_lpf_01);
bool test_sstvala_lpf_01(void) {
    TEST_BEGIN("TVAL-LPF-01: load page-fault stval == unmapped VA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);
    /* UNMAPPED_VA is deliberately not mapped */

    uintptr_t result = vm_run_in_smode(&ctx, smode_load_addr, UNMAPPED_VA);
    TEST_ASSERT_EQ("cause == load page-fault",
                   result, CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT_EQ("stval == faulting VA",
                   trap_get_tval(), UNMAPPED_VA);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * TVAL-LPF-02: load on valid mapping succeeds (reverse check)
 * =================================================================== */
TEST_REGISTER(test_sstvala_lpf_02);
bool test_sstvala_lpf_02(void) {
    TEST_BEGIN("TVAL-LPF-02: load on valid RW page succeeds (no fault)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map test_fault_page with full permissions */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, smode_load_addr, test_va);
    TEST_ASSERT_EQ("no fault on valid page", result, 0);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * TVAL-SPF-01: store page-fault on read-only page
 * =================================================================== */
TEST_REGISTER(test_sstvala_spf_01);
bool test_sstvala_spf_01(void) {
    TEST_BEGIN("TVAL-SPF-01: store page-fault on R-only page, stval == VA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* Map read-only page: V=1, R=1, W=0 */
    uintptr_t test_va = READONLY_VA;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, smode_store_addr, test_va);
    TEST_ASSERT_EQ("cause == store page-fault",
                   result, CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT_EQ("stval == faulting VA",
                   trap_get_tval(), test_va);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * TVAL-SPF-02: store page-fault on unmapped VA
 * =================================================================== */
TEST_REGISTER(test_sstvala_spf_02);
bool test_sstvala_spf_02(void) {
    TEST_BEGIN("TVAL-SPF-02: store page-fault on unmapped VA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);
    /* UNMAPPED_VA is deliberately not mapped */

    uintptr_t result = vm_run_in_smode(&ctx, smode_store_addr, UNMAPPED_VA);
    TEST_ASSERT_EQ("cause == store page-fault",
                   result, CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT_EQ("stval == faulting VA",
                   trap_get_tval(), UNMAPPED_VA);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * TVAL-IPF-01: instruction page-fault on unmapped VA fetch
 * =================================================================== */
TEST_REGISTER(test_sstvala_ipf_01);
bool test_sstvala_ipf_01(void) {
    TEST_BEGIN("TVAL-IPF-01: instruction page-fault on unmapped VA fetch");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);
    /* UNMAPPED_VA is not mapped -> fetch triggers inst page-fault */

    uintptr_t result = vm_run_in_smode(&ctx, smode_exec_addr, UNMAPPED_VA);
    TEST_ASSERT_EQ("cause == instruction page-fault",
                   result, CAUSE_INST_PAGE_FAULT);
    TEST_ASSERT_EQ("stval == faulting VA",
                   trap_get_tval(), UNMAPPED_VA);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * TVAL-IPF-02: instruction page-fault on non-executable page fetch
 * =================================================================== */
TEST_REGISTER(test_sstvala_ipf_02);
bool test_sstvala_ipf_02(void) {
    TEST_BEGIN("TVAL-IPF-02: instruction page-fault on non-executable page");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);
    init_exec_page();

    /* Map page with R but no X permission */
    uintptr_t test_va = (uintptr_t)test_exec_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,  /* X=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, smode_exec_addr, test_va);
    TEST_ASSERT_EQ("cause == instruction page-fault",
                   result, CAUSE_INST_PAGE_FAULT);
    TEST_ASSERT_EQ("stval == faulting VA",
                   trap_get_tval(), test_va);

    pt_pool_reset();
    TEST_END();
}

/* ===================================================================
 * TVAL-LPF-03: load page-fault on non-canonical VA (Sv39)
 *
 * In Sv39, a valid (canonical) VA has bits [63:39] all-zero or all-one.
 * An address like 0x4000000000 has bit 38 set but bits 63:39 are zero,
 * making it non-canonical. Accessing it should trigger a page-fault
 * with stval set to the faulting address.
 * =================================================================== */
TEST_REGISTER(test_sstvala_lpf_03);
bool test_sstvala_lpf_03(void) {
    TEST_BEGIN("TVAL-LPF-03: load page-fault on non-canonical VA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t result = vm_run_in_smode(&ctx, smode_load_addr, NONCANONICAL_VA);
    TEST_ASSERT_EQ("cause == load page-fault",
                   result, CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT_EQ("stval == non-canonical VA",
                   trap_get_tval(), NONCANONICAL_VA);

    pt_pool_reset();
    TEST_END();
}
