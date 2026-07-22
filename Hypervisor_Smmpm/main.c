/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Smmpm Cross Test entry point
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 7.
 *
 * This suite verifies the Smmpm pointer-masking extension behavior
 * in Hypervisor scenarios:
 *   Group 7: MPRV+MPV effective-privilege PM selection (HZPM-MPRV)
 */

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"
#include "pm/pm_cfg.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    printf("\n");
    printf("======================================================\n");
    printf("  Hypervisor x Smmpm Cross Test\n");
    printf("======================================================\n");
    printf("  Platform:     %s\n", CONFIG_NAME);
    printf("  XLEN:         %d\n", __riscv_xlen);
    printf("----------------------------------------------\n");
    printf("  Smmpm (mseccfg.PMM):        %s\n",
           detect_smmpm() ? "detected" : "not detected");
    printf("  Smnpm (menvcfg.PMM):        %s\n",
           detect_smnpm() ? "detected" : "not detected");
    printf("  Ssnpm hyp (henvcfg/HUPMM):  %s\n",
           detect_ssnpm_hyp() ? "detected" : "not detected");
    printf("======================================================\n\n");

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
