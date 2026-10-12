/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor × Sscsrind Cross Test entry point
 *
 * Verifies Sscsrind extension behavior in Hypervisor scenarios:
 *   - VS-level CSR (vsiselect/vsireg*) basic functionality
 *   - Virtual-instruction exception behavior
 *   - hstateen0[60] state-enable control
 *   - VS-mode transparent remapping via sireg*
 *
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 11.
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
    test_print_banner("Hypervisor × Sscsrind Cross Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    /* Clean H-ext baseline before the first test. */
    hyp_reset_state();

    /* This suite reads/writes hstateen0 (bit 60 CSRIND) from S(HS)-mode. Under
     * Smstateen, HS-mode access to hstateen0 is gated by mstateen0.SE0 (bit 63),
     * which spike resets to 0, so hstateen_read() would trap. Open the stateen
     * master gate once here (hyp_reset_state does not clear mstateen0, so it
     * persists). Not done in the shared baseline: the Smstateen/Ssstateen
     * suites test that gate and set it themselves. */
    hyp_enable_envcfg_access();

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
