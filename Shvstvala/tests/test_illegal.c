/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_illegal.c - Group 4: Illegal Instruction Exceptions
 *
 * Tests VSTVAL-ILL-01, VSTVAL-ILL-02, VSTVAL-ILL-03, VSTVAL-ILL-04,
 *       VSTVAL-ILL-05
 *
 * Validates that when an illegal-instruction exception (cause=2) is
 * delegated to VS-mode, vstval is written with the faulting instruction
 * encoding.
 *
 * Setup: hedeleg bit 2 delegates illegal-instruction to VS-mode.
 * Pre-placed instruction encodings at known addresses executed in VS-mode.
 */

/* Pre-placed illegal instruction encodings in .data section */
static uint32_t illegal_custom0   __attribute__((aligned(4), section(".data"))) = 0x0000000B;
static uint32_t illegal_wr_cycle  __attribute__((aligned(4), section(".data"))) = 0xC0001073;
static uint32_t illegal_csr_fff   __attribute__((aligned(4), section(".data"))) = 0xFFF022F3;
static uint16_t illegal_c_zero    __attribute__((aligned(2), section(".data"))) = 0x0000;

/* ------------------------------------------------------------------ */
/* VSTVAL-ILL-01: 32-bit illegal instruction (custom-0 opcode)        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_ill_01);
bool test_shvstvala_ill_01(void) {
    TEST_BEGIN("VSTVAL-ILL-01: illegal custom-0 vstval == 0x0000000B");

    /* Delegate illegal-instruction (cause=2) to VS-mode */
    hyp_delegate_to_vs((1UL << 2), 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = 0;

    /* Two-stage identity map for execution */
    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t lo_end  = r_start & ~(PAGE_SIZE_2M - 1);
    uintptr_t vs_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    two_stage_vs_identity(&ctx, lo_base, lo_end - lo_base,
                          vs_flags, PT_LEVEL_2M);
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_vs_identity(&ctx, r_start, r_size, vs_flags, PT_LEVEL_4K);

    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    uintptr_t addr = (uintptr_t)&illegal_custom0;
    two_stage_run_in_vs(&ctx, vsmode_exec_at, addr);

    TEST_ASSERT_EQ("vscause == illegal-instruction (2)",
                   g_shvstvala_cause, (uintptr_t)2);
    TEST_ASSERT_EQ("vstval == 0x0000000B",
                   g_shvstvala_vstval, (uintptr_t)0x0000000BUL);

    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* VSTVAL-ILL-02: 32-bit illegal (write read-only CSR cycle)          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_ill_02);
