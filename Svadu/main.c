/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "vm/vm.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

/* Defined in tests/test_register.c; populated here before any test runs
 * so that SVADU-CSR-04 can inspect the reset value of menvcfg. */
extern uintptr_t g_menvcfg_reset_value;

static inline uintptr_t menvcfg_read_raw(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x30A" : "=r"(v));
    return v;
}

int main(void) {
    uart_init();
    reset_state();

    /* Snapshot menvcfg BEFORE any test runs. SVADU-CSR-01/02 etc.
     * modify ADUE, so this must happen first. */
    g_menvcfg_reset_value = menvcfg_read_raw();

    /* Print banner */
    printf("\n");
    printf("==============================================\n");
    printf("  RISC-V Svadu Extension Compliance Test\n");
    printf("==============================================\n");
    printf("  Platform:     %s\n", CONFIG_NAME);
    printf("  XLEN:         %d\n", __riscv_xlen);

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
