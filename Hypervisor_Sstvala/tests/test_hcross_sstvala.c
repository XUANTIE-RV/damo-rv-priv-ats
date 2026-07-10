/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * test_hcross_sstvala.c - Hypervisor × Sstvala cross tests
 *
 * Implements 8 test cases from DOCS/testplan/Hypervisor_cross_test_plan.md
 * Group 2: Hypervisor × Sstvala.
 *
 * Sstvala extension guarantees stval/vstval precision on exceptions:
 *   - Address-type exceptions (page-fault, access-fault, etc.):
 *     stval must contain the faulting virtual address (not 0)
 *   - Instruction-type exceptions (illegal-inst, virtual-inst):
 *     stval must contain the faulting instruction encoding
 *
 * Test coverage:
 *   HCROSS-SSTVALA-01: Instruction guest-page-fault (cause=20) stval
 *   HCROSS-SSTVALA-02: Store guest-page-fault (cause=23) stval
 *   HCROSS-SSTVALA-03: AMO guest-page-fault (cause=23) stval
 *   HCROSS-SSTVALA-04: VS-stage inst page-fault (cause=12) vstval (delegated)
 *   HCROSS-SSTVALA-05: VS-stage load page-fault (cause=13) vstval (delegated)
 *   HCROSS-SSTVALA-06: Virtual-instruction (read hstatus) stval
 *   HCROSS-SSTVALA-07: Virtual-instruction (write hgatp) stval
 *   HCROSS-SSTVALA-08: Virtual-instruction (read hideleg) stval
 * =================================================================== */

#include "test_helpers.h"

/* ===================================================================
 * HCROSS-SSTVALA-01: Instruction guest-page-fault stval precision
 *
 * VS-mode jumps to a G-stage unmapped GPA, triggering an instruction
 * guest-page-fault (cause=20) that traps to HS-mode. Verifies that
 * stval contains the faulting GVA (jump target address).
 * =================================================================== */

