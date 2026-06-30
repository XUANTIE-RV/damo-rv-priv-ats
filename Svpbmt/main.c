/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "vm/vm.h"
#include "encoding.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    /* Enable Svpbmt: set menvcfg.PBMTE (bit 62) = 1.
     * This is required for Spike and other implementations that
     * gate Svpbmt behind menvcfg.PBMTE. Without this, any PTE
     * with non-zero PBMT bits will trigger a page fault. */
    uintptr_t menvcfg;
    asm volatile("csrr %0, 0x30A" : "=r"(menvcfg));
    menvcfg |= MENVCFG_PBMTE;
    asm volatile("csrw 0x30A, %0" :: "r"(menvcfg) : "memory");

    /* Print banner */
    printf("\n");
    printf("==============================================\n");
    printf("  RISC-V Svpbmt Extension Compliance Test\n");
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
