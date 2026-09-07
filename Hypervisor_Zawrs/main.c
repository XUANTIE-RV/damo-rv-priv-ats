/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zawrs Cross Compliance Test
 *
 * Tests for the Zawrs (Wait-on-Reservation-Set) extension in
 * Hypervisor scenarios: hstatus.VTW gating of wrs.nto in VS/VU-mode,
 * mstatus.TW priority over VTW, and the absence of spurious
 * virtual-instruction exceptions in HS/VS/VU modes.
 *
 * See DOCS/testplan/Hypervisor_Za_test_plan.md Group 6 for the full
 * test plan. Non-hypervisor Zawrs behavior is covered by the Zawrs/
 * suite (DOCS/testplan/Zawrs_test_plan.md).
 *
 * Test ID mapping:
 *   Group 6.1 (normal execution): HZWRS-01~03
 *   Group 6.2 (VTW gating):       HZWRS-04~07
 *   Group 6.3 (TW priority):      HZWRS-08~12
 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

#ifndef PLATFORM_MTIMECMP_ADDR
#define PLATFORM_MTIMECMP_ADDR  (PLATFORM_CLINT_BASE + 0x4000UL)
#endif

static bool hzawrs_misa_h(void)
{
    uintptr_t misa;
    asm volatile ("csrr %0, misa" : "=r"(misa));
    return (misa & (1UL << ('H' - 'A'))) != 0;
}

int main(void)
{
    uart_init();
    reset_state();

    /* Configure PMP: allow S/U/V-mode full access to all memory. */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"    /* A=NAPOT | R | W | X */
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    test_print_banner("RISC-V Hypervisor x Zawrs Cross Test");

    /* Disarm the M-timer up front: mtimecmp boots at 0, so MTIP is
     * pending from reset. Any later restoration of a saved mie with
     * MTIE set would take a spurious M-timer interrupt mid-test.
     * Uses the CLINT layout that common/trap.c relies on. */
    {
        volatile uint64_t *mtimecmp =
            (volatile uint64_t *)PLATFORM_MTIMECMP_ADDR;
        *mtimecmp = (uint64_t)-1;
    }

    if (!hzawrs_misa_h())
    {
        printf("[SKIP] misa.H clear: H extension unavailable, "
               "all cross cases skip themselves\n");
    }

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    /* Clean H-ext baseline before the first test. Only safe when the
     * H extension is actually implemented (writes to hstatus/hgatp
     * would trap otherwise); each case re-checks via REQUIRE_H_EXT. */
    if (hzawrs_misa_h())
    {
        hyp_reset_state();
    }

    for (unsigned int i = 0; i < test_count; i++)
    {
        _test_table[i]();
    }

    return test_print_summary();
}