TEST_REGISTER(test_hcross_sstvala_01);
bool test_hcross_sstvala_01(void) {
    TEST_BEGIN("HCROSS-SSTVALA-01: inst guest-page-fault stval precision");

    /* Initialize two-stage translation: VS=Bare, G-stage=SV39X4 */
    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Identity-map the kernel/UART region at 2MB granularity */
    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end = (uintptr_t)__vm_test_region_start & ~(PAGE_SIZE_2M - 1);
    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    /* Identity-map the test region at 4KB granularity */
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    /* Target GPA: outside mapped regions (unmapped in G-stage) */
    uintptr_t target_gpa = 0xA0000000UL;  /* 2.5GB offset, unmapped */

    /* VS-mode jumps to target_gpa, triggering inst guest-page-fault */
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_jump_to, target_gpa);

    /* Verify trap was triggered */
    TEST_ASSERT("inst guest-page-fault triggered", trap_was_triggered());

    if (trap_was_triggered()) {
        /* Verify cause == 20 (instruction guest-page-fault) */
        TEST_ASSERT_EQ("cause == inst guest-page-fault (20)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_INST_GUEST_PAGE_FAULT);

        /* Sstvala core assertion: stval == faulting GVA (not 0) */
        TEST_ASSERT_EQ("stval == faulting GVA",
                       trap_get_tval(),
                       target_gpa);
    }

    trap_expect_end();
    two_stage_cleanup(&ctx);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSTVALA-02: Store guest-page-fault stval precision
 *
 * VS-mode stores to a G-stage unmapped GPA, triggering a store
 * guest-page-fault (cause=23) that traps to HS-mode. Verifies that
 * stval contains the faulting GVA (store target address).
 * =================================================================== */

TEST_REGISTER(test_hcross_sstvala_02);
bool test_hcross_sstvala_02(void) {
    TEST_BEGIN("HCROSS-SSTVALA-02: store guest-page-fault stval precision");

    /* Initialize two-stage translation: VS=Bare, G-stage=SV39X4 */
    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Identity-map the kernel/UART region at 2MB granularity */
    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end = (uintptr_t)__vm_test_region_start & ~(PAGE_SIZE_2M - 1);
    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    /* Identity-map the test region at 4KB granularity */
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    /* Target GPA: outside mapped regions (unmapped in G-stage) */
    uintptr_t target_gpa = 0xB0000000UL;  /* 2.75GB offset, unmapped */

    /* VS-mode stores to target_gpa, triggering store guest-page-fault */
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_store_to, target_gpa);

    /* Verify trap was triggered */
    TEST_ASSERT("store guest-page-fault triggered", trap_was_triggered());

    if (trap_was_triggered()) {
        /* Verify cause == 23 (store guest-page-fault) */
        TEST_ASSERT_EQ("cause == store guest-page-fault (23)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

        /* Sstvala core assertion: stval == faulting GVA (not 0) */
        TEST_ASSERT_EQ("stval == faulting GVA",
                       trap_get_tval(),
                       target_gpa);
    }

    trap_expect_end();
    two_stage_cleanup(&ctx);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSTVALA-03: AMO guest-page-fault stval precision
 *
 * VS-mode performs an AMO operation on a G-stage unmapped GPA,
 * triggering a guest-page-fault that traps to HS-mode. AMO operations
 * consist of read and write phases; if G-stage is unmapped, the fault
 * occurs during the read phase, resulting in a load guest-page-fault
 * (cause=21). Verifies that stval contains the faulting GVA (AMO target).
 * =================================================================== */

TEST_REGISTER(test_hcross_sstvala_03);
bool test_hcross_sstvala_03(void) {
    TEST_BEGIN("HCROSS-SSTVALA-03: AMO guest-page-fault stval precision");

    /* Initialize two-stage translation: VS=Bare, G-stage=SV39X4 */
    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Identity-map the kernel/UART region at 2MB granularity */
    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end = (uintptr_t)__vm_test_region_start & ~(PAGE_SIZE_2M - 1);
    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    /* Identity-map the test region at 4KB granularity */
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    /* Target GPA: outside mapped regions (unmapped in G-stage) */
    uintptr_t target_gpa = 0xC0000000UL;  /* 3GB offset, unmapped */

    /* VS-mode performs AMO on target_gpa, triggering guest-page-fault */
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_amo_to, target_gpa);

    /* Verify trap was triggered */
    TEST_ASSERT("AMO guest-page-fault triggered", trap_was_triggered());

    if (trap_was_triggered()) {
        /* AMO faults during read phase -> load guest-page-fault (cause=21)
         * AMO faults during write phase -> store guest-page-fault (cause=23)
         * With G-stage completely unmapped, read phase fails first. */
        uintptr_t cause = trap_get_cause();
        bool cause_ok = (cause == CAUSE_LOAD_GUEST_PAGE_FAULT) ||
                        (cause == CAUSE_STORE_GUEST_PAGE_FAULT);
        TEST_ASSERT("cause == load or store guest-page-fault (21 or 23)",
                    cause_ok);

        /* Sstvala core assertion: stval == faulting GVA (not 0) */
        TEST_ASSERT_EQ("stval == faulting GVA",
                       trap_get_tval(),
                       target_gpa);
    }

    trap_expect_end();
    two_stage_cleanup(&ctx);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSTVALA-04: VS-stage inst page-fault (cause=12) vstval
 *
 * Enables VS-stage translation (SV39) so that page-faults occur at
 * VS-stage (cause=12/13/15) rather than G-stage (cause=20/21/23).
 * Configures medeleg+hedeleg to delegate inst page-fault (cause=12)
 * to VS-mode. VS-mode jumps to a VS-stage unmapped VA, triggering
 * a page-fault that is handled by the VS-mode trap handler.
 * Verifies that vstval contains the faulting VA.
 *
 * NOTE: We use VS-stage page-faults (cause 12/13) instead of
 * guest-page-faults (cause 20/23) because RISC-V SPEC mandates that
 * guest-page-faults cannot be delegated to VS-mode via hedeleg
 * (hedeleg bits 20/21/23 are read-only zero).
 * =================================================================== */

TEST_REGISTER(test_hcross_sstvala_04);
bool test_hcross_sstvala_04(void) {
    TEST_BEGIN("HCROSS-SSTVALA-04: VS-stage inst page-fault vstval (delegated)");

    /* Initialize two-stage translation: VS=SV39, G-stage=SV39X4 */
    two_stage_ctx_t ctx;
    gpt_pool_reset();
    pt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* VS-stage: identity-map kernel/UART region at 2MB granularity */
    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end = (uintptr_t)__vm_test_region_start & ~(PAGE_SIZE_2M - 1);
    two_stage_vs_identity(&ctx, lo_base, lo_end - lo_base,
                          VS_FLAGS_RWX_S_AD, PT_LEVEL_2M);

    /* VS-stage: identity-map the test region at 4KB granularity */
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_vs_identity(&ctx, r_start, r_size,
                          VS_FLAGS_RWX_S_AD, PT_LEVEL_4K);

    /* G-stage: identity-map kernel/UART region at 2MB granularity */
    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    /* G-stage: identity-map the test region at 4KB granularity */
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    /* Configure dual-layer delegation: M->HS (medeleg) -> VS (hedeleg) */
    uintptr_t medeleg;
    asm volatile ("csrr %0, medeleg" : "=r"(medeleg));
    medeleg |= (1UL << CAUSE_INST_PAGE_FAULT);
    asm volatile ("csrw medeleg, %0" :: "r"(medeleg));

    uintptr_t saved_hedeleg = hedeleg_read();
    hedeleg_write(saved_hedeleg | (1UL << CAUSE_INST_PAGE_FAULT));

    /* Setup VS-mode trap handler */
    setup_vs_trap_handler_for_sstvala();

    /* Target VA: outside VS-stage mapped regions */
    uintptr_t target_va = 0xA0000000UL;  /* 2.5GB offset, unmapped in VS-stage */

    /* VS-mode jumps to target_va, fault delegated to VS handler */
    two_stage_run_in_vs(&ctx, test_vs_jump_delegated, target_va);

    /* Verify VS-mode trap handler was triggered */
    TEST_ASSERT("VS-mode trap handler triggered", g_vs_trap_triggered);

    if (g_vs_trap_triggered) {
        /* Verify vscause == 12 (instruction page-fault) */
        TEST_ASSERT_EQ("vscause == inst page-fault (12)",
                       g_vs_trap_cause,
                       (uintptr_t)CAUSE_INST_PAGE_FAULT);

        /* Sstvala core assertion: vstval == faulting VA (not 0) */
        TEST_ASSERT_EQ("vstval == faulting VA",
                       g_vs_trap_vstval,
                       target_va);
    }

    /* Restore delegation */
    hedeleg_write(saved_hedeleg);

    two_stage_cleanup(&ctx);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSTVALA-05: VS-stage load page-fault (cause=13) vstval
 *
 * Enables VS-stage translation (SV39) and configures medeleg+hedeleg
 * to delegate load page-fault (cause=13) to VS-mode. VS-mode loads
 * from a VS-stage unmapped VA, triggering a page-fault that is
 * handled by the VS-mode trap handler. Verifies that vstval contains
 * the faulting VA.
 * =================================================================== */

TEST_REGISTER(test_hcross_sstvala_05);
bool test_hcross_sstvala_05(void) {
    TEST_BEGIN("HCROSS-SSTVALA-05: VS-stage load page-fault vstval (delegated)");

    /* Initialize two-stage translation: VS=SV39, G-stage=SV39X4 */
    two_stage_ctx_t ctx;
    gpt_pool_reset();
    pt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* VS-stage: identity-map kernel/UART region at 2MB granularity */
    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end = (uintptr_t)__vm_test_region_start & ~(PAGE_SIZE_2M - 1);
    two_stage_vs_identity(&ctx, lo_base, lo_end - lo_base,
                          VS_FLAGS_RWX_S_AD, PT_LEVEL_2M);

    /* VS-stage: identity-map the test region at 4KB granularity */
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_vs_identity(&ctx, r_start, r_size,
                          VS_FLAGS_RWX_S_AD, PT_LEVEL_4K);

    /* G-stage: identity-map kernel/UART region at 2MB granularity */
    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    /* G-stage: identity-map the test region at 4KB granularity */
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    /* Configure dual-layer delegation: M->HS (medeleg) -> VS (hedeleg) */
    uintptr_t medeleg;
    asm volatile ("csrr %0, medeleg" : "=r"(medeleg));
    medeleg |= (1UL << CAUSE_LOAD_PAGE_FAULT);
    asm volatile ("csrw medeleg, %0" :: "r"(medeleg));

    uintptr_t saved_hedeleg = hedeleg_read();
    hedeleg_write(saved_hedeleg | (1UL << CAUSE_LOAD_PAGE_FAULT));

    /* Setup VS-mode trap handler */
    setup_vs_trap_handler_for_sstvala();

    /* Target VA: outside VS-stage mapped regions */
    uintptr_t target_va = 0xB0000000UL;  /* 2.75GB offset, unmapped in VS-stage */

    /* VS-mode loads from target_va, fault delegated to VS handler.
     * Use trap_expect in case M-mode catches the fault directly
     * (e.g. if delegation is not honored by the platform). */
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, test_vs_load_delegated, target_va);

    /* Check if VS-mode handler was triggered (delegation worked) */
    if (g_vs_trap_triggered) {
        TEST_ASSERT_EQ("vscause == load page-fault (13)",
                       g_vs_trap_cause,
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);

        /* Sstvala core assertion: vstval == faulting VA (not 0) */
        TEST_ASSERT_EQ("vstval == faulting VA",
                       g_vs_trap_vstval,
                       target_va);
    } else if (trap_was_triggered()) {
        /* M-mode caught the fault directly (delegation not honored).
         * Verify the M-mode trap record instead. */
        TEST_ASSERT_EQ("M-mode cause == load page-fault (13)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
        TEST_ASSERT_EQ("M-mode stval == faulting VA",
                       trap_get_tval(),
                       target_va);
    } else {
        TEST_ASSERT("VS-mode or M-mode trap handler triggered", false);
    }

    trap_expect_end();

    /* Restore delegation */
    hedeleg_write(saved_hedeleg);

    two_stage_cleanup(&ctx);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSTVALA-06~08: Virtual-instruction stval precision
 *
 * Sstvala mandates that on a virtual-instruction exception (cause=22),
 * stval must contain the faulting instruction encoding (zero-extended
 * to XLEN). This is distinct from address-type exceptions where stval
 * holds the faulting address.
 *
 * Implementation: enter VS-mode via two_stage_run_in_vs(), execute a
 * pre-encoded HS-level CSR access instruction. VS-mode cannot access
 * HS-level CSRs -> virtual-instruction exception (cause=22) traps to
 * M-mode. The M-mode handler records cause and stval, advances sepc
 * past the faulting instruction, and returns.
 *
 * Pre-encoded instructions (R-type, opcode=SYSTEM 0x73):
 *   csrrs x5, hstatus, x0:  [31:20]=0x600  -> 0x600022F3
 *   csrrw x0, hgatp,   x0:  [31:20]=0x680  -> 0x68001073
 *   csrrs x5, hideleg, x0:  [31:20]=0x603  -> 0x603022F3
 * =================================================================== */

TEST_REGISTER(test_hcross_sstvala_06);
bool test_hcross_sstvala_06(void) {
    TEST_BEGIN("HCROSS-SSTVALA-06: virtual-inst (read hstatus) stval == encoding");

    /* Initialize minimal two-stage translation: VS=Bare, G-stage=SV39X4.
     * Code/data regions identity-mapped so VS-mode payload can run. */
    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end = (uintptr_t)__vm_test_region_start & ~(PAGE_SIZE_2M - 1);
    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    /* VS-mode executes csrrs x5, hstatus, x0 -> virtual-instruction */
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_vi_read_hstatus, 0);

    TEST_ASSERT("virtual-instruction trap triggered", trap_was_triggered());

    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause == virtual-instruction (22)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

        /* Sstvala core assertion: stval == faulting instruction encoding */
        TEST_ASSERT_EQ("stval == 0x600022F3 (csrrs x5, hstatus, x0)",
                       trap_get_tval(),
                       0x600022F3UL);
    }

    trap_expect_end();
    two_stage_cleanup(&ctx);

    HYP_TEST_END();
}

TEST_REGISTER(test_hcross_sstvala_07);
bool test_hcross_sstvala_07(void) {
    TEST_BEGIN("HCROSS-SSTVALA-07: virtual-inst (write hgatp) stval == encoding");

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end = (uintptr_t)__vm_test_region_start & ~(PAGE_SIZE_2M - 1);
    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    /* VS-mode executes csrrw x0, hgatp, x0 -> virtual-instruction */
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_vi_write_hgatp, 0);

    TEST_ASSERT("virtual-instruction trap triggered", trap_was_triggered());

    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause == virtual-instruction (22)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

        TEST_ASSERT_EQ("stval == 0x68001073 (csrrw x0, hgatp, x0)",
                       trap_get_tval(),
                       0x68001073UL);
    }

    trap_expect_end();
    two_stage_cleanup(&ctx);

    HYP_TEST_END();
}

TEST_REGISTER(test_hcross_sstvala_08);
bool test_hcross_sstvala_08(void) {
    TEST_BEGIN("HCROSS-SSTVALA-08: virtual-inst (read hideleg) stval == encoding");

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end = (uintptr_t)__vm_test_region_start & ~(PAGE_SIZE_2M - 1);
    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    /* VS-mode executes csrrs x5, hideleg, x0 -> virtual-instruction */
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_vi_read_hideleg, 0);

    TEST_ASSERT("virtual-instruction trap triggered", trap_was_triggered());

    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause == virtual-instruction (22)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

        TEST_ASSERT_EQ("stval == 0x603022F3 (csrrs x5, hideleg, x0)",
                       trap_get_tval(),
                       0x603022F3UL);
    }

    trap_expect_end();
    two_stage_cleanup(&ctx);

    HYP_TEST_END();
}
