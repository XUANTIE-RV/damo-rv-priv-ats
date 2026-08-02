/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Smcntrpmf Cross Test entry point
 *
 * Verifies Smcntrpmf extension behavior in Hypervisor scenarios:
 *   - VSINH/VUINH read-only zero when H ext not implemented
 *   - VSINH/VUINH inhibit VS/VU-mode cycle counting
 *   - VSINH/VUINH inhibit VS/VU-mode instret counting
 *   - hcounteren.CY control of VS-mode cycle access
 *   - VSINH inhibition orthogonal to hcounteren access control
 *
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 16.
 */

#include "test_framework.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* Allow S/U/VS/VU-mode counter access by default; individual tests
     * clear hcounteren/mcounteren bits as needed. */
    CSRW(mcounteren, 0xFFFFFFFF);

    /* Clear mcountinhibit so cycle/instret count (trap-protected in case
     * the CSR is not implemented on some platforms). */
    trap_expect_begin();
    asm volatile("csrw 0x320, zero" ::: "memory");  /* mcountinhibit */
    trap_expect_end();

    /* Configure PMP: allow S/U-mode full access to all memory. */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    test_print_banner("Hypervisor x Smcntrpmf Cross Test");

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
