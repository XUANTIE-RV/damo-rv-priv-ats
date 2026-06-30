/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * test_helpers.c - Shared helpers for Sv*x4 G-stage tests
 *
 * These were originally defined inside test_pte_valid.c, but multiple
 * Group test files (test_gpa_high, test_rwx, test_ubit, ...) need
 * them. Extracting to a standalone TU breaks the implicit
 * include-order coupling between Groups in test_register.c.
 *
 * See test_helpers.h for prototypes / contracts.
 * =================================================================== */

#include "test_helpers.h"

void _setup_with_victim(two_stage_ctx_t *ctx,
                        uintptr_t victim_gpa,
                        uintptr_t victim_flags)
{
    gpt_pool_reset();
    two_stage_init(ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    /* (1) Kernel/UART region at 2MB granularity. */
    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end  = (uintptr_t)__vm_test_region_start
                        & ~(PAGE_SIZE_2M - 1);
    for (uintptr_t a = lo_base; a < lo_end; a += PAGE_SIZE_2M)
        (void)gpt_map_page(&ctx->g_ctx, a, a, G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    /* Make sure the UART page is mapped (covers low MMIO if outside
     * the kernel image range). */
    uintptr_t uart_pg = PLATFORM_UART0_BASE & ~(PAGE_SIZE_4K - 1);
    if (uart_pg < lo_base || uart_pg >= lo_end) {
        uintptr_t uart_flags = PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D;
        (void)gpt_map_page(&ctx->g_ctx, uart_pg, uart_pg,
                           uart_flags, PT_LEVEL_4K);
    }

    /* (2)+(3) Test region: each 4KB page, with overrides for victim. */
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_end   = (uintptr_t)__vm_test_region_end;
    uintptr_t victim_page = victim_gpa & ~(PAGE_SIZE_4K - 1);
    for (uintptr_t a = r_start; a < r_end; a += PAGE_SIZE_4K) {
        uintptr_t f = (a == victim_page) ? victim_flags : G_FLAGS_RWXU_AD;
        (void)gpt_map_page(&ctx->g_ctx, a, a, f, PT_LEVEL_4K);
    }
}

bool _vsfault_check(uintptr_t (*helper)(uintptr_t),
                    uintptr_t target,
                    uintptr_t victim_flags,
                    uintptr_t expected_cause)
{
    two_stage_ctx_t ctx;
    _setup_with_victim(&ctx, target, victim_flags);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, helper, target);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    two_stage_cleanup(&ctx);
    hyp_reset_state();

    if (!fired) {
        printf("  expected guest-page-fault but none fired\n");
        return false;
    }
    if (cause != expected_cause) {
        printf("  cause mismatch: got %lu, expected %lu\n",
               (unsigned long)cause, (unsigned long)expected_cause);
        return false;
    }
    return true;
}
