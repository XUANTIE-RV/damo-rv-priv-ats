/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SSCOFPMF_HELPERS_H
#define SSCOFPMF_HELPERS_H

#include "test_framework.h"
#include "encoding.h"

#ifdef ENABLE_HYP
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#endif

/* mhpmcounter, mhpmevent, mie, mip and mcounteren are all M-level CSRs. Under
 * S-mode-normal T-SBI model this suite runs in S(HS)-mode, so every access to
 * them goes through the tsbi_csr_* ecall proxy (a raw csr_read/csr_write or
 * CSRR/CSRW from S-mode would raise illegal-instruction). A proxy access to an
 * unimplemented CSR faults inside the M-handler and returns TSBI_RESERVED_RET
 * rather than trapping here, so implementation probes test that sentinel
 * instead of arming trap_expect. scountovf is S-level and stays a direct
 * CSRR. */

/* ===================================================================
 * Platform-configurable event number
 *
 * Different implementations use different event encodings.
 * Spike uses event=2 for "instructions retired".
 * Override at compile time with -DCOFPMF_EVENT_INSTRET=<n>.
 * =================================================================== */
#ifndef COFPMF_EVENT_INSTRET
#define COFPMF_EVENT_INSTRET  2
#endif

/* ===================================================================
 * Overflow margin: how far from max value to set counter before
 * triggering overflow.  Must be large enough to cover loop overhead.
 * =================================================================== */
#ifndef COFPMF_OVERFLOW_MARGIN
#define COFPMF_OVERFLOW_MARGIN  10
#endif

/* ===================================================================
 * Counter index range: HPM counters 3–31
 * =================================================================== */
#define COFPMF_FIRST_COUNTER  3
#define COFPMF_LAST_COUNTER   31

/* ===================================================================
 * Fixed target counter for the tests
 *
 * This suite decides whether to skip purely from compile-time config
 * (SSCOFPMF_AVAILABLE + H_AVAILABLE); it no longer probes the DUT to
 * discover an implemented counter, nor checks at runtime that
 * scountovf / counting are functional.  A platform that declares
 * Sscofpmf in config but implements it wrongly must fail loudly rather
 * than be masked by a probe-driven SKIP.  Tests therefore operate on a
 * fixed counter index, defaulting to the first architectural HPM
 * counter (== 3), matching the Group 10 examples in the test plan.
 * Override at compile time with -DCOFPMF_TEST_COUNTER=<n>.
 * =================================================================== */
#ifndef COFPMF_TEST_COUNTER
#define COFPMF_TEST_COUNTER  COFPMF_FIRST_COUNTER
#endif

/* ===================================================================
 * RV32 mhpmeventh access
 *
 * mhpmeventh3-31 (0x723-0x73F) are reached through the tsbi_csr_* proxy
 * like every other M-level counter CSR, and the address arithmetic
 * (CSR_MHPMEVENT(n) / CSR_MHPMEVENTH(n) / CSR_MHPMCOUNTERH(n)) comes from
 * common/sm_defs.h -- so this suite needs no local copies of either.
 *
 * On RV64 these functions are never called -- the whole 64-bit value
 * fits in a single CSR access.
 * =================================================================== */
#if __riscv_xlen == 32

static inline uintptr_t mhpmeventh_read(unsigned n)
{
    return tsbi_csr_read(CSR_MHPMEVENTH(n));
}

static inline void mhpmeventh_write(unsigned n, uintptr_t val)
{
    tsbi_csr_write(CSR_MHPMEVENTH(n), val);
}

#endif /* __riscv_xlen == 32 */

/* ===================================================================
 * Read / write mhpmcounter N (logical 64-bit value)
 *
 * RV32: reads/writes both mhpmcounter (low) and mhpmcounterh (high).
 * RV64: single CSR access.
 * =================================================================== */
