/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Sstc cross test suite.
 *
 * All test files (test_hcross_sstc_stce.c / test_hcross_sstc_acc.c /
 * test_hcross_sstc_vs.c) are #included into test_register.c,
 * so static functions and globals defined here are visible across the
 * whole compilation unit.
 */

#ifndef HYPERVISOR_SSTC_TEST_HELPERS_H
#define HYPERVISOR_SSTC_TEST_HELPERS_H

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"

/* ===================================================================
 * Sstc CSR address constants and field masks
 * =================================================================== */

#define CSR_STIMECMP_ADDR    0x14D
#define CSR_VSTIMECMP_ADDR   0x24D
#define CSR_MENVCFG_ADDR     0x30A
#define CSR_HENVCFG_ADDR     0x60A
#define CSR_MCOUNTEREN_ADDR  0x306
#define CSR_HCOUNTEREN_ADDR  0x606
#define CSR_HTIMEDELTA_ADDR  0x605
#define CSR_HIP_ADDR         0x644
#define CSR_HVIP_ADDR        0x645
#define CSR_TIME_ADDR        0xC01

#define MENVCFG_STCE         (1ULL << 63)
#define HENVCFG_STCE         (1ULL << 63)
#define MCOUNTEREN_TM        (1ULL << 1)
#define HCOUNTEREN_TM        (1ULL << 1)
#define HIP_VSTIP            (1ULL << 6)
#define HVIP_VSTIP           (1ULL << 6)
#define MIP_STIP             (1ULL << 5)
#define SIE_STIE             (1ULL << 5)

#define CAUSE_INTERRUPT_BIT  (1ULL << 63)
#define CAUSE_S_TIMER_INTERRUPT  5

/* ===================================================================
 * stimecmp / vstimecmp inline CSR access helpers
 * =================================================================== */

static inline uintptr_t stimecmp_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x14D" : "=r"(v) :: "memory");
    return v;
}

static inline void stimecmp_write(uintptr_t v)
{
    asm volatile("csrw 0x14D, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vstimecmp_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x24D" : "=r"(v) :: "memory");
    return v;
}

static inline void vstimecmp_write(uintptr_t v)
{
    asm volatile("csrw 0x24D, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * time CSR read helper
 * =================================================================== */
static inline uintptr_t time_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0xC01" : "=r"(v) :: "memory");
    return v;
}

/* ===================================================================
 * menvcfg / henvcfg helpers
 * =================================================================== */
static inline uintptr_t menvcfg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x30A" : "=r"(v) :: "memory");
    return v;
}

static inline void menvcfg_write(uintptr_t v)
{
    asm volatile("csrw 0x30A, %0" :: "r"(v) : "memory");
}

static inline void menvcfg_set(uintptr_t bits)
{
    asm volatile("csrs 0x30A, %0" :: "r"(bits) : "memory");
}

static inline void menvcfg_clear(uintptr_t bits)
{
    asm volatile("csrc 0x30A, %0" :: "r"(bits) : "memory");
}

static inline uintptr_t henvcfg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x60A" : "=r"(v) :: "memory");
    return v;
}

static inline void henvcfg_write(uintptr_t v)
{
    asm volatile("csrw 0x60A, %0" :: "r"(v) : "memory");
}

static inline void henvcfg_set(uintptr_t bits)
{
    asm volatile("csrs 0x60A, %0" :: "r"(bits) : "memory");
}

static inline void henvcfg_clear(uintptr_t bits)
{
    asm volatile("csrc 0x60A, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * mcounteren / hcounteren helpers
 * =================================================================== */
static inline void mcounteren_set(uintptr_t bits)
{
    asm volatile("csrs 0x306, %0" :: "r"(bits) : "memory");
}

static inline void mcounteren_clear(uintptr_t bits)
{
    asm volatile("csrc 0x306, %0" :: "r"(bits) : "memory");
}

static inline void hcounteren_set(uintptr_t bits)
{
    asm volatile("csrs 0x606, %0" :: "r"(bits) : "memory");
}

static inline void hcounteren_clear(uintptr_t bits)
{
    asm volatile("csrc 0x606, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * hip / hvip / htimedelta helpers
 * =================================================================== */
static inline uintptr_t hip_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x644" : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t hvip_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x645" : "=r"(v) :: "memory");
    return v;
}

static inline void hvip_set(uintptr_t bits)
{
    asm volatile("csrs 0x645, %0" :: "r"(bits) : "memory");
}

static inline void hvip_clear(uintptr_t bits)
{
    asm volatile("csrc 0x645, %0" :: "r"(bits) : "memory");
}

static inline uintptr_t htimedelta_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x605" : "=r"(v) :: "memory");
    return v;
}

static inline void htimedelta_write(uintptr_t v)
{
    asm volatile("csrw 0x605, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * Delay loop
 * =================================================================== */
#define DELAY_LOOP(n) do { \
    for (volatile int _dl = 0; _dl < (n); _dl++) { } \
} while (0)

#define HCROSS_SSTC_DELAY  100

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
extern unsigned long      sstc_trap_scratch[];

/* ===================================================================
 * VS-mode trampoline functions for run_in_vs_mode()
 * =================================================================== */

/* VS-mode: read stimecmp and return trap cause (0 if no trap) */
static uintptr_t _vs_stimecmp_read(uintptr_t arg)
{
    (void)arg;
    stimecmp_read();
    return trap_get_cause();
}

/* VS-mode: write stimecmp with given value */
static uintptr_t _vs_stimecmp_write(uintptr_t arg)
{
    stimecmp_write(arg);
    return 0;
}

/* VS-mode: trigger timer interrupt and capture via VS trap handler */
static uintptr_t _vs_timer_interrupt_test(uintptr_t arg)
{
    (void)arg;
    /* Enable sstatus.SIE (seen as vsstatus.SIE in VS-mode) */
    CSRS(sstatus, MSTATUS_SIE_BIT);

    /* Trigger VS timer: write stimecmp (= vstimecmp) to past value */
    uintptr_t now = time_read();
    stimecmp_write(now > 0 ? now - 1 : 0);

    /* Wait for interrupt */
    DELAY_LOOP(1000);

    CSRC(sstatus, MSTATUS_SIE_BIT);
    return 0;
}

#endif /* HYPERVISOR_SSTC_TEST_HELPERS_H */
