/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zacas Cross Compliance Test
 *
 * Tests for the Zacas (Atomic Compare-and-Swap) extension in Hypervisor
 * scenarios: amocas.w/d/q exception classification into the store/AMO
 * class, the unconditional write-permission requirement (a FAILED CAS
 * still needs write permission, norm:Zacas_amocas_w_permission),
 * VS-stage/G-stage delegation, htinst transformed-vs-pseudoinstruction
 * disambiguation (funct5=00101), misaligned amocas + MAG relaxation,
 * FIOM/ADUE interaction, the no-guest-atomic-equivalent boundary, and the
 * hstateen0 non-gating check (HZACAS-36).
 *
 * See DOCS/testplan/Hypervisor_Za_test_plan.md Group 3 for the full test
 * plan. Non-hypervisor Zacas behavior is covered by the Zacas/ suite.
 *
 * Test ID mapping:
 *   Group 3.1 (normal exec):        HZACAS-01~04
 *   Group 3.2 (store/AMO class):    HZACAS-05~10
 *   Group 3.3 (G-stage/trap ctx):   HZACAS-11~16
 *   Group 3.4 (htinst):             HZACAS-17~22
 *   Group 3.5 (misaligned/MAG):     HZACAS-23~26
 *   Group 3.6 (FIOM/ADUE):          HZACAS-27~33
 *   Group 3.7 (boundary):           HZACAS-34~35
 *   Group 3.8 (hstateen0):          HZACAS-36
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

    test_print_banner("RISC-V Hypervisor x Zacas Cross Test");
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
