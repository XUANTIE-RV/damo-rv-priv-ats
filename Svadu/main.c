/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "vm/vm.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

/* Defined in tests/test_register.c; populated here before any test runs
 * so that SVADU-CSR-04 can inspect the reset value of menvcfg. */
extern uintptr_t g_menvcfg_reset_value;

int main(void) {
    uart_init();
    reset_state();

    /* Snapshot menvcfg BEFORE any test runs. SVADU-CSR-01/02 etc.
     * modify ADUE, so this must happen first.
     *
     * menvcfg is an M-level CSR and main() runs in S-mode under the
     * T-SBI S-mode-normal model, so the snapshot goes through the
     * T-SBI CSR proxy instead of a bare csrr. */
    g_menvcfg_reset_value = tsbi_csr_read(CSR_MENVCFG);
    test_print_banner("RISC-V Svadu Extension Compliance Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
