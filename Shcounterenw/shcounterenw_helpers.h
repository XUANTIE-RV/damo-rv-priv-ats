/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHCOUNTERENW_HELPERS_H
#define SHCOUNTERENW_HELPERS_H

/*
 * shcounterenw_helpers.h - Shcounterenw Test Helpers
 *
 * Provides:
 *   - Counter implementation detection (wraps framework APIs)
 *   - Counter CSR address mapping
 *   - VS/VU-mode payload functions for counter reads
 *   - H-extension detection
 *
 * Uses framework APIs from common/hyp/hyp_csr.h exclusively.
 */

#include "test_framework.h"
#include "encoding.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_test.h"

/* Dynamic CSR read/write (defined in common/csr_accessors.c) */
extern uintptr_t csr_read(uint16_t csr);
extern void csr_write(uint16_t csr, uintptr_t val);

/* ===================================================================
 * Counter index range constants
 * =================================================================== */
#define SHCNTW_FIRST_HPM  3
#define SHCNTW_LAST_HPM   31

/* ===================================================================
 * CSR address arithmetic helpers
 * =================================================================== */
#ifndef CSR_MHPMCOUNTER
#define CSR_MHPMCOUNTER(n)  (CSR_MHPMCOUNTER3 + ((n) - 3))
#endif
#ifndef CSR_HPMCOUNTER
#define CSR_HPMCOUNTER(n)   (CSR_HPMCOUNTER3  + ((n) - 3))
#endif

/* ===================================================================
 * Counter CSR address by index (0=cycle, 1=time, 2=instret, 3-31=hpm)
 *
 * Returns the user-mode read-only CSR address for counter index i.
 * =================================================================== */
static inline uint16_t counter_csr_addr(unsigned i) {
    switch (i) {
    case 0:  return CSR_CYCLE;
    case 1:  return CSR_TIME;
    case 2:  return CSR_INSTRET;
    default: return (uint16_t)(CSR_HPMCOUNTER3 + (i - 3));
    }
}

/* ===================================================================
 * Counter implementation detection
 *
 * For i=0 (cycle), i=1 (time), i=2 (instret): assume implemented
 * (almost all platforms provide these).
 * For i=3..31: use framework's hpmcounter_is_writable() which probes
 * by writing mhpmcounter and reading back.
 * =================================================================== */
static inline bool is_counter_implemented(unsigned i) {
    if (i <= 2)
        return true;  /* cycle, time, instret assumed implemented */
    if (i > SHCNTW_LAST_HPM)
        return false;
    return hpmcounter_is_writable((int)i);
}

/* ===================================================================
 * Find the first implemented HPM counter (3-31).
 * Returns 0 if none found.
 * =================================================================== */
static inline unsigned find_first_hpm_counter(void) {
    for (unsigned n = SHCNTW_FIRST_HPM; n <= SHCNTW_LAST_HPM; n++) {
        if (is_counter_implemented(n))
            return n;
    }
    return 0;
}

/* ===================================================================
 * H-extension detection
 * =================================================================== */
static inline bool has_h_extension(void) {
    uintptr_t misa = CSRR(misa);
    return (misa & (1UL << ('H' - 'A'))) != 0;
}

/* ===================================================================
 * VS-mode payload functions for counter reads
 *
 * These are passed to run_in_vs_mode() / run_in_vu_mode().
 * Each reads a specific counter CSR via inline assembly.
 * =================================================================== */
static uintptr_t vsmode_read_cycle(uintptr_t arg) {
    (void)arg;
    uintptr_t val;
    asm volatile ("csrr %0, cycle" : "=r"(val));
    return val;
}

static uintptr_t vsmode_read_time(uintptr_t arg) {
    (void)arg;
    uintptr_t val;
    asm volatile ("csrr %0, time" : "=r"(val));
    return val;
}

static uintptr_t vsmode_read_instret(uintptr_t arg) {
    (void)arg;
    uintptr_t val;
    asm volatile ("csrr %0, instret" : "=r"(val));
    return val;
}

/* Generic VS-mode counter read using dynamic CSR accessor.
 * arg = CSR address (e.g., CSR_HPMCOUNTER3). */
static uintptr_t vsmode_read_counter(uintptr_t arg) {
    return csr_read((uint16_t)arg);
}

#endif /* SHCOUNTERENW_HELPERS_H */
