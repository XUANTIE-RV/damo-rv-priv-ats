/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zicbom Cross Test entry point
 *
 * See DOCS/testplan/Hypervisor_CMO_test_plan.md Groups 1, 2, 4, 5, 6.
 *
 * This suite verifies the Zicbom extension (cbo.inval, cbo.clean,
 * cbo.flush) behavior under Hypervisor control:
 *   - henvcfg.CBIE control of VS/VU-mode cbo.inval
 *   - henvcfg.CBCFE control of VS/VU-mode cbo.clean/cbo.flush
 *   - htinst/mtinst standard transformation for CBO instructions
 *   - G-stage address translation faults for management instructions
 *   - virtual-instruction stval behavior
 */

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    printf("\n");
    printf("======================================================\n");
    printf("  Hypervisor x Zicbom Cross Test\n");
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
