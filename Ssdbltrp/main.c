/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * main.c - Ssdbltrp Extension Compliance Test
 */

#include "test_framework.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* Re-install Smdbltrp-specific M-mode trap handler.
     * reset_state() resets mtvec to the common handler (m_trap_entry)
     * which does not clear MDT on trap entry. We need the Smdbltrp
     * handler that clears MDT via MRET to prevent double traps.
     */
    extern void smdbltrp_m_trap_entry(void);
    CSRW(mtvec, (uintptr_t)&smdbltrp_m_trap_entry);

    /* Configure PMP: allow S/U-mode full access */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    printf("\n");
    printf("==============================================\n");
    printf("  RISC-V Ssdbltrp Extension Compliance Test\n");
    printf("==============================================\n");
    printf("  Platform:     %s\n", CONFIG_NAME);
    printf("  XLEN:         %d\n", __riscv_xlen);

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    printf("  Test count:   %u\n", test_count);
    printf("==============================================\n\n");

    for (unsigned int i = 0; i < test_count; i++) {
        /* Re-install Smdbltrp trap handler before each test.
         * reset_state() (called by TEST_END) resets mtvec to the default
         * handler which does not clear MDT, causing double traps.
         */
        extern void smdbltrp_m_trap_entry(void);
        CSRW(mtvec, (uintptr_t)&smdbltrp_m_trap_entry);

        _test_table[i]();
    }

    return test_print_summary();
}
