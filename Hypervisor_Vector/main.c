/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x V-vector Cross Compliance Test
 *
 * See DOCS/testplan/Hypervisor_Zi_test_plan.md Group 5 for the test
 * plan. Spec: SPEC/riscv-isa-manual/src/unpriv/vector-common.adoc
 *
 * Test ID mapping:
 *   Group 5.1 (vsstatus.vs field and gating):  HVEC-01~08
 *   Group 5.2 (vector FP gating vsstatus.fs):  HVEC-09~12
 *   Group 5.3 (conditional, misa.v writable):  HVEC-13
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

    /* PMP is already configured for full S/U/V-mode access by the boot
     * code (entry.S hoists it before dropping privilege), and
     * hyp_reset_state() re-opens it below. A raw pmpaddr0/pmpcfg0 write
     * here would trap in the T-SBI S-mode-normal model, so it is
     * intentionally omitted. */

    test_print_banner("RISC-V Hypervisor x V-vector Cross Test");

    /* Disarm the M-timer up front: mtimecmp boots at 0, so MTIP is
     * pending from reset (zero-pending-interrupt execution contexts
     * are required by the plan). */
    {
        volatile uint64_t *mtimecmp =
            (volatile uint64_t *)PLATFORM_MTIMECMP_ADDR;
        *mtimecmp = (uint64_t)-1;
    }

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    /* Clean H-ext baseline before the first test. Only safe when the
     * H extension is actually implemented; each case re-checks via
     * an inline `if (!H_AVAILABLE) TEST_SKIP(...)`. */
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
