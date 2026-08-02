/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Sstvala Extension Compliance Test entry point
 *
 * See docs/sstvala_test_plan.md for the full test plan.
 *
 * The Sstvala extension specifies the values written to stval on
 * different exception types:
 *   - Address-type exceptions (page-fault, access-fault, misaligned,
 *     non-EBREAK breakpoint): stval = faulting virtual address
 *   - Instruction-type exceptions (illegal-instruction,
 *     virtual-instruction): stval = faulting instruction encoding
 */

#include "test_framework.h"
#include "vm/vm.h"
#include "tests/test_helpers.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    /* Delegate Sstvala-relevant exceptions to S-mode */
    sstvala_delegate_exceptions();
    test_print_banner("RISC-V Sstvala Extension Compliance Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
