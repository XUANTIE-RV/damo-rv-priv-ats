/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "vm/vm.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

#if SUITE_SATP_MODE == SATP_MODE_SV39
    test_print_banner("RISC-V Sv39 Virtual Memory Compliance Test");
#elif SUITE_SATP_MODE == SATP_MODE_SV48
    test_print_banner("RISC-V Sv48 Virtual Memory Compliance Test");
#elif SUITE_SATP_MODE == SATP_MODE_SV57
    test_print_banner("RISC-V Sv57 Virtual Memory Compliance Test");
#else
    test_print_banner("RISC-V Sv* Virtual Memory Compliance Test");
#endif

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
