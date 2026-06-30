/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Ssdbltrp Cross Test entry point
 *
 * Verifies Ssdbltrp extension behavior in Hypervisor scenarios:
 *   - henvcfg.DTE field control
 *   - vsstatus.SDT field behavior and SDT/SIE mutual exclusion
 *   - SRET clears vsstatus.SDT
 *   - MRET/SRET/MNRET cross-mode clear in Hypervisor scenarios
 *
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 12.
 */

#include "test_framework.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* Configure PMP: allow S/U-mode full access to all memory. */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    printf("\n");
    printf("======================================================\n");
    printf("  Hypervisor x Ssdbltrp Cross Test\n");
    printf("======================================================\n");
    printf("  Platform:     %s\n", CONFIG_NAME);
    printf("  XLEN:         %d\n", __riscv_xlen);

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    printf("  Test count:   %u\n", test_count);
    printf("======================================================\n\n");

    /* Clean H-ext baseline before the first test. */
    hyp_reset_state();

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
