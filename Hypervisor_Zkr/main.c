/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zkr Cross Compliance Test
 *
 * Tests for the Zkr (Entropy Source) extension in Hypervisor scenarios,
 * covering HS-mode and VS/VU-mode access control to the seed CSR via
 * mseccfg.SSEED, and the virtual-instruction vs illegal-instruction
 * exception distinction.
 *
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 17 for the full
 * test plan.
 *
 * Test ID mapping:
 *   Group 17.1 (HS-mode):   ZKR-HYP-01~02
 *   Group 17.2 (VS/VU-mode): ZKR-HYP-03~11
 *   Group 17.3 (priority):   ZKR-HYP-12
 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* Configure PMP: allow S/U-mode full access to all memory. */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"    /* A=NAPOT | R | W | X */
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    test_print_banner("RISC-V Hypervisor x Zkr Cross Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    /* Clean H-ext baseline before the first test. */
    hyp_reset_state();

    for (unsigned int i = 0; i < test_count; i++)
    {
        _test_table[i]();
    }

    return test_print_summary();
}
