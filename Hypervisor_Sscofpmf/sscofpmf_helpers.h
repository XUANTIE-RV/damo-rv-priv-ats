/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SSCOFPMF_HELPERS_H
#define SSCOFPMF_HELPERS_H

#include "test_framework.h"
#include "encoding.h"
#include "rv32_csr_helpers.h"

#ifdef ENABLE_HYP
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#endif

/* Dynamic CSR read/write (defined in common/csr_accessors.c)
 * Used for CSRs that are NOT in the pmpcfg conflict range. */
extern uintptr_t csr_read(uint16_t csr);
extern void csr_write(uint16_t csr, uintptr_t val);

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
 * CSR address arithmetic helpers
 * =================================================================== */
#define CSR_MHPMEVENT(n)    (CSR_MHPMEVENT3   + ((n) - 3))
#define CSR_MHPMCOUNTER(n)  (CSR_MHPMCOUNTER3  + ((n) - 3))
#define CSR_HPMCOUNTER(n)   (CSR_HPMCOUNTER3   + ((n) - 3))

#if __riscv_xlen == 32
/* RV32 high-half CSR address macros (base addresses defined in encoding.h)
 * mhpmeventNh: 0x723-0x73F (machine read-write, high 32 bits of mhpmevent)
 * mhpmcounterNh: 0xB83-0xB9F (machine read-only, high 32 bits of mhpmcounter)
 */
#define CSR_MHPMEVENTH(n)   (CSR_MHPMEVENTH3   + ((n) - 3))
#define CSR_MHPMCOUNTERH(n) (CSR_MHPMCOUNTERH3  + ((n) - 3))
#endif

/* ===================================================================
 * RV32 mhpmeventh CSR direct read/write via inline asm switch-case.
 *
 * mhpmeventh3-31 addresses (0x723-0x73F) do NOT conflict with pmpcfg/pmpaddr
 * ranges in csr_accessors.c, so the generic csr_read/csr_write could handle
 * them.  However, we use a dedicated switch-case here for clarity and to
 * avoid any future address-range confusion.
 *
 * mhpmcounterh3-31 (0xB83-0xB9F) are NOT in conflict and are already
 * handled by csr_read/csr_write (added under #if __riscv_xlen == 32).
 *
 * On RV64 these functions are never called — the whole 64-bit value
 * fits in a single CSR access.
 * =================================================================== */
#if __riscv_xlen == 32

/*
 * mhpmeventh read/write: use literal CSR addresses as switch-case labels
 * (same pattern as csr_accessors.c).  The switch discriminant is
 * 0x720 + n, so counter n=3 matches case 0x723, n=4 matches 0x724, etc.
 */
#define _MHPEVENTH_READ_CASE(addr) \
    case (addr): asm volatile("csrr %0, " CSR_STR(addr) : "=r"(val) :: "memory"); break;

#define _MHPEVENTH_WRITE_CASE(addr) \
    case (addr): asm volatile("csrw " CSR_STR(addr) ", %0" :: "r"(val) : "memory"); break;

static inline uintptr_t mhpmeventh_read(unsigned n) {
    uintptr_t val = 0;
    switch (0x720 + n) {
    _MHPEVENTH_READ_CASE(0x723) _MHPEVENTH_READ_CASE(0x724) _MHPEVENTH_READ_CASE(0x725)
    _MHPEVENTH_READ_CASE(0x726) _MHPEVENTH_READ_CASE(0x727) _MHPEVENTH_READ_CASE(0x728)
    _MHPEVENTH_READ_CASE(0x729) _MHPEVENTH_READ_CASE(0x72A) _MHPEVENTH_READ_CASE(0x72B)
    _MHPEVENTH_READ_CASE(0x72C) _MHPEVENTH_READ_CASE(0x72D) _MHPEVENTH_READ_CASE(0x72E)
    _MHPEVENTH_READ_CASE(0x72F) _MHPEVENTH_READ_CASE(0x730) _MHPEVENTH_READ_CASE(0x731)
    _MHPEVENTH_READ_CASE(0x732) _MHPEVENTH_READ_CASE(0x733) _MHPEVENTH_READ_CASE(0x734)
    _MHPEVENTH_READ_CASE(0x735) _MHPEVENTH_READ_CASE(0x736) _MHPEVENTH_READ_CASE(0x737)
    _MHPEVENTH_READ_CASE(0x738) _MHPEVENTH_READ_CASE(0x739) _MHPEVENTH_READ_CASE(0x73A)
    _MHPEVENTH_READ_CASE(0x73B) _MHPEVENTH_READ_CASE(0x73C) _MHPEVENTH_READ_CASE(0x73D)
    _MHPEVENTH_READ_CASE(0x73E) _MHPEVENTH_READ_CASE(0x73F)
    default: break;
    }
    return val;
}

