/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor Extension Comprehensive Test entry point
 *
 * See DOCS/testplan/Hypervisor_test_plan.md for the full test plan.
 *
 * This suite covers the H-extension baseline functionality including:
 *   - VS CSR substitution and hstatus behaviour
 *   - Exception/interrupt delegation (hedeleg/hideleg)
 *   - Virtual interrupt injection (hvip/hip/hie)
 *   - Guest external interrupts (hgeip/hgeie)
 *   - Environment configuration (henvcfg/hcounteren/htimedelta)
 *   - VS interrupt/timer CSRs (vsip/vsie/vstimecmp)
 *   - Virtual-instruction exception (cause=22)
 *   - Trap entry/return behaviour under H-extension
 *   - htinst/mtinst transformed instructions
 *   - mstatus enhancements (MPV/GVA/TVM/MPRV)
 *   - M-level interrupt delegation enhancements
 *   - mtval2/mtinst registers
 *   - Exception priority
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

    printf("\n");
    printf("======================================================\n");
    printf("  RISC-V Hypervisor Extension Comprehensive Test\n");
    printf("======================================================\n");
    printf("  Platform:     %s\n", CONFIG_NAME);
    printf("  XLEN:         %d\n", __riscv_xlen);

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    printf("  Test count:   %u\n", test_count);
    printf("======================================================\n\n");

    /* Clean H-ext baseline before the first test. */
    hyp_reset_state();

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
