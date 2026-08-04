/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor Exceptions Subset Test entry point
 *
 * See DOCS/testplan/Hypervisor_Exceptions_test_plan.md for the full test plan.
 *
 * This suite covers the H-extension exception and trap behaviour including:
 *   - Virtual-instruction exception (cause=22)
 *   - Trap entry/return behaviour under H-extension
 *   - htinst/mtinst transformed instructions
 *   - mstatus enhancements (MPV/GVA/TVM/MPRV)
 *   - mtval2/mtinst registers
 *   - Exception priority
 *   - hedeleg exception delegation chain
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

    test_print_banner("RISC-V Hypervisor Exceptions Subset Test");

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
