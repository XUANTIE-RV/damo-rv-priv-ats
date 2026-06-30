/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_misaligned.c - Group 3: Misaligned Address Exceptions
 *
 * Tests VSTVAL-IMA-01, VSTVAL-LMA-01, VSTVAL-SMA-01
 *
 * Validates that when a misaligned exception is delegated to VS-mode,
 * vstval is written with the faulting (unaligned) virtual address.
 *
 * Note: LMA/SMA tests may be skipped on platforms with hardware
 * misaligned access support.
 */

/* ------------------------------------------------------------------ */
/* VSTVAL-IMA-01: instruction address misaligned (jump to odd addr)   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_ima_01);
bool test_shvstvala_ima_01(void) {
    TEST_BEGIN("VSTVAL-IMA-01: instruction misaligned vstval == faulting addr");

    /* With C extension (IALIGN=16), JALR clears bit 0 of target address,
     * making it always 2-byte aligned. Instruction-address-misaligned is
     * impossible in this configuration. Detect C extension via misa. */
    uintptr_t misa;
    asm volatile ("csrr %0, misa" : "=r"(misa));
    if (misa & (1UL << 2)) {  /* bit 2 = 'C' extension */
        TEST_SKIP("C extension present: JALR always clears bit 0, "
                  "instruction misaligned impossible");
    }

    /* Delegate instruction address misaligned (cause=0) to VS-mode */
    hyp_delegate_to_vs((1UL << 0), 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = ~0UL;

    /* Two-stage identity map for VS-mode execution */
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

    /* Without C extension, IALIGN=32. Jump target with bit 1 set
     * (but bit 0 cleared by JALR) gives a 2-byte aligned address
     * that is not 4-byte aligned -> instruction misaligned. */
    uintptr_t target = ((uintptr_t)test_exec_page) | 0x2UL;

    two_stage_run_in_vs(&ctx, vsmode_jump_misaligned, target);

    TEST_ASSERT_EQ("vscause == instruction address misaligned (0)",
                   g_shvstvala_cause, (uintptr_t)0);
    TEST_ASSERT_EQ("vstval == misaligned target address",
                   g_shvstvala_vstval, target);

    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* VSTVAL-LMA-01: load address misaligned                             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_lma_01);
bool test_shvstvala_lma_01(void) {
    TEST_BEGIN("VSTVAL-LMA-01: load misaligned vstval == faulting addr");

    /* Delegate load address misaligned (cause=4) to VS-mode */
    hyp_delegate_to_vs((1UL << 4), 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = ~0UL;

    /* Two-stage identity map */
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

    /* Misaligned address: base + 3 (not 8-byte aligned for ld) */
    uintptr_t misaligned_addr = ((uintptr_t)test_data_area) + 3;

    two_stage_run_in_vs(&ctx, vsmode_load_misaligned, misaligned_addr);

    /* If platform supports hardware misaligned load, no trap occurs */
    if (g_shvstvala_cause == ~0UL) {
        two_stage_cleanup(&ctx);
        hyp_undelegate();
        TEST_SKIP("platform supports hardware misaligned load");
    }

    TEST_ASSERT_EQ("vscause == load address misaligned (4)",
                   g_shvstvala_cause, (uintptr_t)4);
    TEST_ASSERT_EQ("vstval == misaligned address",
                   g_shvstvala_vstval, misaligned_addr);

    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* VSTVAL-SMA-01: store address misaligned                            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_sma_01);
bool test_shvstvala_sma_01(void) {
    TEST_BEGIN("VSTVAL-SMA-01: store misaligned vstval == faulting addr");

    /* Delegate store address misaligned (cause=6) to VS-mode */
    hyp_delegate_to_vs((1UL << 6), 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = ~0UL;

    /* Two-stage identity map */
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

    /* Misaligned address: base + 5 (not 8-byte aligned for sd) */
    uintptr_t misaligned_addr = ((uintptr_t)test_data_area) + 5;

    two_stage_run_in_vs(&ctx, vsmode_store_misaligned, misaligned_addr);

    /* If platform supports hardware misaligned store, no trap occurs */
    if (g_shvstvala_cause == ~0UL) {
        two_stage_cleanup(&ctx);
        hyp_undelegate();
        TEST_SKIP("platform supports hardware misaligned store");
    }

    TEST_ASSERT_EQ("vscause == store address misaligned (6)",
                   g_shvstvala_cause, (uintptr_t)6);
    TEST_ASSERT_EQ("vstval == misaligned address",
                   g_shvstvala_vstval, misaligned_addr);

    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}
