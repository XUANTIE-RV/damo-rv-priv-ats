/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zicfiss Cross Test entry point
 *
 * See DOCS/testplan/Hypervisor_CFI_test_plan.md Part B.
 *
 * This suite verifies the interaction between the Hypervisor (H)
 * extension and the Zicfiss extension (backward-edge CFI / shadow stack):
 *   - henvcfg.SSE field control for VS-mode Zicfiss enable
 *   - ssp CSR access control in VS/VU-mode
 *   - VS-stage Shadow Stack page type (pte.xwr=010)
 *   - G-stage translation interaction with shadow stack
 *   - satp/vsatp Bare mode shadow stack behavior
 *   - Zicfiss exception delegation (M->HS->VS)
 *   - Zicfiss functional completeness (SSPUSH/SSPOPCHK/SSAMOSWAP)
 *   - SSE=0 instruction reversion behavior
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

    /* Configure PMP: allow S/U-mode full access to all memory. */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    printf("\n");
    printf("======================================================\n");
    printf("  Hypervisor x Zicfiss Cross Test\n");
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
