/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zicbop Cross Test entry point
 *
 * See DOCS/testplan/Hypervisor_CMO_test_plan.md Group 7.
 *
 * This suite verifies the Zicbop extension (prefetch.r, prefetch.w,
 * prefetch.i) behavior under Hypervisor control:
 *   - prefetch instructions do not trigger virtual-instruction in
 *     VS/VU-mode regardless of henvcfg CMO field settings
 *   - prefetch does not raise exceptions on G-stage faults
 *   - prefetch does not check A/D bits
 */

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    printf("\n");
    printf("======================================================\n");
    printf("  Hypervisor x Zicbop Cross Test\n");
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
