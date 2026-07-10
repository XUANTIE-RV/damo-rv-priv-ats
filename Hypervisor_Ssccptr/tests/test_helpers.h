/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Hypervisor × Ssccptr cross tests
 *
 * Provides H extension detection, PMA configuration capability detection,
 * and VS-mode test trampolines.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 */

#ifndef HYPERVISOR_SSCCPTR_TEST_HELPERS_H
#define HYPERVISOR_SSCCPTR_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_fence.h"
#include "hyp/hyp_ldst.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/test_vs_helpers.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/hyp_platform.h"

/* ===================================================================
 * Linker-provided test-region symbols (see Hypervisor_Ssccptr/kernel.ld).
 * =================================================================== */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t test_exec_page[];
extern uint8_t test_exec_target[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

#define TEST_REGION_BASE   ((uintptr_t)__vm_test_region_start)

/* ===================================================================
 * Guest-page-fault cause codes (from RISC-V Privileged Spec)
 * =================================================================== */
#define CAUSE_INST_GUEST_PAGE_FAULT    20
#define CAUSE_LOAD_GUEST_PAGE_FAULT    21
#define CAUSE_STORE_GUEST_PAGE_FAULT   23

/* ===================================================================
 * H extension detection
 * =================================================================== */
static bool check_h_extension(void) {
    uint64_t misa = CSRR(misa);
    if (!(misa & (1UL << ('H' - 'A')))) {
        return false;
    }
    return true;
}

#define H_REQUIRED_OR_SKIP() do { \
    if (!check_h_extension()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * PMA configuration capability detection
 *
 * Ssccptr is a PMA-level constraint extension. Most platforms (QEMU,
 * Spike, and general-purpose hardware) do not support dynamic PMA
 * attribute configuration. This helper detects whether the platform
 * supports such configuration for HCROSS-SSCCPTR-04.
 *
 * Currently, we assume no platform supports dynamic PMA configuration
 * unless explicitly detected. This can be extended in the future if
 * specific platforms provide PMA configuration interfaces.
 * =================================================================== */
static bool platform_supports_pma_config(void) {
    /*
     * Most simulation platforms (QEMU, Spike) and general-purpose hardware
     * do not support dynamic PMA attribute configuration. PMA attributes
     * are typically hardwired at design time.
     *
     * If a platform provides PMA configuration registers or interfaces,
     * this function should be extended to detect them.
     *
     * For now, return false to trigger TEST_SKIP in HCROSS-SSCCPTR-04.
     */
    return false;
}

#define PMA_CONFIG_REQUIRED_OR_SKIP() do { \
    if (!platform_supports_pma_config()) { \
        TEST_SKIP("Platform does not support dynamic PMA configuration"); \
    } \
} while (0)

/* ===================================================================
 * VS-mode test trampolines
 * =================================================================== */

/* VS-mode load: returns 0 on success, cause on trap.
 * The loaded value is stored in vs_loaded_value so the M-mode
 * caller can verify data correctness after the call returns. */
static uintptr_t vs_loaded_value;

static uintptr_t vs_load(uintptr_t arg) {
    trap_expect_begin();
    vs_loaded_value = *(volatile uintptr_t *)arg;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode store: returns 0 on success, cause on trap */
static uintptr_t vs_store(uintptr_t arg) {
    trap_expect_begin();
    *(volatile uintptr_t *)arg = 0xDEADBEEF;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

#endif /* HYPERVISOR_SSCCPTR_TEST_HELPERS_H */
