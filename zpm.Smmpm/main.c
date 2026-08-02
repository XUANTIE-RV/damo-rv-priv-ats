/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - ZPM Smmpm (M-mode Pointer Masking) Test entry point
 *
 * See DOCS/testplan/zpm_test_plan.md Smmpm-related groups:
 *   Group 1.c: Smmpm capability detection (ZPM-CAP-03, 06)
 *   Group 2c:  mseccfg.PMM CSR control (ZPM-CSR-11~15)
 *   Group 6:   M-mode PA ignore transform (ZPM-MPA-01~09)
 *   Group 8.c: MPRV - M-mode PM (ZPM-MPRV-04, 05)
 *   Group 9.b: MXR - M-mode (ZPM-MXR-03, 04)
 *   Group 10.b: Trap - M-mode mtval (ZPM-TRAP-02)
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    test_print_banner("RISC-V Pointer Masking (Smmpm) Compliance Test");

    printf("  Smmpm (M-mode PM): %s\n",
           detect_smmpm() ? "detected" : "not detected");


    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
