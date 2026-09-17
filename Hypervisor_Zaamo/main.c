/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zaamo Cross Compliance Test
 *
 * Tests for the Zaamo (Atomic Memory Operations) extension in Hypervisor
 * scenarios: all AMO exceptions classified into the store/AMO class
 * (never load class), the R+W permission requirement, VS-stage/G-stage
 * delegation, htinst transformed-vs-pseudoinstruction disambiguation,
 * implicit-walk faults reported as cause=23 (AMO original type), the
 * misaligned-atomicity-granule (MAG) relaxation, FIOM/ADUE interaction
 * (an AMO always writes, so the D-bit requirement is mandatory), and the
 * absence of any guest-atomic AMO equivalent instruction.
 *
 * See DOCS/testplan/Hypervisor_Za_test_plan.md Group 2 for the full test
 * plan. Non-hypervisor Zaamo behavior is covered by the Zaamo/ suite.
 *
 * Test ID mapping:
 *   Group 2.1 (normal exec):        HZAMO-01~04
 *   Group 2.2 (store/AMO class):    HZAMO-05~09
 *   Group 2.3 (G-stage/trap ctx):   HZAMO-10~14
 *   Group 2.4 (htinst):             HZAMO-15~20
 *   Group 2.5 (misaligned/MAG):     HZAMO-21~24
 *   Group 2.6 (FIOM/ADUE):          HZAMO-25~29
 *   Group 2.7 (boundary):           HZAMO-30~31
 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

#ifndef PLATFORM_MTIMECMP_ADDR
#define PLATFORM_MTIMECMP_ADDR  (PLATFORM_CLINT_BASE + 0x4000UL)
#endif

int main(void)
{
    uart_init();
    reset_state();

    test_print_banner("RISC-V Hypervisor x Zaamo Cross Test");
    printf("  VS-stage:     Sv39\n");
    printf("  G-stage:      Sv39x4\n");

    /* Disarm the M-timer up front (mtimecmp boots at 0 -> MTIP pending). */
    {
        volatile uint64_t *mtimecmp =
            (volatile uint64_t *)PLATFORM_MTIMECMP_ADDR;
        *mtimecmp = (uint64_t)-1;
    }

    if (!H_AVAILABLE)
    {
        printf("[SKIP] H extension not declared by platform config, "
               "all cross cases skip themselves\n");
    }

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    if (H_AVAILABLE)
    {
        hyp_reset_state();
    }

    for (unsigned int i = 0; i < test_count; i++)
    {
        _test_table[i]();
    }

    return test_print_summary();
}
