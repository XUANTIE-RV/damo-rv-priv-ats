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
    asm volatile("csrw 0x14D, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vstimecmp_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x24D" : "=r"(v) :: "memory");
    return v;
}

static inline void vstimecmp_write(uintptr_t v) {
    asm volatile("csrw 0x24D, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * time CSR read helper
 * =================================================================== */
static inline uintptr_t time_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0xC01" : "=r"(v) :: "memory");
    return v;
}

/* ===================================================================
 * menvcfg / henvcfg helpers
 * =================================================================== */
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

/* ===================================================================
 * mcounteren / hcounteren helpers
 * =================================================================== */
static inline uintptr_t mcounteren_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x306" : "=r"(v) :: "memory");
    return v;
}

static inline void mcounteren_set(uintptr_t bits) {
    asm volatile("csrs 0x306, %0" :: "r"(bits) : "memory");
}

static inline void mcounteren_clear(uintptr_t bits) {
    asm volatile("csrc 0x306, %0" :: "r"(bits) : "memory");
}

static inline void hcounteren_set(uintptr_t bits) {
    asm volatile("csrs 0x606, %0" :: "r"(bits) : "memory");
}

static inline void hcounteren_clear(uintptr_t bits) {
    asm volatile("csrc 0x606, %0" :: "r"(bits) : "memory");
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
    stimecmp_write((uintptr_t)-1);
}

/* Trigger STIP by setting stimecmp to a past value */
static inline void sstc_trigger_stip(void) {
    uintptr_t now = time_read();
    stimecmp_write(now > 0 ? now - 1 : 0);
}

/* Install S-mode trap handler for sstc tests */
static inline void sstc_install_strap(void) {
    g_sstc_trap_cause = 0;
    asm volatile("csrw sscratch, %0" :: "r"(sstc_trap_scratch) : "memory");
    asm volatile("csrw stvec, %0" :: "r"((uintptr_t)sstc_trap_entry) : "memory");
}

#endif /* SSTC_TEST_HELPERS_H */
