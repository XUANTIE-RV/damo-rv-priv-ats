/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Sscofpmf Cross Test entry point
 *
 * Verifies Sscofpmf extension behavior in Hypervisor scenarios:
 *   - VS-mode scountovf dual gating (mcounteren + hcounteren)
 *   - VSINH/VUINH counting inhibition in VS/VU-mode
 *   - VSINH/VUINH read-only zero when H extension is absent
 *
 * See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 10.
 */

#include "test_framework.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_csr.h"
#include "sscofpmf_helpers.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* Initialize PMU: allow S/U-mode counter access and enable counting.
     * mcounteren is an M-level CSR; from the S(HS)-mode normal state it must
     * be written through the T-SBI proxy rather than a raw CSRW. */
    tsbi_csr_write(CSR_MCOUNTEREN, 0xFFFFFFFF);

    /* Clear mcountinhibit to enable all counters (trap-protected,
     * in case the CSR is not implemented on some platforms) */
    trap_expect_begin();
    CSRW(0x320, 0);  /* mcountinhibit */
    trap_expect_end();

    /* PMP full-space RWX for S/U is already installed by entry.S during the
     * M-mode boot phase (before dropping to S), so no per-suite PMP setup is
     * needed here; a raw csrw pmpaddr0/pmpcfg0 from S-mode would trap. */
    test_print_banner("Hypervisor x Sscofpmf Cross Test");

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
