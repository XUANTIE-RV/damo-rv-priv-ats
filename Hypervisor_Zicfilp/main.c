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
#include "hyp/hyp_csr.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void) {
    uart_init();
    reset_state();

    /* PMP is opened for S/U by the boot path (common/entry.S) before it drops
     * privilege, and again by hyp_reset_state(); pmpaddr0/pmpcfg0 are M-level,
     * so writing them from here (S-mode under the T-SBI model) would raise an
     * illegal-instruction. */

    test_print_banner("Hypervisor x Zicfilp Cross Test");

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    /* Clean H-ext baseline before the first test. */
    hyp_reset_state();

    /* This suite drives menvcfg.LPE and henvcfg/senvcfg from S(HS)-mode and
     * from VS/VU. Under Smstateen the HS-level henvcfg/senvcfg accesses are
     * gated by mstateen0.ENVCFG and the VS/VU ones by hstateen0.ENVCFG, both
     * reset to 0, so open the gates once here (hyp_reset_state clears neither
     * mstateen0 nor hstateen0, so they persist across tests). Not done in the
     * shared baseline: the Smstateen/Ssstateen suites test that gate and set
     * it themselves. */
    hyp_enable_envcfg_access();

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
