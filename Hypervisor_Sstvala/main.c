/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor × Sstvala Cross Test entry point
 *
 * Verifies Sstvala extension behavior in Hypervisor scenarios:
 *   - Guest-page-fault stval/vstval precision (cause 20/23):
 *     Instruction fetch, store, and AMO guest-page-faults
 *   - Delegated guest-page-faults to VS-mode (vstval verification)
 *   - Virtual-instruction stval precision (cause=22):
 *     VS-mode access to HS-level CSRs (hstatus, hgatp, hideleg)
 *
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 2.
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
    printf("  Hypervisor × Sstvala Cross Test\n");
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
