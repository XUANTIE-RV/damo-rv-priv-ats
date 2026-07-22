/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zicfilp Cross Test entry point
 *
 * See DOCS/testplan/Hypervisor_CFI_test_plan.md Part A.
 *
 * This suite verifies the interaction between the Hypervisor (H)
 * extension and the Zicfilp extension (forward-edge CFI / landing pads):
 *   - henvcfg.LPE field control for VS-mode Zicfilp enable
 *   - vsstatus.SPELP save/restore across VS-mode traps
 *   - VS-mode Landing Pad enforcement and software-check exception
 *   - software-check exception delegation (M->HS->VS)
 *   - Async event interaction with ELP state
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
    printf("  Hypervisor x Zicfilp Cross Test\n");
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
