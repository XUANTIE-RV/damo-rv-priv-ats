/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zihintntl Cross Compliance Test
 *
 * See DOCS/testplan/Hypervisor_Zi_test_plan.md Group 2 for the test
 * plan. Spec: SPEC/riscv-isa-manual/src/unpriv/zihintntl.adoc
 *       (norm:NTL_target_definition, norm:NTL_range)
 *
 * Test ID mapping:
 *   Group 2.1 (HS/VS/VU normal): NTL-HYP-01~03
 *   Group 2.2 (HLV/HSV/HLVX):    NTL-HYP-04
 *   Group 2.3 (NTL + CMO):       NTL-HYP-05
 *   Group 2.4 (G-stage fault):   NTL-HYP-06
 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_csr.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

#ifndef PLATFORM_MTIMECMP_ADDR
#define PLATFORM_MTIMECMP_ADDR  (PLATFORM_CLINT_BASE + 0x4000UL)
#endif

int main(void)
{
    uart_init();
    reset_state();

    /* PMP is opened for S/U by the boot path (common/entry.S) before it drops
     * privilege, and again by hyp_reset_state(); pmpaddr0/pmpcfg0 are M-level,
     * so writing them from here (S-mode under the T-SBI model) would raise an
     * illegal-instruction. */

    test_print_banner("RISC-V Hypervisor x Zihintntl Cross Test");

    /* Disarm the M-timer up front: mtimecmp boots at 0, so MTIP is
     * pending from reset. Any later restoration of a saved mie with
     * MTIE set would take a spurious M-timer interrupt mid-test. */
    {
        volatile uint64_t *mtimecmp =
            (volatile uint64_t *)PLATFORM_MTIMECMP_ADDR;
        *mtimecmp = (uint64_t)-1;
    }

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    /* Clean H-ext baseline before the first test. Only safe when the
     * H extension is actually implemented; each case re-checks via
     * an inline `if (!H_AVAILABLE) TEST_SKIP(...)`. */
    if (H_AVAILABLE)
    {
        hyp_reset_state();

        /* NTL-05a/05b drive the menvcfg/henvcfg CMO fields from S(HS)-mode.
         * Under Smstateen the HS-level henvcfg access is gated by
         * mstateen0.ENVCFG, which spike resets to 0, so open the gate once
         * here (hyp_reset_state clears neither mstateen0 nor hstateen0, so it
         * persists across tests). Not done in the shared baseline: the
         * Smstateen/Ssstateen suites test that gate and set it themselves. */
        hyp_enable_envcfg_access();
    }

    for (unsigned int i = 0; i < test_count; i++)
    {
        _test_table[i]();
    }

    return test_print_summary();
}
