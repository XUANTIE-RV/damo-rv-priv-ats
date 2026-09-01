/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zihpm Cross Compliance Test
 *
 * Tests for the Zihpm (Hardware Performance Counters) extension in
 * Hypervisor scenarios:
 *   - Unimplemented hpmcounter behavior when V=1 (constant value or
 *     exception are both legal) and its interaction with hcounteren
 *     gating
 *   - VU-mode three-layer gating chain (mcounteren -> hcounteren ->
 *     scounteren) for implemented hpmcounters
 *
 * See DOCS/testplan/Hypervisor_Zi_test_plan.md Group 7 for the full
 * test plan.
 *
 * Test ID mapping:
 *   Group 7.1 (unimplemented counters): HZHPM-01~04
 *   Group 7.2 (VU three-layer chain):   HZHPM-05
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

    test_print_banner("RISC-V Hypervisor x Zihpm Cross Test");

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
