/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor × Smcsrind test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 */

#ifndef HYPERVISOR_SMCSRIND_TEST_HELPERS_H
#define HYPERVISOR_SMCSRIND_TEST_HELPERS_H

#include "test_framework.h"

#ifdef ENABLE_HYP
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_csr.h"
#endif

/* ===================================================================
 * Indirect CSR accessors (vsiselect/vsireg*, miselect/mireg*) are
 * provided by common/hyp/hyp_csr.h. Only suite-specific VS-mode
 * callbacks and the stateen writable-probe below remain local.
 * =================================================================== */

/* ===================================================================
 * State-enable CSR helpers
 *
 * mstateen0 / hstateen0 read/write/set/clear accessors come from
 * common/hyp/hyp_csr.h (mstateen_read(0) / hstateen_set_bits(0, ...) etc.).
 * Only the writable-probe helper below is suite-specific.
 * =================================================================== */

/* Check if a specific bit in hstateen0 is writable */
static inline bool hstateen0_bit_writable(uintptr_t bit)
{
    uintptr_t saved = hstateen_read(0);
    hstateen_set_bits(0, bit);
    uintptr_t rb = hstateen_read(0);
    hstateen_write(0, saved);
    return (rb & bit) != 0;
}

/* ===================================================================
 * VS-mode callback functions for run_in_vs_mode()
 *
 * In VS-mode (V=1), siselect (0x150) and sireg (0x151) are
 * actually vsiselect and vsireg due to H-ext CSR remapping.
 * =================================================================== */

/* VS-mode: read siselect (CSR 0x150, really vsiselect) */
static uintptr_t _vs_read_siselect(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(CSR_SISELECT) : "=r"(val));
    return val;
}

/* VS-mode: read sireg (CSR 0x151, really vsireg) */
static uintptr_t _vs_read_sireg(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(CSR_SIREG) : "=r"(val));
    return val;
}

/* VS-mode: write siselect and read it back (returns final value) */
static uintptr_t _vs_write_and_read_siselect(uintptr_t val)
{
    asm volatile("csrw " CSR_STR(CSR_SISELECT) ", %0" :: "r"(val));
    uintptr_t rb;
    asm volatile("csrr %0, " CSR_STR(CSR_SISELECT) : "=r"(rb));
    return rb;
}

/* ===================================================================
 * Convenience macros for S-mode (HS-mode) access testing
 * =================================================================== */

/* Execute csr_stmt in S-mode and verify it traps with illegal-instruction */
#define TEST_SMODE_BLOCKED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_TRAP(msg, CAUSE_ILLEGAL_INST); \
} while (0)

/* Execute csr_stmt in S-mode and verify it does NOT trap */
#define TEST_SMODE_ALLOWED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_NO_TRAP(msg); \
} while (0)

#endif /* HYPERVISOR_SMCSRIND_TEST_HELPERS_H */