bool test_shvstvala_ill_02(void) {
    TEST_BEGIN("VSTVAL-ILL-02: write read-only CSR vstval == 0xC0001073");

    hyp_delegate_to_vs((1UL << 2), 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = 0;

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t lo_end  = r_start & ~(PAGE_SIZE_2M - 1);
    uintptr_t vs_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    two_stage_vs_identity(&ctx, lo_base, lo_end - lo_base,
                          vs_flags, PT_LEVEL_2M);
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_vs_identity(&ctx, r_start, r_size, vs_flags, PT_LEVEL_4K);

    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    uintptr_t addr = (uintptr_t)&illegal_wr_cycle;
    two_stage_run_in_vs(&ctx, vsmode_exec_at, addr);

    TEST_ASSERT_EQ("vscause == illegal-instruction (2)",
                   g_shvstvala_cause, (uintptr_t)2);
    TEST_ASSERT_EQ("vstval == 0xC0001073",
                   g_shvstvala_vstval, (uintptr_t)0xC0001073UL);

    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* VSTVAL-ILL-03: 32-bit illegal (access nonexistent CSR 0xFFF)       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_ill_03);
bool test_shvstvala_ill_03(void) {
    TEST_BEGIN("VSTVAL-ILL-03: nonexistent CSR vstval == 0xFFF022F3");

    hyp_delegate_to_vs((1UL << 2), 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = 0;

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t lo_end  = r_start & ~(PAGE_SIZE_2M - 1);
    uintptr_t vs_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    two_stage_vs_identity(&ctx, lo_base, lo_end - lo_base,
                          vs_flags, PT_LEVEL_2M);
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_vs_identity(&ctx, r_start, r_size, vs_flags, PT_LEVEL_4K);

    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    uintptr_t addr = (uintptr_t)&illegal_csr_fff;
    two_stage_run_in_vs(&ctx, vsmode_exec_at, addr);

    TEST_ASSERT_EQ("vscause == illegal-instruction (2)",
                   g_shvstvala_cause, (uintptr_t)2);
    TEST_ASSERT_EQ("vstval == 0xFFF022F3",
                   g_shvstvala_vstval, (uintptr_t)0xFFF022F3UL);

    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* VSTVAL-ILL-04: 16-bit compressed illegal instruction (0x0000)      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_ill_04);
bool test_shvstvala_ill_04(void) {
    TEST_BEGIN("VSTVAL-ILL-04: compressed illegal vstval == 0x0000");

    hyp_delegate_to_vs((1UL << 2), 0);

    g_shvstvala_vstval = ~0UL;
    g_shvstvala_cause  = 0;

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t lo_end  = r_start & ~(PAGE_SIZE_2M - 1);
    uintptr_t vs_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    two_stage_vs_identity(&ctx, lo_base, lo_end - lo_base,
                          vs_flags, PT_LEVEL_2M);
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_vs_identity(&ctx, r_start, r_size, vs_flags, PT_LEVEL_4K);

    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    uintptr_t addr = (uintptr_t)&illegal_c_zero;
    two_stage_run_in_vs(&ctx, vsmode_exec_at, addr);

    TEST_ASSERT_EQ("vscause == illegal-instruction (2)",
                   g_shvstvala_cause, (uintptr_t)2);
    TEST_ASSERT_EQ("vstval == 0x0000 (16-bit encoding)",
                   g_shvstvala_vstval, (uintptr_t)0x0000UL);

    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* VSTVAL-ILL-05: consecutive illegal instructions have different     */
/*               vstval values                                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_ill_05);
bool test_shvstvala_ill_05(void) {
    TEST_BEGIN("VSTVAL-ILL-05: consecutive illegal instructions track vstval");

    hyp_delegate_to_vs((1UL << 2), 0);

    /* --- First illegal instruction: custom-0 (0x0000000B) --- */
    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = 0;

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t lo_end  = r_start & ~(PAGE_SIZE_2M - 1);
    uintptr_t vs_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    two_stage_vs_identity(&ctx, lo_base, lo_end - lo_base,
                          vs_flags, PT_LEVEL_2M);
    uintptr_t r_size = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_vs_identity(&ctx, r_start, r_size, vs_flags, PT_LEVEL_4K);

    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    uintptr_t addr1 = (uintptr_t)&illegal_custom0;
    two_stage_run_in_vs(&ctx, vsmode_exec_at, addr1);

    TEST_ASSERT_EQ("1st: vscause == 2", g_shvstvala_cause, (uintptr_t)2);
    TEST_ASSERT_EQ("1st: vstval == 0x0000000B",
                   g_shvstvala_vstval, (uintptr_t)0x0000000BUL);

    two_stage_cleanup(&ctx);

    /* --- Second illegal instruction: CSR 0xFFF (0xFFF022F3) --- */
    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = 0;

    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    two_stage_vs_identity(&ctx, lo_base, lo_end - lo_base,
                          vs_flags, PT_LEVEL_2M);
    two_stage_vs_identity(&ctx, r_start, r_size, vs_flags, PT_LEVEL_4K);
    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    uintptr_t addr2 = (uintptr_t)&illegal_csr_fff;
    two_stage_run_in_vs(&ctx, vsmode_exec_at, addr2);

    TEST_ASSERT_EQ("2nd: vscause == 2", g_shvstvala_cause, (uintptr_t)2);
    TEST_ASSERT_EQ("2nd: vstval == 0xFFF022F3",
                   g_shvstvala_vstval, (uintptr_t)0xFFF022F3UL);

    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}
