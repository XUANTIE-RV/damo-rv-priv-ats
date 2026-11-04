/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    /* The all-space PMP entry (pmpaddr0 = -1, pmpcfg0 = NAPOT|R|W|X) is
     * hoisted into common/entry.S and programmed in M-mode before boot drops
     * to S(HS)-mode, so it must NOT be re-written here: a bare csrw pmpaddr0
     * from S-mode traps illegal under the T-SBI S-mode-normal model. */

    test_print_banner("RISC-V Hypervisor x Smstateen Cross Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
