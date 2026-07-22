/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* Configure PMP: allow S/U-mode full access to all memory.
     * Without PMP entries, S/U-mode has no memory access at all.
     * Use NAPOT mode with all-ones address to cover entire address space. */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"    /* A=NAPOT(0x18) | R(0x01) | W(0x02) | X(0x04) = 0x1F */
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    /* Print banner */
    printf("\n");
    printf("==============================================\n");
    printf("  RISC-V Zicfilp (Landing Pad) Compliance Test\n");
    printf("==============================================\n");
    printf("  Platform:     %s\n", CONFIG_NAME);
    printf("  XLEN:         %d\n", __riscv_xlen);

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    printf("  Test count:   %u\n", test_count);
    printf("==============================================\n\n");

    for (unsigned int i = 0; i < test_count; i++)
    {
        _test_table[i]();
    }

    return test_print_summary();
}
