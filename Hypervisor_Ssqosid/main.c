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

    /* PMP full-space RWX for S/U is already installed by entry.S during the
     * M-mode boot phase (before dropping to S), so no per-suite PMP setup is
     * needed here; a raw csrw pmpaddr0/pmpcfg0 from S-mode would trap. */
    test_print_banner("RISC-V Hypervisor x Ssqosid Cross Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
