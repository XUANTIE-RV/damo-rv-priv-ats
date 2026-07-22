/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HZPM_SSNPM_TEST_HELPERS_H
#define HZPM_SSNPM_TEST_HELPERS_H

/* ===================================================================
 * Shared helpers for Hypervisor x Ssnpm cross tests
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Groups 1-5 and 8.
 *
 * Provides:
 *   - Capability gates (H extension, Ssnpm, Ssnpm hyp controls)
 *   - Scratch data addresses inside .vm_test_region
 *   - Shared VS/VU-mode payload functions (tagged load/store/AMO,
 *     senvcfg/henvcfg access, ecall)
 * =================================================================== */

#include "test_framework.h"
#include "encoding.h"
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

/* Ssnpm hypervisor-side controls: henvcfg.PMM + hstatus.HUPMM */
#define SSNPM_HYP_REQUIRED_OR_SKIP() do { \
    H_REQUIRED_OR_SKIP(); \
    if (!detect_ssnpm_hyp()) { \
        TEST_SKIP("Ssnpm hyp controls (henvcfg.PMM/hstatus.HUPMM) not implemented"); \
    } \
} while (0)

#define SSNPM_REQUIRED_OR_SKIP() do { \
    H_REQUIRED_OR_SKIP(); \
    if (!detect_ssnpm()) { \
        TEST_SKIP("Ssnpm (senvcfg.PMM) not implemented"); \
    } \
} while (0)

/* ===================================================================
 * Scratch data addresses (inside .vm_test_region)
 *
 * ts2_setup_full() maps the whole region at 4KB granularity in both
 * VS-stage and G-stage (identity), so these addresses are usable as
 * VA (vsatp != Bare), GPA (vsatp == Bare) and SPA alike.
 * =================================================================== */

#define HZPM_DATA1      (TEST_REGION_BASE + 0x0200)
#define HZPM_DATA2      (TEST_REGION_BASE + 0x1000)
#define HZPM_DATA3      (TEST_REGION_BASE + 0x2000)
#define HZPM_DATA4      (TEST_REGION_BASE + 0x3000)

/* An address inside the region's 2MB superpage but beyond the mapped
 * 4KB pages: usable as "unmapped" VA/GPA for fault tests. */
#define HZPM_UNMAPPED   (TEST_REGION_END + 0x10000)

#define HZPM_MAGIC1     0xDEADBEEFCAFE1234ULL
#define HZPM_MAGIC2     0x1234567890ABCDEFULL
#define HZPM_MAGIC3     0x0F0F0F0FF0F0F0F0ULL
#define HZPM_STORE_MAGIC 0xA5A5A5A55A5A5A5AULL
#define HZPM_AMO_ADDEND 42ULL

/* ===================================================================
 * PMM configuration helpers (return false instead of skipping so
 * that TEST_SKIP stays in the test body)
 * =================================================================== */

/* Set henvcfg.PMM; returns true iff the encoding read back. */
static bool hzpm_try_set_vs_pmm(unsigned pmm) {
    pm_set_vsmode(pmm);
    return pm_get_vsmode() == pmm;
}

/* Set senvcfg.PMM; returns true iff the encoding read back. */
static bool hzpm_try_set_u_pmm(unsigned pmm) {
    pm_set_umode(pmm);
    return pm_get_umode() == pmm;
}

/* Set hstatus.HUPMM; returns true iff the encoding read back. */
static bool hzpm_try_set_hupmm(unsigned pmm) {
    pm_set_hupmm(pmm);
    return pm_get_hupmm() == pmm;
}

/* ===================================================================
 * Shared VS/VU-mode payload functions
 *
 * Executed via two_stage_run_in_vs() / two_stage_run_in_vu() /
 * run_in_vs_mode() / run_in_vu_mode(). They perform a single memory
 * or CSR operation against the (possibly tagged) address in arg.
 * =================================================================== */

static uintptr_t hzpm_load64(uintptr_t addr) {
    return (uintptr_t)*(volatile uint64_t *)addr;
}

static uintptr_t hzpm_store64(uintptr_t addr) {
    *(volatile uint64_t *)addr = HZPM_STORE_MAGIC;
    return 0;
}

static uintptr_t hzpm_amoadd64(uintptr_t addr) {
    uint64_t old_val;
    uint64_t addend = HZPM_AMO_ADDEND;
    asm volatile("amoadd.d %0, %1, (%2)"
                 : "=r"(old_val) : "r"(addend), "r"(addr) : "memory");
    return (uintptr_t)old_val;
}

/* Write senvcfg.PMM from VS-mode (norm:H_scsrs_nomatch: senvcfg has
 * no VS CSR and keeps its usual accessibility when V=1). */
static uintptr_t hzpm_write_senvcfg_pmm(uintptr_t pmm) {
    uintptr_t v = CSRR(CSR_SENVCFG);
    v &= ~SENVCFG_PMM_MASK;
    v |= ((uintptr_t)(pmm & 0x3) << SENVCFG_PMM_OFF);
    CSRW(CSR_SENVCFG, v);
    return 0;
}

/* Access henvcfg from VS-mode: must raise virtual-instruction. */
static uintptr_t hzpm_write_henvcfg(uintptr_t val) {
    CSRW(CSR_HENVCFG, val);
    return 0;
}

/* Plain ecall (environment call from VS/VU). */
static uintptr_t hzpm_ecall(uintptr_t unused) {
    (void)unused;
    asm volatile("ecall" ::: "memory");
    return 0;
}

#endif /* HZPM_SSNPM_TEST_HELPERS_H */
