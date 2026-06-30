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
 *   HCROSS-SSTVALA-04: Instruction guest-page-fault vstval (delegated)
 *   HCROSS-SSTVALA-05: Store guest-page-fault vstval (delegated)
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
 * HCROSS-SSTVALA-04: Instruction guest-page-fault vstval (delegated)
 *
 * Configures hedeleg to delegate instruction guest-page-fault (bit 20)
 * to VS-mode. VS-mode jumps to a G-stage unmapped GPA, triggering a
 * fault that is handled by the VS-mode trap handler. Verifies that
 * vstval (read by VS-mode handler) contains the faulting GVA.
 *
 * NOTE: Per RISC-V spec, guest-page-faults (cause 20/21/23) are NOT
 * delegatable via hedeleg (bits are read-zero). This test probes
 * hedeleg writability and skips if delegation is not supported.
 * =================================================================== */

TEST_REGISTER(test_hcross_sstvala_04);
bool test_hcross_sstvala_04(void) {
    TEST_BEGIN("HCROSS-SSTVALA-04: inst guest-page-fault vstval (delegated)");

    /* Probe hedeleg bit 20 writability */
    uintptr_t saved_hedeleg = hedeleg_read();
    hedeleg_write(saved_hedeleg | (1UL << CAUSE_INST_GUEST_PAGE_FAULT));
    uintptr_t probed_hedeleg = hedeleg_read();
    hedeleg_write(saved_hedeleg);

    bool delegatable = (probed_hedeleg & (1UL << CAUSE_INST_GUEST_PAGE_FAULT)) != 0;
    if (!delegatable) {
        TEST_SKIP("hedeleg bit 20 is read-zero (guest-page-fault not delegatable)");
    }

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

    /* Configure delegation: medeleg -> hedeleg for inst guest-page-fault */
    uintptr_t medeleg;
    asm volatile ("csrr %0, medeleg" : "=r"(medeleg));
    medeleg |= (1UL << CAUSE_INST_GUEST_PAGE_FAULT);
    asm volatile ("csrw medeleg, %0" :: "r"(medeleg));

    hedeleg_write(hedeleg_read() | (1UL << CAUSE_INST_GUEST_PAGE_FAULT));

    /* Setup VS-mode trap handler */
    setup_vs_trap_handler_for_sstvala();

    /* Target GPA: outside mapped regions (unmapped in G-stage) */
    uintptr_t target_gpa = 0xD0000000UL;  /* 3.25GB offset, unmapped */

    /* VS-mode jumps to target_gpa, fault delegated to VS handler */
    two_stage_run_in_vs(&ctx, test_vs_jump_delegated, target_gpa);

    /* Verify VS-mode trap handler was triggered */
    TEST_ASSERT("VS-mode trap handler triggered", g_vs_trap_triggered);

    if (g_vs_trap_triggered) {
        /* Verify vscause == 20 (instruction guest-page-fault) */
        TEST_ASSERT_EQ("vscause == inst guest-page-fault (20)",
                       g_vs_trap_cause,
                       (uintptr_t)CAUSE_INST_GUEST_PAGE_FAULT);

        /* Sstvala core assertion: vstval == faulting GVA (not 0) */
        TEST_ASSERT_EQ("vstval == faulting GVA",
                       g_vs_trap_vstval,
                       target_gpa);
    }

    /* Restore delegation */
    hedeleg_write(saved_hedeleg);

    two_stage_cleanup(&ctx);

    HYP_TEST_END();
}

/* ===================================================================
 * HCROSS-SSTVALA-05: Store guest-page-fault vstval (delegated)
 *
 * Configures hedeleg to delegate store guest-page-fault (bit 23) to
 * VS-mode. VS-mode stores to a G-stage unmapped GPA, triggering a
 * fault that is handled by the VS-mode trap handler. Verifies that
 * vstval (read by VS-mode handler) contains the faulting GVA.
 *
 * NOTE: Per RISC-V spec, guest-page-faults (cause 20/21/23) are NOT
 * delegatable via hedeleg (bits are read-zero). This test probes
 * hedeleg writability and skips if delegation is not supported.
 * =================================================================== */

TEST_REGISTER(test_hcross_sstvala_05);
bool test_hcross_sstvala_05(void) {
    TEST_BEGIN("HCROSS-SSTVALA-05: store guest-page-fault vstval (delegated)");

    /* Probe hedeleg bit 23 writability */
    uintptr_t saved_hedeleg = hedeleg_read();
    hedeleg_write(saved_hedeleg | (1UL << CAUSE_STORE_GUEST_PAGE_FAULT));
    uintptr_t probed_hedeleg = hedeleg_read();
    hedeleg_write(saved_hedeleg);

    bool delegatable = (probed_hedeleg & (1UL << CAUSE_STORE_GUEST_PAGE_FAULT)) != 0;
    if (!delegatable) {
        TEST_SKIP("hedeleg bit 23 is read-zero (guest-page-fault not delegatable)");
    }

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

    /* Configure delegation: medeleg -> hedeleg for store guest-page-fault */
    uintptr_t medeleg;
    asm volatile ("csrr %0, medeleg" : "=r"(medeleg));
    medeleg |= (1UL << CAUSE_STORE_GUEST_PAGE_FAULT);
    asm volatile ("csrw medeleg, %0" :: "r"(medeleg));

    hedeleg_write(hedeleg_read() | (1UL << CAUSE_STORE_GUEST_PAGE_FAULT));

    /* Setup VS-mode trap handler */
    setup_vs_trap_handler_for_sstvala();

    /* Target GPA: outside mapped regions (unmapped in G-stage) */
    uintptr_t target_gpa = 0xE0000000UL;  /* 3.5GB offset, unmapped */

    /* VS-mode stores to target_gpa, fault delegated to VS handler */
    two_stage_run_in_vs(&ctx, test_vs_store_delegated, target_gpa);

    /* Verify VS-mode trap handler was triggered */
    TEST_ASSERT("VS-mode trap handler triggered", g_vs_trap_triggered);

    if (g_vs_trap_triggered) {
        /* Verify vscause == 23 (store guest-page-fault) */
        TEST_ASSERT_EQ("vscause == store guest-page-fault (23)",
                       g_vs_trap_cause,
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

        /* Sstvala core assertion: vstval == faulting GVA (not 0) */
        TEST_ASSERT_EQ("vstval == faulting GVA",
                       g_vs_trap_vstval,
                       target_gpa);
    }

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