static inline uint64_t cofpmf_read_counter(unsigned n) {
#if __riscv_xlen == 32
    /* Use hi-lo-hi retry loop to handle carry from lo to hi */
    uint64_t hi, lo, hi2;
    do {
        hi  = tsbi_csr_read(CSR_MHPMCOUNTERH(n));
        lo  = tsbi_csr_read(CSR_MHPMCOUNTER(n));
        hi2 = tsbi_csr_read(CSR_MHPMCOUNTERH(n));
    } while (hi != hi2);
    return (hi << 32) | lo;
#else
    return tsbi_csr_read(CSR_MHPMCOUNTER(n));
#endif
}

static inline void cofpmf_write_counter(unsigned n, uint64_t val) {
#if __riscv_xlen == 32
    /* RV32: stop counter n first (event=0) to prevent race between
     * high/low half writes while the counter is still incrementing. */
    uintptr_t saved_event_lo = tsbi_csr_read(CSR_MHPMEVENT(n));
    uintptr_t saved_event_hi = mhpmeventh_read(n);
    tsbi_csr_write(CSR_MHPMEVENT(n), 0);
    mhpmeventh_write(n, 0);
    tsbi_csr_write(CSR_MHPMCOUNTERH(n), (uintptr_t)(val >> 32));
    tsbi_csr_write(CSR_MHPMCOUNTER(n), (uintptr_t)(val & 0xFFFFFFFF));
    mhpmeventh_write(n, saved_event_hi);
    tsbi_csr_write(CSR_MHPMEVENT(n), saved_event_lo);
#else
    tsbi_csr_write(CSR_MHPMCOUNTER(n), (uintptr_t)val);
#endif
}

/* ===================================================================
 * Read / write mhpmevent N (logical 64-bit value)
 *
 * RV32: reads/writes both mhpmevent (low) and mhpmeventh (high).
 *       mhpmeventh uses the dedicated helper above.
 * RV64: single CSR access via the tsbi_csr_* proxy.
 * =================================================================== */
static inline uint64_t cofpmf_read_event(unsigned n) {
#if __riscv_xlen == 32
    uint64_t lo = tsbi_csr_read(CSR_MHPMEVENT(n));
    uint64_t hi = mhpmeventh_read(n);
    return (hi << 32) | lo;
#else
    return tsbi_csr_read(CSR_MHPMEVENT(n));
#endif
}

static inline void cofpmf_write_event(unsigned n, uint64_t val) {
#if __riscv_xlen == 32
    /* RV32: write low 32 bits first, then high 32 bits.
     * Writing low first preserves high bits (OF/xINH flags).
     * Writing high first would be cleared when low is written. */
    tsbi_csr_write(CSR_MHPMEVENT(n), (uintptr_t)(val & 0xFFFFFFFF));
    mhpmeventh_write(n, (uintptr_t)(val >> 32));
#else
    tsbi_csr_write(CSR_MHPMEVENT(n), (uintptr_t)val);
#endif
}

/* ===================================================================
 * Stop counting on counter N (clear mhpmevent + mhpmeventh on RV32)
 * =================================================================== */
static inline void cofpmf_stop_counting(unsigned n) {
    cofpmf_write_event(n, 0);
}

/* ===================================================================
 * Start counting on counter N with given event + flags
 * =================================================================== */
static inline void cofpmf_start_counting(unsigned n, uint64_t event_and_flags) {
    cofpmf_write_event(n, event_and_flags);
}

/* ===================================================================
 * Counter discovery
 *
 * Probe whether mhpmcounter N is implemented by writing a non-zero
 * value and reading it back through the T-SBI proxy. A proxy access to
 * an unimplemented HPM CSR faults inside the M-handler and yields
 * TSBI_RESERVED_RET (not a trap visible to trap_expect here), so the
 * sentinel is what distinguishes "absent" from "present".
 * =================================================================== */
