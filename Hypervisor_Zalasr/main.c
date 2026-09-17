/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zalasr Cross Compliance Test
 *
 * Tests for the Zalasr (Atomic Load-Acquire / Store-Release) extension
 * in Hypervisor scenarios: the exception-class split (load-acquire is a
 * pure load -> load class, only needs read permission; store-release is a
 * pure store -> store/AMO class, needs write permission and always
 * writes), VS-stage/G-stage delegation, htinst transformed-atomic in the
 * unified opcode 0x2F format (retaining funct5=00110/00111 to tell
 * load-acquire from store-release), misaligned access + MAG relaxation,
 * FIOM/ADUE interaction (Zalasr always carries aq/rl), the reserved
 * encodings (load without aq / store without rl -> illegal-instruction,
 * cause=2), and the no-atomic-ordered-guest-equivalent boundary.
 *
 * See DOCS/testplan/Hypervisor_Za_test_plan.md Group 5 for the full test
 * plan. Non-hypervisor Zalasr behavior is covered by the Zalasr/ suite.
 *
 * Test ID mapping:
 *   Group 5.1 (normal exec):        HZLASR-01~04
 *   Group 5.2 (class split / perm): HZLASR-05~11
 *   Group 5.3 (G-stage/trap ctx):   HZLASR-12~17
 *   Group 5.4 (htinst):             HZLASR-18~24
 *   Group 5.5 (misaligned/MAG):     HZLASR-25~28
 *   Group 5.6 (FIOM/ADUE):          HZLASR-29~33
 *   Group 5.7 (reserved encoding):  HZLASR-34
 *   Group 5.8 (boundary):           HZLASR-35~36
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

    test_print_banner("RISC-V Hypervisor x Zalasr Cross Test");
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
