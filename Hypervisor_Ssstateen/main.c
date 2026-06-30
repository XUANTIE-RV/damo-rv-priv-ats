/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Ssstateen Cross Compliance Test
 *
 * Tests for the Ssstateen extension in Hypervisor scenarios,
 * covering hstateen CSR access, bit 63 control, VS-mode ROZ propagation,
 * functional bit controls, read-only constraints, and encoding consistency.
 *
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 8 for the full test plan.
 *
 * These tests were extracted from Ssstateen/ Groups 7-12 (H-mode tests).
 * Test ID mapping:
 *   Group 8.1 (HEXIST):  SS-HEXIST-01~06  -> HCROSS-SSSTA-01~06
 *   Group 8.2 (HB63):    SS-HB63-01~09    -> HCROSS-SSSTA-07~15
 *   Group 8.3 (VSPROP):  SS-VSPROP-01~05  -> HCROSS-SSSTA-16~20
 *   Group 8.4 (HFUNC):   SS-HSE0/ENVCFG/CSRIND/IMSIC/AIA/CTX-xx -> HCROSS-SSSTA-21~38
 *   Group 8.5 (HRO):     SS-HRO-01~07     -> HCROSS-SSSTA-39~45
 *   Group 8.6 (HENC):    SS-HENC-01~05    -> HCROSS-SSSTA-46~50
 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* Configure PMP: allow S/U-mode full access to all memory. */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"    /* A=NAPOT | R | W | X */
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    printf("\n");
    printf("======================================================\n");
    printf("  RISC-V Hypervisor x Ssstateen Cross Test\n");
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

    for (unsigned int i = 0; i < test_count; i++)
    {
        _test_table[i]();
    }

    return test_print_summary();
}
