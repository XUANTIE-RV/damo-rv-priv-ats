/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zalrsc Cross Compliance Test
 *
 * Tests for the Zalrsc (Load-Reserved / Store-Conditional) extension in
 * Hypervisor scenarios: LR/SC exception-class split, VS-stage/G-stage
 * delegation, htinst transformed-vs-pseudoinstruction disambiguation,
 * the SC retire-permission check (a failed SC still raises a store-class
 * fault), misalignment (no MAG relaxation), FIOM/ADUE interaction, the
 * constrained-loop forward-progress guarantee under virtualization traps,
 * and the absence of any guest-atomic (HLR/HSC) equivalent instruction.
 *
 * See DOCS/testplan/Hypervisor_Za_test_plan.md Group 1 for the full test
 * plan. Non-hypervisor Zalrsc behavior is covered by the Zalrsc/ suite
 * (DOCS/testplan/Zalrsc_test_plan.md).
 *
 * Test ID mapping:
 *   Group 1.1 (normal exec):        HZLRSC-01~04
 *   Group 1.2 (class split/deleg):  HZLRSC-05~08
 *   Group 1.3 (G-stage/trap ctx):   HZLRSC-09~14
 *   Group 1.4 (htinst):             HZLRSC-15~21
 *   Group 1.5 (SC permission):      HZLRSC-22~26
 *   Group 1.6 (misaligned):         HZLRSC-27~29
 *   Group 1.7 (FIOM/ADUE):          HZLRSC-30~34
 *   Group 1.8 (forward progress):   HZLRSC-35~38
 *   Group 1.9 (boundary):           HZLRSC-39~40
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

    test_print_banner("RISC-V Hypervisor x Zalrsc Cross Test");
    printf("  VS-stage:     Sv39\n");
    printf("  G-stage:      Sv39x4\n");

    /* Disarm the M-timer up front: mtimecmp boots at 0, so MTIP is
     * pending from reset. Any later restoration of a saved mie with
     * MTIE set would take a spurious M-timer interrupt mid-test. */
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

    /* Clean H-ext baseline before the first test (also opens PMP for
     * VS/VU access). Only safe when the H extension is implemented;
     * each case re-checks via H_AVAILABLE gating. */
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