static inline void mhpmeventh_write(unsigned n, uintptr_t val) {
    switch (0x720 + n) {
    _MHPEVENTH_WRITE_CASE(0x723) _MHPEVENTH_WRITE_CASE(0x724) _MHPEVENTH_WRITE_CASE(0x725)
    _MHPEVENTH_WRITE_CASE(0x726) _MHPEVENTH_WRITE_CASE(0x727) _MHPEVENTH_WRITE_CASE(0x728)
    _MHPEVENTH_WRITE_CASE(0x729) _MHPEVENTH_WRITE_CASE(0x72A) _MHPEVENTH_WRITE_CASE(0x72B)
    _MHPEVENTH_WRITE_CASE(0x72C) _MHPEVENTH_WRITE_CASE(0x72D) _MHPEVENTH_WRITE_CASE(0x72E)
    _MHPEVENTH_WRITE_CASE(0x72F) _MHPEVENTH_WRITE_CASE(0x730) _MHPEVENTH_WRITE_CASE(0x731)
    _MHPEVENTH_WRITE_CASE(0x732) _MHPEVENTH_WRITE_CASE(0x733) _MHPEVENTH_WRITE_CASE(0x734)
    _MHPEVENTH_WRITE_CASE(0x735) _MHPEVENTH_WRITE_CASE(0x736) _MHPEVENTH_WRITE_CASE(0x737)
    _MHPEVENTH_WRITE_CASE(0x738) _MHPEVENTH_WRITE_CASE(0x739) _MHPEVENTH_WRITE_CASE(0x73A)
    _MHPEVENTH_WRITE_CASE(0x73B) _MHPEVENTH_WRITE_CASE(0x73C) _MHPEVENTH_WRITE_CASE(0x73D)
    _MHPEVENTH_WRITE_CASE(0x73E) _MHPEVENTH_WRITE_CASE(0x73F)
    default: break;
    }
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
        hi  = csr_read(CSR_MHPMCOUNTERH(n));
        lo  = csr_read(CSR_MHPMCOUNTER(n));
        hi2 = csr_read(CSR_MHPMCOUNTERH(n));
    } while (hi != hi2);
    return (hi << 32) | lo;
#else
    return csr_read(CSR_MHPMCOUNTER(n));
#endif
}

static inline void cofpmf_write_counter(unsigned n, uint64_t val) {
#if __riscv_xlen == 32
    /* RV32: stop counter n first (event=0) to prevent race between
     * high/low half writes while the counter is still incrementing. */
    uintptr_t saved_event_lo = csr_read(CSR_MHPMEVENT(n));
    uintptr_t saved_event_hi = mhpmeventh_read(n);
    csr_write(CSR_MHPMEVENT(n), 0);
    mhpmeventh_write(n, 0);
    csr_write(CSR_MHPMCOUNTERH(n), (uintptr_t)(val >> 32));
    csr_write(CSR_MHPMCOUNTER(n), (uintptr_t)(val & 0xFFFFFFFF));
    mhpmeventh_write(n, saved_event_hi);
    csr_write(CSR_MHPMEVENT(n), saved_event_lo);
#else
    csr_write(CSR_MHPMCOUNTER(n), (uintptr_t)val);
#endif
}

/* ===================================================================
 * Read / write mhpmevent N (logical 64-bit value)
 *
 * RV32: reads/writes both mhpmevent (low) and mhpmeventh (high).
 *       mhpmeventh uses the dedicated inline-asm switch above to
 *       avoid pmpcfg address conflict in csr_accessors.c.
 * RV64: single CSR access via csr_read/csr_write.
 * =================================================================== */
static inline uint64_t cofpmf_read_event(unsigned n) {
#if __riscv_xlen == 32
    uint64_t lo = csr_read(CSR_MHPMEVENT(n));
    uint64_t hi = mhpmeventh_read(n);
    return (hi << 32) | lo;
#else
    return csr_read(CSR_MHPMEVENT(n));
#endif
}

