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
 * The mhpmevent event-selector encoding is implementation-defined and the
 * SPEC never requires a particular selector to count, so the value comes
 * from the platform config: SSCOFPMF_COUNT_EVENT declared in
 * config/<platform>/platform_config.h and normalized by
 * common/capabilities.h into SSCOFPMF_COUNTING_AVAILABLE.  A platform that
 * declares no counting event has the counting-dependent cases skipped (see
 * COFPMF_REQUIRE_COUNTING below), so the fallback here is never used as a
 * stimulus.  Override with -DCOFPMF_EVENT_INSTRET=<n> for experiments.
 * =================================================================== */
#ifndef COFPMF_EVENT_INSTRET
#if SSCOFPMF_COUNTING_AVAILABLE
#define COFPMF_EVENT_INSTRET  SSCOFPMF_COUNT_EVENT
#else
#define COFPMF_EVENT_INSTRET  2
#endif
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
 * Skipping is decided by compile-time config only, in two layers:
 *   1. SSCOFPMF_AVAILABLE + H_AVAILABLE -- are Sscofpmf and the H extension
 *      under test.
 *   2. SSCOFPMF_COUNTING_AVAILABLE      -- does this platform declare an
 *      event selector that really increments mhpmcounterN.  Required by the
 *      counting-inhibition cases; see COFPMF_REQUIRE_COUNTING below.
 * Neither layer probes the DUT: a platform that declares Sscofpmf but
 * implements it wrongly still fails loudly, while a platform whose counters
 * never count simply has no counting precondition to test (an
 * implementation-defined platform fact, not non-compliance).  Tests operate
 * on a fixed counter index, defaulting to the first architectural HPM
 * counter (== 3), matching the Group 10 examples in the test plan.
 * Override at compile time with -DCOFPMF_TEST_COUNTER=<n>.
 * =================================================================== */
#ifndef COFPMF_TEST_COUNTER
#define COFPMF_TEST_COUNTER  COFPMF_FIRST_COUNTER
#endif

/* Layer-2 gate for counting-dependent cases.  The reason string states the
 * compile-time platform fact; it is never derived from a DUT probe. */
#define COFPMF_REQUIRE_COUNTING()                                       \
    if (!SSCOFPMF_COUNTING_AVAILABLE)                                   \
        TEST_SKIP("platform declares no counting HPM event "            \
                  "(implementation-defined)")

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
 * Counter-access gates: write, then read back
 *
 * mcounteren and hcounteren fields are WARL and may be read-only zero
 * (norm:mcounteren_flds_rdonly0), so writing a bit is not evidence that the
 * gate is open.  A case that asserts "VS-mode sees the real OF value" needs
 * both gates actually set, and a case that asserts "the bit is masked" needs
 * the masking gate actually clear; when the read-back disagrees the
 * precondition is unreachable and the caller skips instead of reporting a
 * scountovf mirroring failure.  mcounteren is M-level (T-SBI proxy),
 * hcounteren is HS-level (direct access).
 * =================================================================== */
static inline bool cofpmf_mcounteren_set(unsigned n) {
    tsbi_csr_write(CSR_MCOUNTEREN, tsbi_csr_read(CSR_MCOUNTEREN) | (1UL << n));
    return (tsbi_csr_read(CSR_MCOUNTEREN) & (1UL << n)) != 0;
}

static inline bool cofpmf_mcounteren_clear(unsigned n) {
    tsbi_csr_write(CSR_MCOUNTEREN, tsbi_csr_read(CSR_MCOUNTEREN) & ~(1UL << n));
    return (tsbi_csr_read(CSR_MCOUNTEREN) & (1UL << n)) == 0;
}

static inline bool cofpmf_hcounteren_set(unsigned n) {
    CSRW(hcounteren, CSRR(hcounteren) | (1UL << n));
    return (CSRR(hcounteren) & (1UL << n)) != 0;
}

static inline bool cofpmf_hcounteren_clear(unsigned n) {
    CSRW(hcounteren, CSRR(hcounteren) & ~(1UL << n));
    return (CSRR(hcounteren) & (1UL << n)) == 0;
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

#endif /* SSCOFPMF_HELPERS_H */
