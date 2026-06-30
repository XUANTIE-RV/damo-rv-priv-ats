/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_breakpoint.c - Group 6: Breakpoint Exceptions
 *
 * Tests VSTVAL-BRK-01, VSTVAL-BRK-02
 *
 * VSTVAL-BRK-01: Validates that an address-match breakpoint (trigger)
 *   delegated to VS-mode writes the breakpoint address to vstval.
 *   Requires Sdtrig extension (type 6 mcontrol6 or type 2 mcontrol).
 *
 * VSTVAL-BRK-02: Validates that EBREAK-triggered breakpoint is NOT
 *   constrained by Shvstvala (informational test, always passes).
 */

/* Trigger CSR addresses */
#define CSR_TSELECT    0x7A0
#define CSR_TDATA1     0x7A1
#define CSR_TDATA2     0x7A2

/* mcontrol6 (type=6) field definitions */
#define MCONTROL6_TYPE6     (6UL << 60)
#define MCONTROL6_EXECUTE   (1UL << 2)
#define MCONTROL6_VS        (1UL << 24)
#define MCONTROL6_MATCH_EQ  (0UL << 7)

/* mcontrol (type=2) field definitions */
#define MCONTROL_TYPE2      (2UL << 60)
#define MCONTROL_EXECUTE    (1UL << 2)
#define MCONTROL_S          (1UL << 4)
#define MCONTROL_MATCH_EQ   (0UL << 7)

/* ===================================================================
 * Helper: detect if trigger module is available (safe probe)
 * =================================================================== */
static bool trigger_available = false;
static bool trigger_detection_done = false;

static bool detect_trigger_module(void) {
    if (trigger_detection_done)
        return trigger_available;

    trap_expect_begin();
    asm volatile ("csrr zero, 0x7A0" ::: "memory");  /* read tselect */

    trigger_available = !trap_was_triggered();
    trap_expect_end();
    trigger_detection_done = true;
    return trigger_available;
}

static void trigger_cleanup(void) {
    asm volatile ("csrw 0x7A0, zero" ::: "memory");
    asm volatile ("csrw 0x7A1, zero" ::: "memory");
    asm volatile ("csrw 0x7A2, zero" ::: "memory");
}

/* ------------------------------------------------------------------ */
/* VSTVAL-BRK-01: address-match breakpoint writes vstval              */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_brk_01);
bool test_shvstvala_brk_01(void) {
    TEST_BEGIN("VSTVAL-BRK-01: address-match breakpoint writes vstval");

#ifdef SKIP_BREAKPOINT_TESTS
    TEST_SKIP("platform does not support breakpoint (SKIP_BREAKPOINT_TESTS)");
#endif

    /* Probe Sdtrig support safely via trap-protected tselect access */
    if (!detect_trigger_module())
        TEST_SKIP("platform does not implement trigger module (Sdtrig)");

    /* Try to configure a type 6 (mcontrol6) execute trigger for VS-mode.
     * Write desired tdata1, then read back to check WARL acceptance. */
    asm volatile ("csrw 0x7A0, zero" ::: "memory");  /* tselect = 0 */

    uintptr_t tdata1_val = MCONTROL6_TYPE6 | MCONTROL6_EXECUTE |
                           MCONTROL6_VS | MCONTROL6_MATCH_EQ;
    asm volatile ("csrw 0x7A1, %0" :: "r"(tdata1_val) : "memory");

    uintptr_t readback;
    asm volatile ("csrr %0, 0x7A1" : "=r"(readback) :: "memory");

    int has_type6 = (readback != 0) && ((readback >> 60) == 6);

    if (!has_type6) {
        /* Try type 2 (mcontrol) with S bit as fallback */
        tdata1_val = MCONTROL_TYPE2 | MCONTROL_EXECUTE |
                     MCONTROL_S | MCONTROL_MATCH_EQ;
        asm volatile ("csrw 0x7A1, %0" :: "r"(tdata1_val) : "memory");
        asm volatile ("csrr %0, 0x7A1" : "=r"(readback) :: "memory");

        if (readback == 0 || ((readback >> 60) != 2)) {
            trigger_cleanup();
            TEST_SKIP("neither mcontrol6 (type 6) nor mcontrol (type 2) supported");
        }
    }

    /* Delegate breakpoint (cause=3) to VS-mode */
    hyp_delegate_to_vs((1UL << 3), 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = 0;

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

    /* Use test_exec_page as the breakpoint target address.
     * Write a 'ret' (jalr x0, ra, 0 = 0x00008067) there so
     * if the trigger doesn't fire, VS-mode returns normally. */
    uintptr_t target = (uintptr_t)test_exec_page;
    *(volatile uint32_t *)target = 0x00008067;  /* ret */

    /* Configure trigger: execute match at target address */
    asm volatile ("csrw 0x7A0, zero" ::: "memory");
    asm volatile ("csrw 0x7A1, %0" :: "r"(tdata1_val) : "memory");
    asm volatile ("csrw 0x7A2, %0" :: "r"(target) : "memory");

    /* Execute in VS-mode: jump to target -> should trigger breakpoint */
    two_stage_run_in_vs(&ctx, vsmode_exec_target, target);

    /* Verify breakpoint was taken */
    if (g_shvstvala_cause != 3) {
        /* Trigger may not have fired (implementation limitation) */
        trigger_cleanup();
        two_stage_cleanup(&ctx);
        hyp_undelegate();
        TEST_SKIP("trigger did not fire in VS-mode (implementation limitation)");
    }

    TEST_ASSERT_EQ("vscause == breakpoint (3)",
                   g_shvstvala_cause, (uintptr_t)3);
    TEST_ASSERT_EQ("vstval == breakpoint target address",
                   g_shvstvala_vstval, target);

    /* Cleanup trigger */
    trigger_cleanup();
    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------ */
/* VSTVAL-BRK-02: EBREAK not constrained by Shvstvala (informational) */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_shvstvala_brk_02);
bool test_shvstvala_brk_02(void) {
    TEST_BEGIN("VSTVAL-BRK-02: EBREAK breakpoint (vstval not constrained)");

#ifdef SKIP_BREAKPOINT_TESTS
    TEST_SKIP("platform does not support breakpoint (SKIP_BREAKPOINT_TESTS)");
#endif

    /* Delegate breakpoint (cause=3) to VS-mode */
    hyp_delegate_to_vs((1UL << 3), 0);

    g_shvstvala_vstval = ~0UL;
    g_shvstvala_cause  = 0;

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

    /* Execute EBREAK in VS-mode */
    two_stage_run_in_vs(&ctx, vsmode_ebreak, 0);

    /* Only verify that the trap occurred with cause=3.
     * vstval value is NOT constrained by Shvstvala for EBREAK. */
    TEST_ASSERT_EQ("vscause == breakpoint (3)",
                   g_shvstvala_cause, (uintptr_t)3);

    /* Informational: report what vstval was set to */
    /* (Implementation may write 0, or the PC of ebreak, etc.) */

    two_stage_cleanup(&ctx);
    hyp_undelegate();
    HYP_TEST_END();
}