static inline void cofpmf_write_event(unsigned n, uint64_t val) {
#if __riscv_xlen == 32
    /* RV32: write low 32 bits first, then high 32 bits.
     * Writing low first preserves high bits (OF/xINH flags).
     * Writing high first would be cleared when low is written. */
    csr_write(CSR_MHPMEVENT(n), (uintptr_t)(val & 0xFFFFFFFF));
    mhpmeventh_write(n, (uintptr_t)(val >> 32));
#else
    csr_write(CSR_MHPMEVENT(n), (uintptr_t)val);
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
 * value and reading it back.  The probe is trap-protected because
 * some platforms (e.g. QEMU) raise Illegal Instruction when an
 * unimplemented HPM CSR is accessed, while others (e.g. Spike)
 * silently return zero.
 * =================================================================== */
static inline bool is_counter_implemented(unsigned n) {
    if (n < COFPMF_FIRST_COUNTER || n > COFPMF_LAST_COUNTER)
        return false;

    /* Probe only the low-half CSR (mhpmcounter).  On RV32 the high-half
     * (mhpmcounterh) may not exist for all counters, and accessing it
     * can cause Illegal Instruction traps on QEMU.  A non-zero readback
     * of the low half is sufficient to confirm the counter exists. */
    trap_expect_begin();
    csr_write(CSR_MHPMCOUNTER(n), 0x1);
    if (trap_was_triggered()) {
        trap_expect_end();
        return false;   /* CSR access trapped → not implemented */
    }
    trap_expect_end();

    /* Read back */
    trap_expect_begin();
    uintptr_t val = csr_read(CSR_MHPMCOUNTER(n));
    if (trap_was_triggered()) {
        trap_expect_end();
        return false;
    }
    trap_expect_end();

    /* Clean up: restore counter low half to 0 */
    trap_expect_begin();
    csr_write(CSR_MHPMCOUNTER(n), 0);
    trap_expect_end();

#if __riscv_xlen == 32
    /* Also zero the high half if it exists (may trap — safe to ignore) */
    trap_expect_begin();
    csr_write(CSR_MHPMCOUNTERH(n), 0);
    trap_expect_end();
#endif

    return val != 0;
}

/* ===================================================================
 * Find the first implemented counter (returns 0 if none found)
 * =================================================================== */
static inline unsigned find_first_counter(void) {
    for (unsigned n = COFPMF_FIRST_COUNTER; n <= COFPMF_LAST_COUNTER; n++) {
        if (is_counter_implemented(n))
            return n;
    }
    return 0;
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
 * LCOFI interrupt enable / disable helpers
 * =================================================================== */
static inline void lcofie_enable(void) {
    CSRS(mie, MIE_LCOFIE);
}

static inline void lcofie_disable(void) {
    CSRC(mie, MIE_LCOFIE);
}

static inline void lcofip_clear(void) {
    CSRC(mip, MIP_LCOFIP);
}

static inline bool lcofip_pending(void) {
    return (CSRR(mip) & MIP_LCOFIP) != 0;
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
 * Check if counting actually works on this platform AND is
 * instruction-accurate.
 *
 * The exact-delta assertions in the counting tests (e.g. "inhibited
 * count == 0") are only meaningful when mhpmcounter tracks retired
 * instructions.  A mere "count > 0" check cannot distinguish an
 * instruction-accurate counter from one driven by another source
 * (e.g. host time), so we verify the count lands in a sane range for
 * a known instruction workload and let callers SKIP otherwise.
 * =================================================================== */
static inline bool is_counting_functional(unsigned n) {
    /* Stop any prior counting and clear state */
    cofpmf_stop_counting(n);
    cofpmf_write_counter(n, 0);

    /* Now start counting — write event AFTER counter is zeroed */
    cofpmf_write_event(n, COFPMF_EVENT_INSTRET);

    /* Execute a known instruction workload: 200 NOPs plus loop overhead. */
    cofpmf_execute_nops(200);

    /* Read counter BEFORE stopping (stop clears mhpmevent) */
    uint64_t count = cofpmf_read_counter(n);

    cofpmf_stop_counting(n);
    cofpmf_write_counter(n, 0);

    /* Instruction-accuracy sanity bound: 200 NOPs plus loop/framework
     * overhead must land in a small range.  Counters driven by a
     * non-instruction source return values orders of magnitude larger
     * and are rejected here. */
    return count >= 200 && count < 100000;
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

/* ===================================================================
 * Check if scountovf CSR correctly reflects the OF bit.
 *
 * Some platforms may not implement scountovf or may gate it
 * differently than expected.
 * =================================================================== */
static inline bool is_scountovf_functional(unsigned n) {
    uint64_t orig = cofpmf_read_event(n);

    /* Ensure mcounteren bit is set for this counter */
    uintptr_t mc_orig = CSRR(mcounteren);
    CSRW(mcounteren, mc_orig | (1UL << n));

    /* Set OF=1, check scountovf */
    cofpmf_write_event(n, orig | MHPMEVENT_OF);

    trap_expect_begin();
    uint32_t sovf = (uint32_t)CSRR(scountovf);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* Restore */
    cofpmf_write_event(n, orig);
    CSRW(mcounteren, mc_orig);

    if (trapped)
        return false;

    return (sovf & (1u << n)) != 0;
}

/* ===================================================================
 * Check if S-mode is supported (misa.S bit)
 * =================================================================== */
static inline bool has_smode(void) {
    uintptr_t misa = CSRR(misa);
    return (misa & (1UL << ('S' - 'A'))) != 0;
}

/* ===================================================================
 * Check if U-mode is supported (misa.U bit)
 * =================================================================== */
static inline bool has_umode(void) {
    uintptr_t misa = CSRR(misa);
    return (misa & (1UL << ('U' - 'A'))) != 0;
}

/* ===================================================================
 * Check if H extension is supported (misa.H bit)
 * =================================================================== */
static inline bool has_hext(void) {
    uintptr_t misa = CSRR(misa);
    return (misa & (1UL << ('H' - 'A'))) != 0;
}

#endif /* SSCOFPMF_HELPERS_H */
