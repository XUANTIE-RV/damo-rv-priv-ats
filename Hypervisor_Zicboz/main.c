/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Hypervisor x Zicboz Cross Test entry point
 *
 * See DOCS/testplan/Hypervisor_CMO_test_plan.md Groups 3, 4, 5, 6.
 *
 * This suite verifies the Zicboz extension (cbo.zero) behavior
 * under Hypervisor control:
 *   - henvcfg.CBZE control of VS/VU-mode cbo.zero
 *   - htinst/mtinst standard transformation for cbo.zero
 *   - G-stage address translation faults for cbo.zero
 *   - virtual-instruction stval behavior for cbo.zero
 */

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_csr.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    test_print_banner("Hypervisor x Zicboz Cross Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    /* Clean H-ext baseline before the first test. */
    hyp_reset_state();

    /* This suite drives the menvcfg/henvcfg/senvcfg CMO fields from
     * S(HS)-mode. Under Smstateen the HS-level henvcfg/senvcfg accesses are
     * gated by mstateen0.ENVCFG, which spike resets to 0, so open the gate
     * once here (hyp_reset_state clears neither mstateen0 nor hstateen0, so
     * it persists across tests). Not done in the shared baseline: the
     * Smstateen/Ssstateen suites test that gate and set it themselves. */
    hyp_enable_envcfg_access();

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
