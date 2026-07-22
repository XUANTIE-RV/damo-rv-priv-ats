/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - ZPM Ssnpm (U-mode Pointer Masking) Test entry point
 *
 * See DOCS/testplan/zpm_test_plan.md Ssnpm-related groups:
 *   Group 1.a: Ssnpm capability detection (ZPM-CAP-01, 04)
 *   Group 2a:  senvcfg.PMM CSR control (ZPM-CSR-01~05)
 *   Group 3:   U-mode VA ignore transform - load/store (ZPM-UVA-01~08)
 *   Group 4:   U-mode VA ignore transform - AMO (ZPM-UAMO-01~04)
 *   Group 7.a: PM non-application - U-mode (ZPM-NEG-01, 02)
 *   Group 8.b: MPRV - U-mode PM (ZPM-MPRV-03)
 *   Group 10.a: Trap - U-mode stval (ZPM-TRAP-01, 05)
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
    printf("  RISC-V Pointer Masking (Ssnpm) Compliance Test\n");
    printf("======================================================\n");
    printf("  Platform:     %s\n", CONFIG_NAME);
    printf("  XLEN:         %d\n", __riscv_xlen);
    printf("  MEM_BASE:     0x%lx\n", (unsigned long)PLATFORM_MEM_BASE);
    printf("  MEM_SIZE:     0x%lx\n", (unsigned long)PLATFORM_MEM_SIZE);
    printf("----------------------------------------------\n");
    printf("  Ssnpm (U-mode PM): %s\n",
           detect_ssnpm() ? "detected" : "not detected");
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
