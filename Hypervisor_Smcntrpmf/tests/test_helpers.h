/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Smcntrpmf cross test.
 *
 * All test files (test_csr_rw.c / test_cycle_filter.c /
 * test_instret_filter.c / test_counteren.c) are #included into
 * test_register.c, so static functions and globals defined here are
 * visible across the whole compilation unit.
 */

#ifndef HYPERVISOR_SMCNTRPMF_TEST_HELPERS_H
#define HYPERVISOR_SMCNTRPMF_TEST_HELPERS_H

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"

/* ===================================================================
 * Smcntrpmf CSR addresses and field masks
 * =================================================================== */
#define CSR_MCYCLECFG_ADDR     0x321
#define CSR_MINSTRETCFG_ADDR   0x322
#define CSR_MCOUNTEREN_ADDR    0x306
#define CSR_HCOUNTEREN_ADDR    0x606

/* xINH bit positions (shared encoding with Sscofpmf mhpmevent).
 * Bit 63 (OF) is read-only zero for mcyclecfg/minstretcfg. */
#define CYCLECFG_MINH          (1ULL << 62)
#define CYCLECFG_SINH          (1ULL << 61)
#define CYCLECFG_UINH          (1ULL << 60)
#define CYCLECFG_VSINH         (1ULL << 59)
#define CYCLECFG_VUINH         (1ULL << 58)

#define COUNTEREN_CY           (1UL << 0)

/* ===================================================================
 * mcyclecfg / minstretcfg inline CSR access helpers
 * =================================================================== */
static inline uintptr_t mcyclecfg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_MCYCLECFG_ADDR) : "=r"(v) :: "memory");
    return v;
}

static inline void mcyclecfg_write(uintptr_t v)
{
    asm volatile("csrw " CSR_STR(CSR_MCYCLECFG_ADDR) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t minstretcfg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_MINSTRETCFG_ADDR) : "=r"(v) :: "memory");
    return v;
}

static inline void minstretcfg_write(uintptr_t v)
{
    asm volatile("csrw " CSR_STR(CSR_MINSTRETCFG_ADDR) ", %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * mcounteren / hcounteren helpers
 * =================================================================== */
static inline void mcounteren_set(uintptr_t bits)
{
    asm volatile("csrs " CSR_STR(CSR_MCOUNTEREN_ADDR) ", %0" :: "r"(bits) : "memory");
}

static inline void mcounteren_clear(uintptr_t bits)
{
    asm volatile("csrc " CSR_STR(CSR_MCOUNTEREN_ADDR) ", %0" :: "r"(bits) : "memory");
}

static inline void hcounteren_write(uintptr_t v)
{
    asm volatile("csrw " CSR_STR(CSR_HCOUNTEREN_ADDR) ", %0" :: "r"(v) : "memory");
}

static inline void hcounteren_set(uintptr_t bits)
{
    asm volatile("csrs " CSR_STR(CSR_HCOUNTEREN_ADDR) ", %0" :: "r"(bits) : "memory");
}

static inline void hcounteren_clear(uintptr_t bits)
{
    asm volatile("csrc " CSR_STR(CSR_HCOUNTEREN_ADDR) ", %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * mcycle / minstret read (M-mode, 64-bit)
 * =================================================================== */
static inline uint64_t read_mcycle(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_MCYCLE) : "=r"(v) :: "memory");
    return (uint64_t)v;
}

static inline uint64_t read_minstret(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_MINSTRET) : "=r"(v) :: "memory");
    return (uint64_t)v;
}

/* ===================================================================
 * Execute a known number of NOPs
 * =================================================================== */
static inline void execute_nops(unsigned count)
{
    for (volatile unsigned i = 0; i < count; i++) {
        asm volatile("nop");
    }
}

/* ===================================================================
 * Smcntrpmf availability
 *
 * Smcntrpmf support is config-declaration driven: gate on the
 * compile-time SMCNTRPMF_AVAILABLE macro (normalized in
 * common/capabilities.h from SMCNTRPMF_SUPPORTED in rvtest_config.h).
 * Do NOT probe mcyclecfg.MINH writability at runtime.
 * =================================================================== */

/* ===================================================================
 * Check if cycle / instret counters are functional
 * =================================================================== */
static inline bool cycle_counter_functional(void)
{
    uint64_t start = read_mcycle();
    execute_nops(100);
    uint64_t end = read_mcycle();
    return end > start;
}

static inline bool instret_counter_functional(void)
{
    uint64_t start = read_minstret();
    execute_nops(100);
    uint64_t end = read_minstret();
    return end > start;
}

/* ===================================================================
 * VS/VU-mode trampoline functions for run_in_vs_mode()/run_in_vu_mode()
 *
 * These run with V=1. They execute a fixed NOP loop so the caller can
 * measure the cycle/instret delta contributed by VS/VU-mode execution.
 * The argument is the number of NOP iterations.
 * =================================================================== */
static uintptr_t _v_exec_nops(uintptr_t arg)
{
    execute_nops((unsigned)arg);
    return 0;
}

/* VS-mode: read the cycle CSR, return trap cause (0 if no trap).
 * Used to verify hcounteren-based access control. */
static uintptr_t _vs_read_cycle(uintptr_t arg)
{
    (void)arg;
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_CYCLE) : "=r"(v) :: "memory");  /* cycle */
    return trap_get_cause();
}

#endif /* HYPERVISOR_SMCNTRPMF_TEST_HELPERS_H */
