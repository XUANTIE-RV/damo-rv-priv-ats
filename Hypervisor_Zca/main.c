/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zca Cross Compliance Test
 *
 * Tests for the Zca (integer compressed instruction) extension in
 * Hypervisor scenarios: normal execution of compressed instructions in
 * HS/VS/VU-mode with virtual-instruction (cause=22) exclusion, the
 * three-step compressed transformation of htinst/mtinst for all eight
 * compressed load/store variants (register-based c.lw/c.sw/c.ld/c.sd
 * and stack-pointer-based c.lwsp/c.swsp/c.ldsp/c.sdsp) trapping as
 * load/store guest-page faults, the load-vs-store transformed format
 * distinction, Addr. Offset semantics, IALIGN=16 with
 * instruction-address-misaligned (cause=0) exclusion, and the rule
 * that fetch-class exceptions never write a transformed htinst.
 *
 * See DOCS/testplan_en/Hypervisor_Zc_test_plan_en.md Group 1 for the
 * full test plan. Non-hypervisor Zca behavior is covered by the
 * standalone Zca test plan; the mechanism-existence sample (a single
 * c.lw) is TINST-08 of Hypervisor_Exceptions.
 *
 * Test ID mapping:
 *   Group 1.1 (normal exec):        HZCA-01~06
 *   Group 1.2 (reg-based htinst):   HZCA-07~10
 *   Group 1.3 (sp-based htinst):    HZCA-11~14
 *   Group 1.4 (format distinction): HZCA-15~17
 *   Group 1.5 (Addr. Offset/zero):  HZCA-18~20
 *   Group 1.6 (mtinst M-mode):      HZCA-21~23
 *   Group 1.7 (interrupt zero):     HZCA-24
 *   Group 1.8 (IALIGN=16):          HZCA-25~27
 *   Group 1.9 (fetch-class):        HZCA-28~29
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

    test_print_banner("RISC-V Hypervisor x Zca Cross Test");
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
