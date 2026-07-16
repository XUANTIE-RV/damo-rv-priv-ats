/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Sstc test suite.
 *
 * All test files (test_stce.c / test_rw.c / test_access.c / test_timer.c /
 * test_stip.c / test_vstimecmp.c) are #included into test_register.c,
 * so static functions and globals defined here are visible across the
 * whole compilation unit.
 */

#ifndef SSTC_TEST_HELPERS_H
#define SSTC_TEST_HELPERS_H

#include "test_framework.h"
#ifdef ENABLE_HYP
#include "hyp/hyp_priv.h"
/* mcounteren_read/set/clear are defined in hyp_csr.c (linked via ENABLE_HYP).
 * Declared here as extern to avoid pulling in hyp_csr.h which would
 * conflict with local static inline definitions below. */
extern uintptr_t mcounteren_read(void);
extern void mcounteren_set(uintptr_t mask);
extern void mcounteren_clear(uintptr_t mask);
#endif

/* ===================================================================
 * stimecmp / vstimecmp inline CSR access helpers
 *
 * These use raw CSR address immediates because the CSR names may not
 * be recognized by the assembler without -march extensions.
 * =================================================================== */

static inline uintptr_t stimecmp_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x14D" : "=r"(v) :: "memory");
    return v;
}

static inline void stimecmp_write(uintptr_t v) {
#if __riscv_xlen == 32
    /* RV32: three-step write to avoid transient STIP:
     *   1. stimecmph = MAX  (prevent stimecmp < time transient)
     *   2. stimecmp  = v    (set low half)
     *   3. stimecmph = 0    (set high half to target) */
    asm volatile("csrw 0x15D, %0" :: "r"((uintptr_t)-1) : "memory");
    asm volatile("csrw 0x14D, %0" :: "r"(v) : "memory");
    asm volatile("csrw 0x15D, zero" ::: "memory");
#else
    asm volatile("csrw 0x14D, %0" :: "r"(v) : "memory");
#endif
}

static inline uintptr_t vstimecmp_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x24D" : "=r"(v) :: "memory");
    return v;
}

static inline void vstimecmp_write(uintptr_t v) {
#if __riscv_xlen == 32
    /* RV32: three-step write to avoid transient VSTIP */
    asm volatile("csrw 0x25D, %0" :: "r"((uintptr_t)-1) : "memory");
    asm volatile("csrw 0x24D, %0" :: "r"(v) : "memory");
    asm volatile("csrw 0x25D, zero" ::: "memory");
#else
    asm volatile("csrw 0x24D, %0" :: "r"(v) : "memory");
#endif
}

/* ===================================================================
 * time CSR read helper
 *
 * RV32: time is 64-bit, split across time(0xC01) and timeh(0xC81).
 * RV64: single CSR.
 * =================================================================== */
static inline uintptr_t time_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0xC01" : "=r"(v) :: "memory");
    return v;
}

/* ===================================================================
 * menvcfg / henvcfg helpers
 *
 * rv64: single 64-bit CSR (0x30A/0x60A). STCE=bit63.
 * rv32: split into menvcfg(0x30A)[31:0] + menvcfgh(0x31A)[63:32].
 *       STCE (bit63) lives in menvcfgh bit31 on rv32.
 *       All Sstc-relevant bits are in the high half, so rv32
 *       set/clear/read must target menvcfgh/henvcfgh.
 * =================================================================== */
#if __riscv_xlen == 32
/* Override encoding.h: STCE bit63 → menvcfgh bit31 on RV32 */
#undef MENVCFG_STCE
#define MENVCFG_STCE    (1UL << 31)

static inline uintptr_t menvcfg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x31A" : "=r"(v) :: "memory");
    return v;
}

static inline void menvcfg_write(uintptr_t v) {
    asm volatile("csrw 0x31A, %0" :: "r"(v) : "memory");
}

static inline void menvcfg_set(uintptr_t bits) {
    asm volatile("csrs 0x31A, %0" :: "r"(bits) : "memory");
}

static inline void menvcfg_clear(uintptr_t bits) {
    asm volatile("csrc 0x31A, %0" :: "r"(bits) : "memory");
}

static inline uintptr_t henvcfg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x61A" : "=r"(v) :: "memory");
    return v;
}

static inline void henvcfg_write(uintptr_t v) {
    asm volatile("csrw 0x61A, %0" :: "r"(v) : "memory");
}

static inline void henvcfg_set(uintptr_t bits) {
    asm volatile("csrs 0x61A, %0" :: "r"(bits) : "memory");
}

static inline void henvcfg_clear(uintptr_t bits) {
    asm volatile("csrc 0x61A, %0" :: "r"(bits) : "memory");
}
#else
static inline uintptr_t menvcfg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x30A" : "=r"(v) :: "memory");
    return v;
}

static inline void menvcfg_write(uintptr_t v) {
    asm volatile("csrw 0x30A, %0" :: "r"(v) : "memory");
}

static inline void menvcfg_set(uintptr_t bits) {
    asm volatile("csrs 0x30A, %0" :: "r"(bits) : "memory");
}

