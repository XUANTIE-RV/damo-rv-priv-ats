/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "vm/vm.h"
#include "pmp_cfg.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();
    pmp_clear_all();

    /* Print banner */
    printf("\n");
    printf("==============================================\n");
    printf("  RISC-V Ssccptr Extension Compliance Test\n");
    printf("==============================================\n");
    printf("  Platform:     %s\n", CONFIG_NAME);
    printf("  XLEN:         %d\n", __riscv_xlen);
    printf("  MEM_BASE:     0x%lx\n", (unsigned long)PLATFORM_MEM_BASE);
    printf("  MEM_SIZE:     0x%lx\n", (unsigned long)PLATFORM_MEM_SIZE);

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    printf("  Test count:   %u\n", test_count);
    printf("==============================================\n\n");

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
