/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HZPM_SMNPM_TEST_HELPERS_H
#define HZPM_SMNPM_TEST_HELPERS_H

/* ===================================================================
 * Shared helpers for Hypervisor x Smnpm cross tests
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 6.
 * =================================================================== */

#include "test_framework.h"
#include "encoding.h"
#include "vm/vm.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/hyp_platform.h"
#include "pm/pm_cfg.h"
#include "pm/pm_addr.h"

/* ===================================================================
 * Capability gates
 * =================================================================== */

static bool check_h_extension(void) {
    uint64_t misa_val = CSRR(misa);
    return (misa_val & (1UL << ('H' - 'A'))) != 0;
}

#define H_REQUIRED_OR_SKIP() do { \
    if (!check_h_extension()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

#define SMNPM_REQUIRED_OR_SKIP() do { \
    H_REQUIRED_OR_SKIP(); \
    if (!detect_smnpm()) { \
        TEST_SKIP("Smnpm (menvcfg.PMM) not implemented"); \
    } \
} while (0)

/* ===================================================================
 * Scratch data addresses (inside .vm_test_region, used by the
 * VS/VU cross-checks via two_stage helpers)
 * =================================================================== */

#define HZPM_DATA1      (TEST_REGION_BASE + 0x0200)

#define HZPM_MAGIC1     0xDEADBEEFCAFE1234ULL
#define HZPM_MAGIC2     0x1234567890ABCDEFULL
#define HZPM_AMO_ADDEND 42ULL

/* Set menvcfg.PMM; returns true iff the encoding read back. */
static bool hzpm_try_set_s_pmm(unsigned pmm) {
    pm_set_smode(pmm);
    return pm_get_smode() == pmm;
}

/* Set henvcfg.PMM; returns true iff the encoding read back. */
static bool hzpm_try_set_vs_pmm(unsigned pmm) {
    pm_set_vsmode(pmm);
    return pm_get_vsmode() == pmm;
}

/* ===================================================================
 * Shared HS-mode payload functions (executed via vm_run_in_smode)
 * =================================================================== */

static uintptr_t hzpm_hs_load64(uintptr_t addr) {
    return (uintptr_t)*(volatile uint64_t *)addr;
}

static uintptr_t hzpm_hs_amoadd64(uintptr_t addr) {
    uint64_t old_val;
    uint64_t addend = HZPM_AMO_ADDEND;
    asm volatile("amoadd.d %0, %1, (%2)"
                 : "=r"(old_val) : "r"(addend), "r"(addr) : "memory");
    return (uintptr_t)old_val;
}

/* Shared VS/VU-mode payload (executed via two_stage_run_in_vs/vu) */
static uintptr_t hzpm_load64(uintptr_t addr) {
    return (uintptr_t)*(volatile uint64_t *)addr;
}

/* HS-mode (S-mode, V=0) Sv39 identity VM setup */
static void hzpm_setup_hs_vm(pt_context_t *ctx) {
    pt_pool_reset();
    pt_init(ctx, SATP_MODE_SV39);
    pt_setup_identity_mapping(ctx, PLATFORM_MEM_BASE, PLATFORM_MEM_SIZE,
        PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D, PT_LEVEL_2M);
}

#endif /* HZPM_SMNPM_TEST_HELPERS_H */