static inline void menvcfg_clear(uintptr_t bits) {
    asm volatile("csrc 0x30A, %0" :: "r"(bits) : "memory");
}

static inline uintptr_t henvcfg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x60A" : "=r"(v) :: "memory");
    return v;
}

static inline void henvcfg_write(uintptr_t v) {
    asm volatile("csrw 0x60A, %0" :: "r"(v) : "memory");
}

static inline void henvcfg_set(uintptr_t bits) {
    asm volatile("csrs 0x60A, %0" :: "r"(bits) : "memory");
}

static inline void henvcfg_clear(uintptr_t bits) {
    asm volatile("csrc 0x60A, %0" :: "r"(bits) : "memory");
}
#endif

/* ===================================================================
 * hcounteren helpers
 *
 * mcounteren_read/set/clear are provided by hyp_csr.c (declared as
 * extern above). Local hcounteren_set/clear inline definitions are
 * kept for RV32 compatibility (hyp_csr.c versions use CSR_HCOUNTEREN
 * which targets the low 32-bit half on RV32).
 * =================================================================== */
static inline void hcounteren_set(uintptr_t bits) {
    CSRS(CSR_HCOUNTEREN, bits);
}

static inline void hcounteren_clear(uintptr_t bits) {
    CSRC(CSR_HCOUNTEREN, bits);
}

/* ===================================================================
 * mip / sip / mie / sie helpers
 * =================================================================== */
static inline uintptr_t mip_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, mip" : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t sip_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, sip" : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t hip_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x644" : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t hvip_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x645" : "=r"(v) :: "memory");
    return v;
}

static inline void hvip_set(uintptr_t bits) {
    asm volatile("csrs 0x645, %0" :: "r"(bits) : "memory");
}

static inline void hvip_clear(uintptr_t bits) {
    asm volatile("csrc 0x645, %0" :: "r"(bits) : "memory");
}

static inline uintptr_t htimedelta_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x605" : "=r"(v) :: "memory");
    return v;
}

static inline void htimedelta_write(uintptr_t v) {
    asm volatile("csrw 0x605, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * Delay loop: spin for n iterations to allow hardware to reflect
 * STIP / VSTIP changes.
 * =================================================================== */
#define DELAY_LOOP(n) do { \
    for (volatile int _dl = 0; _dl < (n); _dl++) { } \
} while (0)

/* Default delay count */
#define SSTC_DELAY  100

/* ===================================================================
 * H extension detection
 * =================================================================== */
#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

/* ===================================================================
 * Globals provided by tests/sstc_strap.S
 * =================================================================== */
extern volatile uintptr_t g_sstc_trap_cause;
extern void               sstc_trap_entry(void);
extern unsigned long       sstc_trap_scratch[];

/* ===================================================================
 * Common setup / cleanup helpers
 * =================================================================== */

/* Enable STCE and TM for S-mode stimecmp access */
static inline void sstc_enable_smode_access(void) {
    menvcfg_set(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);
}

/* Disable STCE */
static inline void sstc_disable_stce(void) {
    menvcfg_clear(MENVCFG_STCE);
}

/* Ensure stimecmp is set to max (no pending interrupt) */
static inline void sstc_clear_timer(void) {
#if __riscv_xlen == 32
    /* RV32: write both halves to max */
    asm volatile("csrw 0x15D, %0" :: "r"((uintptr_t)-1) : "memory");
    asm volatile("csrw 0x14D, %0" :: "r"((uintptr_t)-1) : "memory");
#else
    stimecmp_write((uintptr_t)-1);
#endif
}

/* Trigger STIP by setting stimecmp to a past value */
static inline void sstc_trigger_stip(void) {
#if __riscv_xlen == 32
    /* RV32: read full 64-bit time, write stimecmp = time - 1 */
    uint32_t lo, hi, hi2;
    do {
        asm volatile("csrr %0, 0xC81" : "=r"(hi) :: "memory");
        asm volatile("csrr %0, 0xC01" : "=r"(lo) :: "memory");
        asm volatile("csrr %0, 0xC81" : "=r"(hi2) :: "memory");
    } while (hi != hi2);
    /* Compute time - 1 with 64-bit borrow */
    if (lo == 0) {
        asm volatile("csrw 0x15D, %0" :: "r"(hi - 1) : "memory");
    } else {
        asm volatile("csrw 0x15D, %0" :: "r"(hi) : "memory");
    }
    uint32_t cmp_lo = (lo == 0) ? 0xFFFFFFFF : (lo - 1);
    asm volatile("csrw 0x14D, %0" :: "r"(cmp_lo) : "memory");
#else
    uintptr_t now = time_read();
    stimecmp_write(now > 0 ? now - 1 : 0);
#endif
}

/* Install S-mode trap handler for sstc tests */
static inline void sstc_install_strap(void) {
    g_sstc_trap_cause = 0;
    asm volatile("csrw sscratch, %0" :: "r"(sstc_trap_scratch) : "memory");
    asm volatile("csrw stvec, %0" :: "r"((uintptr_t)sstc_trap_entry) : "memory");
}

#endif /* SSTC_TEST_HELPERS_H */
