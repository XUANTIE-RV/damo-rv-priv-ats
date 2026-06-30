/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Shlcofideleg Extension Compliance Test entry point
 *
 * See DOCS/testplan/Shlcofideleg_test_plan.md for the full test plan.
 *
 * Shlcofideleg controls whether LCOFI (Local Count Overflow Interrupt)
 * can be delegated from HS-mode to VS-mode via hideleg[13].
 *
 * Group 1 (WR)   : hideleg[13] writability
 * Group 2 (RO)   : hideleg[13]=0 read-only zero
 * Group 3 (ALIAS): hideleg[13]=1 alias verification
 * Group 4 (VS)   : VS-mode end-to-end
 * Group 5 (DYN)  : hideleg dynamic toggle consistency
 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    printf("\n");
    printf("======================================================\n");
    printf("  RISC-V Shlcofideleg Extension Compliance Test\n");
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