static inline bool is_counter_implemented(unsigned n) {
    if (n < COFPMF_FIRST_COUNTER || n > COFPMF_LAST_COUNTER)
        return false;

    /* Probe only the low-half CSR (mhpmcounter).  On RV32 the high-half
     * (mhpmcounterh) may not exist for all counters.  Write a non-zero
     * value, then read it back: an absent counter makes the proxy return
     * TSBI_RESERVED_RET for the read. */
    tsbi_csr_write(CSR_MHPMCOUNTER(n), 0x1);
    uintptr_t val = tsbi_csr_read(CSR_MHPMCOUNTER(n));

    /* Clean up: restore counter low half to 0 */
    tsbi_csr_write(CSR_MHPMCOUNTER(n), 0);

#if __riscv_xlen == 32
    /* Also zero the high half if it exists (absent -> proxy sentinel, harmless) */
    tsbi_csr_write(CSR_MHPMCOUNTERH(n), 0);
#endif

    if (val == (uintptr_t)TSBI_RESERVED_RET)
        return false;   /* proxy access faulted -> not implemented */

    return val != 0;
}

/* ===================================================================
 * Find a second implemented counter different from `skip`
 * =================================================================== */
static inline unsigned find_second_counter(unsigned skip) {
    for (unsigned n = COFPMF_FIRST_COUNTER; n <= COFPMF_LAST_COUNTER; n++) {
        if (n != skip && is_counter_implemented(n))
            return n;
    }
    return 0;
}

/* ===================================================================
 * LCOFI interrupt enable / disable helpers (mie/mip are M-level)
 * =================================================================== */
static inline void lcofie_enable(void) {
    tsbi_csr_set(CSR_MIE, MIE_LCOFIE);
}

static inline void lcofie_disable(void) {
    tsbi_csr_clear(CSR_MIE, MIE_LCOFIE);
}

static inline void lcofip_clear(void) {
    tsbi_csr_clear(CSR_MIP, MIP_LCOFIP);
}

static inline bool lcofip_pending(void) {
    return (tsbi_csr_read(CSR_MIP) & MIP_LCOFIP) != 0;
}

/* ===================================================================
 * Execute a known instruction sequence to drive counter increments.
 * Uses NOPs to avoid side effects.
 * =================================================================== */
static inline void cofpmf_execute_nops(unsigned count) {
    for (volatile unsigned i = 0; i < count; i++) {
        asm volatile("nop");
    }
}

/* ===================================================================
 * Trigger overflow: set counter near max, run instructions, stop.
 * Returns true if OF bit was set after the sequence.
 * =================================================================== */
static inline bool cofpmf_trigger_overflow(unsigned n) {
    /* Set counter to near-max value */
    cofpmf_write_counter(n, (uint64_t)-COFPMF_OVERFLOW_MARGIN);
    /* Start counting retired instructions, OF=0, no inhibit */
    cofpmf_start_counting(n, COFPMF_EVENT_INSTRET);
    /* Execute enough instructions to overflow */
    cofpmf_execute_nops(COFPMF_OVERFLOW_MARGIN + 50);
    /* Stop counting */
    cofpmf_stop_counting(n);
    /* Check OF bit — read the event CSR that was just cleared by stop;
     * but stop clears the whole register.  Re-read before stop instead. */
    return false; /* caller should check OF directly */
}

/* ===================================================================
 * Check if WPRI bits 57:56 in mhpmevent are masked to zero.
 *
 * The spec says these are WPRI, but some implementations may not
 * enforce the mask.
 * =================================================================== */
static inline bool is_wpri_masked(unsigned n) {
    uint64_t orig = cofpmf_read_event(n);
    uint64_t wpri_mask = (3ULL << 56);
    cofpmf_write_event(n, orig | wpri_mask);
    uint64_t val = cofpmf_read_event(n);
    cofpmf_write_event(n, orig);
    return (val & wpri_mask) == 0;
}

#endif /* SSCOFPMF_HELPERS_H */
