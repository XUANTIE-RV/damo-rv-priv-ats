/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - ZPM Smnpm (S-mode Pointer Masking) Test entry point
 *
 * See DOCS/testplan/zpm_test_plan.md Smnpm-related groups:
 *   Group 1.b: Smnpm capability detection (ZPM-CAP-02, 05)
 *   Group 2b:  menvcfg.PMM CSR control (ZPM-CSR-06~10)
 *   Group 5:   S-mode VA ignore transform (ZPM-SVA-01~09)
 *   Group 7.b: PM non-application - S-mode (ZPM-NEG-03, 04, 05)
 *   Group 8.a: MPRV - S-mode PM (ZPM-MPRV-01, 02)
 *   Group 9.a: MXR - S-mode (ZPM-MXR-01, 02)
 *   Group 10.c: Trap - S-mode (ZPM-TRAP-03, 04)
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    printf("\n");
    printf("======================================================\n");
    printf("  RISC-V Pointer Masking (Smnpm) Compliance Test\n");
    printf("======================================================\n");
    printf("  Platform:     %s\n", CONFIG_NAME);
    printf("  XLEN:         %d\n", __riscv_xlen);
    printf("  MEM_BASE:     0x%lx\n", (unsigned long)PLATFORM_MEM_BASE);
    printf("  MEM_SIZE:     0x%lx\n", (unsigned long)PLATFORM_MEM_SIZE);
    printf("----------------------------------------------\n");
    printf("  Smnpm (S-mode PM): %s\n",
           detect_smnpm() ? "detected" : "not detected");
    printf("======================================================\n\n");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    printf("  Test count:   %u\n\n", test_count);

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
