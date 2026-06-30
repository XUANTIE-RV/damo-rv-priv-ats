/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Ssstateen test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Extracted from Ssstateen/tests/test_helpers.h, keeping only the
 * H-mode (Groups 7-12) relevant helpers.
 */

#ifndef HYPERVISOR_SSSTATEEN_TEST_HELPERS_H
#define HYPERVISOR_SSSTATEEN_TEST_HELPERS_H

#include "test_framework.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_trap.h"

/* ===================================================================
 * Feature detection
 * =================================================================== */

#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

#define REQUIRE_H_EXT() do { \
    if (!HAS_H_EXT()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * Probe helper: check if a bit of hstateen0 is writable.
 * =================================================================== */

static inline bool hstateen0_bit_writable(uintptr_t bit)
{
    uintptr_t saved = hstateen_read(0);
    hstateen_set_bits(0, bit);
    uintptr_t rb = hstateen_read(0);
    hstateen_write(0, saved);
    return (rb & bit) != 0;
}

static inline bool mstateen0_bit_writable(uintptr_t bit)
{
    uintptr_t saved = mstateen_read(0);
    mstateen_set_bits(0, bit);
    uintptr_t rb = mstateen_read(0);
    mstateen_write(0, saved);
    return (rb & bit) != 0;
}

/* ===================================================================
 * VS-mode callback functions (invoked via run_in_vs_mode)
 * =================================================================== */

static uintptr_t _vs_read_sstateen0(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, 0x10C" : "=r"(val));
    return val;
}

static uintptr_t _vs_write_sstateen0(uintptr_t arg)
{
    asm volatile("csrw 0x10C, %0" :: "r"(arg));
    return 0;
}

static uintptr_t _vs_read_sstateen(uintptr_t idx)
{
    uintptr_t val;
    switch ((int)idx)
    {
    case 0: asm volatile("csrr %0, 0x10C" : "=r"(val)); break;
    case 1: asm volatile("csrr %0, 0x10D" : "=r"(val)); break;
    case 2: asm volatile("csrr %0, 0x10E" : "=r"(val)); break;
    case 3: asm volatile("csrr %0, 0x10F" : "=r"(val)); break;
    default: val = 0; break;
    }
    return val;
}

static uintptr_t _vs_read_senvcfg(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, 0x10A" : "=r"(val));
    return val;
}

static uintptr_t _vs_write_senvcfg(uintptr_t arg)
{
    asm volatile("csrw 0x10A, %0" :: "r"(arg));
    return 0;
}

/* siselect (CSR 0x150) */
static uintptr_t _vs_read_siselect(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, 0x150" : "=r"(val));
    return val;
}

/* sireg (CSR 0x151) */
static uintptr_t _vs_read_sireg(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, 0x151" : "=r"(val));
    return val;
}

/* stopei (CSR 0x15C) */
static uintptr_t _vs_read_stopei(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, 0x15C" : "=r"(val));
    return val;
}

/* scontext (CSR 0x5A8) */
static uintptr_t _vs_read_scontext(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, 0x5A8" : "=r"(val));
    return val;
}

static uintptr_t _vs_write_scontext(uintptr_t arg)
{
    asm volatile("csrw 0x5A8, %0" :: "r"(arg));
    return 0;
}

#endif /* HYPERVISOR_SSSTATEEN_TEST_HELPERS_H */
