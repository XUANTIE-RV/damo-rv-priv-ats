/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zabha Cross Compliance Test
 *
 * Tests for the Zabha (Byte and Halfword Atomic Memory Operations)
 * extension in Hypervisor scenarios: byte/half AMO exception
 * classification into the store/AMO class, width-independent permission,
 * VS-stage/G-stage delegation, htinst transformed-atomic funct3 width
 * retention (Zabha-specific), byte-AMO-never-misaligned vs half-AMO
 * misaligned/MAG, FIOM/ADUE interaction, amocas.b/h (Zabha x Zacas), the
 * reserved byte/half lr/sc encodings (illegal-instruction, cause=2), and
 * the no-guest-atomic-equivalent boundary.
 *
 * See DOCS/testplan/Hypervisor_Za_test_plan.md Group 4 for the full test
 * plan. Non-hypervisor Zabha behavior is covered by the Zabha/ suite.
 *
 * Test ID mapping:
 *   Group 4.1 (normal exec):        HZABHA-01~04
 *   Group 4.2 (store/AMO class):    HZABHA-05~09
 *   Group 4.3 (G-stage/trap ctx):   HZABHA-10~14
 *   Group 4.4 (htinst):             HZABHA-15~21
 *   Group 4.5 (alignment/MAG):      HZABHA-22~26
 *   Group 4.6 (FIOM/ADUE):          HZABHA-27~31
 *   Group 4.7 (amocas.b/h):         HZABHA-32~36
 *   Group 4.8 (reserved lr/sc):     HZABHA-37
 *   Group 4.9 (boundary):           HZABHA-38~39
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

    test_print_banner("RISC-V Hypervisor x Zabha Cross Test");
    printf("  VS-stage:     Sv39\n");
    printf("  G-stage:      Sv39x4\n");

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
