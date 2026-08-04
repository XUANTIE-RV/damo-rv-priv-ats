/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor Interrupts Subset Test entry point
 *
 * See DOCS/testplan/Hypervisor_Interrupts_test_plan.md for the full test plan.
 *
 * This suite covers the H-extension interrupt behaviour including:
 *   - Virtual interrupt injection (hvip/hip/hie)
 *   - Guest external interrupts (hgeip/hgeie)
 *   - M-level interrupt delegation enhancements (mideleg/mip/mie)
 *   - hideleg interrupt delegation and VS interrupt number translation
 */

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    test_print_banner("RISC-V Hypervisor Interrupts Subset Test");

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
