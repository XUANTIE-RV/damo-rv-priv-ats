/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "smcntrpmf_helpers.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();

    /* Common reset */
    reset_state();

    /* Allow S/U-mode counter access */
    CSRW(mcounteren, 0xFFFFFFFF);

    /* Clear mcountinhibit to enable all counters (trap-protected,
     * in case the CSR is not implemented on some platforms) */
    trap_expect_begin();
    mcountinhibit_write(0);
    trap_expect_end();

    /* Configure PMP: allow S/U-mode full access to all memory.
     * Use NAPOT mode with all-ones address to cover entire address space. */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"    /* A=NAPOT(0x18) | R(0x01) | W(0x02) | X(0x04) = 0x1F */
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );
    test_print_banner("RISC-V Smcntrpmf Compliance Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
