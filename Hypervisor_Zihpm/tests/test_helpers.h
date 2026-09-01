/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zihpm test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * hpmcounter accesses use the dynamic CSR accessor (csr_read) with
 * the user-visible counter address (0xC00+N), i.e. the canonical
 * read-only CSR form (csrrs rd, csr, x0).
 */

#ifndef HYPERVISOR_ZIHPM_TEST_HELPERS_H
#define HYPERVISOR_ZIHPM_TEST_HELPERS_H

#include "test_framework.h"
#include "encoding.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_trap.h"

/* Dynamic CSR read (defined in common/csr_accessors.c) */
extern uintptr_t csr_read(uint16_t csr);

/* ===================================================================
 * Counter index range
 * =================================================================== */

#define HZHPM_FIRST     3
#define HZHPM_LAST      31

/* User-visible hpmcounter CSR address for index n (3..31). */
#define CSR_HPMCOUNTER(n)   ((uint16_t)(CSR_HPMCOUNTER3 + ((n) - 3)))

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
 * Counter implementation detection
 *
 * Strategy (identical to Shcounterenw_test_plan.md): in M-mode, write
 * a non-zero value to mhpmcounterN and read it back; if the write
 * does not stick the counter mirror is read-only zero and the counter
 * is treated as unimplemented for these tests.
 *
 * hpmcounter_is_writable() from hyp_csr.h performs exactly that probe.
 * =================================================================== */

/* Find the first unimplemented (read-only zero) hpmcounter, 3..31.
 * Returns 0 if every hpmcounter is implemented. */
static inline unsigned hzhpM_find_unimplemented(void)
{
    for (unsigned n = HZHPM_FIRST; n <= HZHPM_LAST; n++) {
        if (!hpmcounter_is_writable((int)n))
            return n;
    }
    return 0;
}

/* Find the first implemented (write-holding) hpmcounter, 3..31.
 * Returns 0 if none is implemented. */
static inline unsigned hzhpM_find_implemented(void)
{
    for (unsigned n = HZHPM_FIRST; n <= HZHPM_LAST; n++) {
        if (hpmcounter_is_writable((int)n))
            return n;
    }
    return 0;
}

/* ===================================================================
 * VS/VU-mode hpmcounter read payload
 *
 * arg = user-visible CSR address (CSR_HPMCOUNTER(n)). When the access
 * traps, the M-mode handler records the cause and skips the
 * instruction; run_in_vs/vu_mode then returns normally.
 * =================================================================== */

static uintptr_t _vs_read_hpm(uintptr_t arg)
{
    return csr_read((uint16_t)arg);
}

/* ===================================================================
 * counteren probe helpers (WARL: individual bits may be read-only)
 * =================================================================== */

/* Try to set a counteren bit; returns true iff it stuck. */
static inline bool counteren_bit_set_sticky(
    uintptr_t (*rd)(void), void (*set)(uintptr_t), uintptr_t bit)
{
    set(bit);
    return (rd() & bit) != 0;
}

/* Try to clear a counteren bit; returns true iff it stuck. */
static inline bool counteren_bit_clear_sticky(
    uintptr_t (*rd)(void), void (*clr)(uintptr_t), uintptr_t bit)
{
    clr(bit);
    return (rd() & bit) == 0;
}

#define MCNT_SET_STICK(bit) \
    counteren_bit_set_sticky(mcounteren_read, mcounteren_set, (bit))
#define MCNT_CLR_STICK(bit) \
    counteren_bit_clear_sticky(mcounteren_read, mcounteren_clear, (bit))
#define HCNT_SET_STICK(bit) \
    counteren_bit_set_sticky(hcounteren_read, hcounteren_set, (bit))
#define HCNT_CLR_STICK(bit) \
    counteren_bit_clear_sticky(hcounteren_read, hcounteren_clear, (bit))
#define SCNT_SET_STICK(bit) \
    counteren_bit_set_sticky(scounteren_read, scounteren_set, (bit))
#define SCNT_CLR_STICK(bit) \
    counteren_bit_clear_sticky(scounteren_read, scounteren_clear, (bit))

#endif /* HYPERVISOR_ZIHPM_TEST_HELPERS_H */
