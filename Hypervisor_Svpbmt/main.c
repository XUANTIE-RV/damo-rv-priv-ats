/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Svpbmt Cross Test entry point
 *
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 7.
 *
 * This suite verifies the Svpbmt extension (Page-Based Memory Types)
 * behavior in Hypervisor two-stage address translation scenarios:
 *   - G-stage PBMT=NC overrides PMA
 *   - VS-stage PBMT=IO overrides intermediate attributes
 *   - Both stages nonzero PBMT stacking
 *   - hgatp.MODE=0 (Bare) skips G-stage override
 */

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    printf("\n");
    printf("======================================================\n");
    printf("  Hypervisor x Svpbmt Cross Test\n");
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
