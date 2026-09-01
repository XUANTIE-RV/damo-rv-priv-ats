/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Sscofpmf Cross Test entry point
 *
 * Verifies Sscofpmf extension behavior in Hypervisor scenarios:
 *   - VS-mode scountovf dual gating (mcounteren + hcounteren)
 *   - VSINH/VUINH counting inhibition in VS/VU-mode
 *   - VSINH/VUINH read-only zero when H extension is absent
 *
 * See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 10.
 */

#include "test_framework.h"
#include "hyp/hyp_reset.h"
#include "sscofpmf_helpers.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* Initialize PMU: allow S/U-mode counter access and enable counting */
    CSRW(mcounteren, 0xFFFFFFFF);

    /* Clear mcountinhibit to enable all counters (trap-protected,
     * in case the CSR is not implemented on some platforms) */
    trap_expect_begin();
    CSRW(0x320, 0);  /* mcountinhibit */
    trap_expect_end();

    /* Configure PMP: allow S/U/VS/VU-mode full access to all memory. */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"    /* A=NAPOT(0x18) | R(0x01) | W(0x02) | X(0x04) = 0x1F */
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    test_print_banner("Hypervisor x Sscofpmf Cross Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    /* Clean H-ext baseline before the first test. */
    hyp_reset_state();

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
