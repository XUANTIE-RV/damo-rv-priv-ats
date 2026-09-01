/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Ssccfg Cross Test entry point
 *
 * Verifies Smcdeleg/Ssccfg counter-delegation behavior in Hypervisor
 * scenarios:
 *   - scountovf / scountinhibit VS/VU-mode virtualization
 *   - hvip/hvien LCOFI bit (bit 13) implementation
 *   - vsiselect/vsireg* multi-privilege access rules
 *   - hstateen0 bit 60 gating of VS-mode indirect access
 *
 * See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 11.
 */

#include "test_framework.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* Configure PMP: allow S/U/VS/VU-mode full access to all memory. */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"    /* A=NAPOT(0x18) | R(0x01) | W(0x02) | X(0x04) = 0x1F */
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    test_print_banner("Hypervisor x Ssccfg Cross Test");

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
