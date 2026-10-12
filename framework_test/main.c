/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Framework privilege switching tests - main entry point
 */

#include "test_framework.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* PMP is configured once at boot (common/entry.S) before dropping to
     * S-mode: the S-mode-normal model needs S/U access in place before
     * main() can even run, and pmpaddr/pmpcfg are M-level CSRs that a
     * test body no longer has the authority to write directly. */

    test_print_banner("Framework Privilege Switching Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
