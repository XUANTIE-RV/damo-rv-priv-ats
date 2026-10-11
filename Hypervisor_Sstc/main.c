/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Sstc Cross Test entry point
 *
 * Verifies Sstc extension behavior in Hypervisor scenarios:
 *   - henvcfg.STCE field control
 *   - VS-mode stimecmp/vstimecmp access control
 *   - vstimecmp CSR read/write
 *   - VSTIP synthesis logic (hip.VSTIP = hvip.VSTIP OR vstimecmp signal)
 *   - htimedelta impact on vstimecmp comparison
 *   - VS-mode timer interrupt capture
 *
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 9.
 */

#include "test_framework.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_csr.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* PMP full-space RWX for S/U is already installed by entry.S during the
     * M-mode boot phase (before dropping to S), so no per-suite PMP setup is
     * needed here; a raw csrw pmpaddr0/pmpcfg0 from S-mode would trap. */
    test_print_banner("Hypervisor x Sstc Cross Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    /* Clean H-ext baseline before the first test. */
    hyp_reset_state();

    /* This suite manipulates henvcfg from S(HS)-mode. Under Smstateen it is
     * gated by mstateen0.ENVCFG, which spike resets to 0, so open the gate once
     * here (hyp_reset_state does not clear mstateen0, so it persists across
     * tests). Not done in the shared baseline: the Smstateen/Ssstateen suites
     * test that gate and set it themselves. */
    hyp_enable_envcfg_access();

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
