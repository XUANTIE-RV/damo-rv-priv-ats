/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Ssstateen (Supervisor-level State Enable) Extension Compliance Test
 *
 * See DOCS/testplan/ssstateen_test_plan.md for the full test plan.
 *
 * Groups 1-6:   S-mode tests (sstateen0-3)
 *
 * H-mode tests (Groups 7-12) have been extracted to Hypervisor_Ssstateen/.
 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    /* PMP full-space RWX is hoisted into common/entry.S (boot, M-mode);
     * a bare csrw pmpaddr0/pmpcfg0 here would trap in S-mode-normal. */

    test_print_banner("RISC-V Ssstateen Extension Compliance Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    /* Clean H-ext baseline before the first test. */
    hyp_reset_state();

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
