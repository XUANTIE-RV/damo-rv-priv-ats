/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_transparent.c - Group 5: vstval Transparent Access Verification
 *
 * Tests VSTVAL-TRANS-01, VSTVAL-TRANS-02, VSTVAL-TRANS-03, VSTVAL-TRANS-04
 *
 * Validates that when V=1, accessing stval actually operates on vstval
 * (transparent substitution), and that vstval is WARL holding full
 * address-width values.
 */

/* ------------------------------------------------------------------ */
/* VSTVAL-TRANS-01: VS writes stval, HS reads vstval                  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_trans_01);
bool test_shvstvala_trans_01(void) {
    TEST_BEGIN("VSTVAL-TRANS-01: VS writes stval -> HS reads vstval");

    /* Save original vstval */
    uintptr_t saved;
    asm volatile ("csrr %0, 0x243" : "=r"(saved));

    /* Clear vstval before test */
    asm volatile ("csrw 0x243, zero");

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

    uintptr_t test_val = 0xDEADBEEFUL;
    two_stage_run_in_vs(&ctx, vsmode_write_stval, test_val);

    /* HS-mode: read vstval (CSR 0x243) */
    uintptr_t readback;
    asm volatile ("csrr %0, 0x243" : "=r"(readback));

    TEST_ASSERT_EQ("vstval == value written by VS via stval",
                   readback, test_val);

    two_stage_cleanup(&ctx);
    /* Restore */
    asm volatile ("csrw 0x243, %0" :: "r"(saved));
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* VSTVAL-TRANS-02: HS writes vstval, VS reads stval                  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_trans_02);
bool test_shvstvala_trans_02(void) {
    TEST_BEGIN("VSTVAL-TRANS-02: HS writes vstval -> VS reads stval");

    /* Save original vstval */
    uintptr_t saved;
    asm volatile ("csrr %0, 0x243" : "=r"(saved));

    /* HS-mode: write vstval with known value */
    uintptr_t test_val = 0xCAFE0000UL;
    asm volatile ("csrw 0x243, %0" :: "r"(test_val));

    /* Two-stage identity map for VS-mode */
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

    /* VS-mode reads stval (which is actually vstval when V=1) */
    uintptr_t vs_read = two_stage_run_in_vs(&ctx, vsmode_read_stval, 0);

    TEST_ASSERT_EQ("VS reads stval == value written by HS to vstval",
                   vs_read, test_val);

    two_stage_cleanup(&ctx);
    asm volatile ("csrw 0x243, %0" :: "r"(saved));
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* VSTVAL-TRANS-03: vstval persists after VS trap and return           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_trans_03);
bool test_shvstvala_trans_03(void) {
    TEST_BEGIN("VSTVAL-TRANS-03: vstval persists after trap return to HS");

    /* Delegate load page-fault (cause=13) to VS-mode */
    hyp_delegate_to_vs((1UL << 13), 0);

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

    /* Trigger load page-fault in VS-mode (trap handler writes vstval) */
    two_stage_run_in_vs(&ctx, vsmode_load_addr, UNMAPPED_VA_1);

    /* The VS-mode trap handler recorded vstval; verify it */
    TEST_ASSERT_EQ("trap handler saw cause == 13",
                   g_shvstvala_cause, (uintptr_t)13);
    TEST_ASSERT_EQ("trap handler saw vstval == UNMAPPED_VA_1",
                   g_shvstvala_vstval, UNMAPPED_VA_1);

    /* After returning to HS-mode, read vstval directly */
    uintptr_t hs_readback;
    asm volatile ("csrr %0, 0x243" : "=r"(hs_readback));
    TEST_ASSERT_EQ("HS reads vstval == trap faulting VA (not cleared)",
                   hs_readback, UNMAPPED_VA_1);

    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* VSTVAL-TRANS-04: vstval holds full address-width value (WARL)      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_trans_04);
bool test_shvstvala_trans_04(void) {
    TEST_BEGIN("VSTVAL-TRANS-04: vstval holds full address-width value");

    /* Save original vstval */
    uintptr_t saved;
    asm volatile ("csrr %0, 0x243" : "=r"(saved));

    /* Write a high-bit address value to vstval from HS-mode */
    uintptr_t test_val = 0x7FFFFFFFFFULL;  /* 39-bit address max */
    asm volatile ("csrw 0x243, %0" :: "r"(test_val));

    /* Read back and verify no truncation */
    uintptr_t readback;
    asm volatile ("csrr %0, 0x243" : "=r"(readback));
    TEST_ASSERT_EQ("vstval holds 0x7FFFFFFFFF without truncation",
                   readback, test_val);

    /* Also test with bit pattern across full XLEN */
    uintptr_t test_val2 = 0xAAAAAAAAAAAAAAAAULL;
    asm volatile ("csrw 0x243, %0" :: "r"(test_val2));
    uintptr_t readback2;
    asm volatile ("csrr %0, 0x243" : "=r"(readback2));
    /* vstval is WARL - it may not hold all bits; at minimum it should
     * hold address-width bits. Just verify read == write. */
    TEST_ASSERT_EQ("vstval WARL: readback matches written value",
                   readback2, test_val2);

    /* Restore */
    asm volatile ("csrw 0x243, %0" :: "r"(saved));
    HYP_TEST_END();
}
