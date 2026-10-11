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
#include "csr_accessors.h"

/* ===================================================================
 * Counter index range
 * =================================================================== */

#define HZHPM_FIRST     3
#define HZHPM_LAST      31

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
 *
 * One pair of helpers per CSR instead of a single function-pointer generic:
 * mcounteren is M-level, so under the S-mode-normal (T-SBI) model the test
 * body in S(HS)-mode must reach it through the tsbi_csr_* proxy (a bare
 * csrr/csrw mcounteren raises illegal-instruction), while hcounteren and
 * scounteren are HS/S-level and keep the direct accessors from hyp_csr.h.
 * Spelling the CSR number at each access keeps the M-level path explicit --
 * the tsbi_csr_* primitives take the CSR as an argument, so they cannot be
 * passed as the zero-argument / one-argument function pointers a generic
 * helper would need, and binding them behind suite-local read/set/clear
 * adapters would resurrect the named M-level accessor family that was
 * deliberately removed repo-wide.  Same shape as
 * Hypervisor_Zicntr/tests/test_helpers.h.
 * =================================================================== */

/* Try to set an mcounteren bit; returns true iff it stuck. */
static inline bool mcounteren_bit_set_sticky(uintptr_t bit)
{
    tsbi_csr_set(CSR_MCOUNTEREN, bit);
    return (tsbi_csr_read(CSR_MCOUNTEREN) & bit) != 0;
}

/* Try to clear an mcounteren bit; returns true iff it stuck. */
static inline bool mcounteren_bit_clear_sticky(uintptr_t bit)
{
    tsbi_csr_clear(CSR_MCOUNTEREN, bit);
    return (tsbi_csr_read(CSR_MCOUNTEREN) & bit) == 0;
}

/* Try to set an hcounteren bit; returns true iff it stuck. */
static inline bool hcounteren_bit_set_sticky(uintptr_t bit)
{
    hcounteren_set(bit);
    return (hcounteren_read() & bit) != 0;
}

/* Try to clear an hcounteren bit; returns true iff it stuck. */
static inline bool hcounteren_bit_clear_sticky(uintptr_t bit)
{
    hcounteren_clear(bit);
    return (hcounteren_read() & bit) == 0;
}

/* Try to set a scounteren bit; returns true iff it stuck. */
static inline bool scounteren_bit_set_sticky(uintptr_t bit)
{
    scounteren_set(bit);
    return (scounteren_read() & bit) != 0;
}

/* Try to clear a scounteren bit; returns true iff it stuck. */
static inline bool scounteren_bit_clear_sticky(uintptr_t bit)
{
    scounteren_clear(bit);
    return (scounteren_read() & bit) == 0;
}

#define MCNT_SET_STICK(bit)  mcounteren_bit_set_sticky(bit)
#define MCNT_CLR_STICK(bit)  mcounteren_bit_clear_sticky(bit)
#define HCNT_SET_STICK(bit)  hcounteren_bit_set_sticky(bit)
#define HCNT_CLR_STICK(bit)  hcounteren_bit_clear_sticky(bit)
#define SCNT_SET_STICK(bit)  scounteren_bit_set_sticky(bit)
#define SCNT_CLR_STICK(bit)  scounteren_bit_clear_sticky(bit)

#endif /* HYPERVISOR_ZIHPM_TEST_HELPERS_H */
