/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMCNTRPMF_HELPERS_H
#define SMCNTRPMF_HELPERS_H

#include "test_framework.h"
#include "encoding.h"

/* ===================================================================
 * Smcntrpmf CSR addresses
 *
 * mcyclecfg:   0x321 (machine counter configuration for cycle)
 * minstretcfg: 0x322 (machine counter configuration for instret)
 * mcyclecfgh:  0x721 (RV32 only, high 32 bits)
 * minstretcfgh:0x722 (RV32 only, high 32 bits)
 * =================================================================== */
#define CSR_MCYCLECFG       0x321
#define CSR_MINSTRETCFG     0x322
#define CSR_MCYCLECFGH      0x721
#define CSR_MINSTRETCFGH    0x722

/* mcountinhibit CSR address */
#define CSR_MCOUNTINHIBIT   0x320

/* ===================================================================
 * xINH bit positions (same encoding as Sscofpmf mhpmevent)
 *
 * Bit 63: OF (read-only 0 for mcyclecfg/minstretcfg)
 * Bit 62: MINH
 * Bit 61: SINH
 * Bit 60: UINH
 * Bit 59: VSINH
 * Bit 58: VUINH
 * Bits 57:0: WPRI
 * =================================================================== */
#define CYCLECFG_OF         (1ULL << 63)
#define CYCLECFG_MINH       (1ULL << 62)
#define CYCLECFG_SINH       (1ULL << 61)
#define CYCLECFG_UINH       (1ULL << 60)
#define CYCLECFG_VSINH      (1ULL << 59)
#define CYCLECFG_VUINH      (1ULL << 58)

/* Mask for all xINH bits */
#define CYCLECFG_ALL_XINH   (CYCLECFG_MINH | CYCLECFG_SINH | CYCLECFG_UINH | \
                             CYCLECFG_VSINH | CYCLECFG_VUINH)

/* Mask for WPRI bits (57:0) */
#define CYCLECFG_WPRI_MASK  ((1ULL << 58) - 1)

/* ===================================================================
 * mcountinhibit bit positions
 * =================================================================== */
#define MCOUNTINHIBIT_CY    (1UL << 0)
#define MCOUNTINHIBIT_IR    (1UL << 2)

/* ===================================================================
 * Direct CSR access via inline assembly
 *
 * mcyclecfg/minstretcfg are not in the common csr_accessors.c
 * switch-case, so we use direct inline asm here.
 * =================================================================== */

static inline uintptr_t mcyclecfg_read(void)
{
    uintptr_t val;
    asm volatile("csrr %0, 0x321" : "=r"(val) :: "memory");
    return val;
}

static inline void mcyclecfg_write(uintptr_t val)
{
    asm volatile("csrw 0x321, %0" :: "r"(val) : "memory");
}

static inline uintptr_t minstretcfg_read(void)
{
    uintptr_t val;
    asm volatile("csrr %0, 0x322" : "=r"(val) :: "memory");
    return val;
}

static inline void minstretcfg_write(uintptr_t val)
{
    asm volatile("csrw 0x322, %0" :: "r"(val) : "memory");
}

#if __riscv_xlen == 32
static inline uintptr_t mcyclecfgh_read(void)
{
    uintptr_t val;
    asm volatile("csrr %0, 0x721" : "=r"(val) :: "memory");
    return val;
}

static inline void mcyclecfgh_write(uintptr_t val)
{
    asm volatile("csrw 0x721, %0" :: "r"(val) : "memory");
}

static inline uintptr_t minstretcfgh_read(void)
{
    uintptr_t val;
    asm volatile("csrr %0, 0x722" : "=r"(val) :: "memory");
    return val;
}

static inline void minstretcfgh_write(uintptr_t val)
{
    asm volatile("csrw 0x722, %0" :: "r"(val) : "memory");
}
#endif /* __riscv_xlen == 32 */

/* ===================================================================
 * mcountinhibit access
 * =================================================================== */
static inline uintptr_t mcountinhibit_read(void)
{
    uintptr_t val;
    asm volatile("csrr %0, 0x320" : "=r"(val) :: "memory");
    return val;
}

static inline void mcountinhibit_write(uintptr_t val)
{
    asm volatile("csrw 0x320, %0" :: "r"(val) : "memory");
}

/* ===================================================================
 * cycle / instret counter read (64-bit logical value)
 *
 * RV32: uses hi-lo-hi retry loop for atomicity.
 * RV64: single CSR read.
 * =================================================================== */
static inline uint64_t read_cycle(void)
{
#if __riscv_xlen == 32
    uint32_t hi, lo, hi2;
    do {
        asm volatile("csrr %0, 0xC80" : "=r"(hi) :: "memory");  /* cycleh */
        asm volatile("csrr %0, 0xC00" : "=r"(lo) :: "memory");  /* cycle */
        asm volatile("csrr %0, 0xC80" : "=r"(hi2) :: "memory");
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
#else
    uintptr_t val;
    asm volatile("csrr %0, 0xC00" : "=r"(val) :: "memory");
    return (uint64_t)val;
#endif
}

static inline uint64_t read_instret(void)
{
#if __riscv_xlen == 32
    uint32_t hi, lo, hi2;
    do {
        asm volatile("csrr %0, 0xC82" : "=r"(hi) :: "memory");  /* instreth */
        asm volatile("csrr %0, 0xC02" : "=r"(lo) :: "memory");  /* instret */
        asm volatile("csrr %0, 0xC82" : "=r"(hi2) :: "memory");
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
#else
    uintptr_t val;
    asm volatile("csrr %0, 0xC02" : "=r"(val) :: "memory");
    return (uint64_t)val;
#endif
}

/* ===================================================================
 * M-mode cycle / instret read (mcycle / minstret)
 * =================================================================== */
static inline uint64_t read_mcycle(void)
{
#if __riscv_xlen == 32
    uint32_t hi, lo, hi2;
    do {
        asm volatile("csrr %0, 0xB80" : "=r"(hi) :: "memory");  /* mcycleh */
        asm volatile("csrr %0, 0xB00" : "=r"(lo) :: "memory");  /* mcycle */
        asm volatile("csrr %0, 0xB80" : "=r"(hi2) :: "memory");
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
#else
    uintptr_t val;
    asm volatile("csrr %0, 0xB00" : "=r"(val) :: "memory");
    return (uint64_t)val;
#endif
}

static inline uint64_t read_minstret(void)
{
#if __riscv_xlen == 32
    uint32_t hi, lo, hi2;
    do {
        asm volatile("csrr %0, 0xB82" : "=r"(hi) :: "memory");  /* minstreth */
        asm volatile("csrr %0, 0xB02" : "=r"(lo) :: "memory");  /* minstret */
        asm volatile("csrr %0, 0xB82" : "=r"(hi2) :: "memory");
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
#else
    uintptr_t val;
    asm volatile("csrr %0, 0xB02" : "=r"(val) :: "memory");
    return (uint64_t)val;
#endif
}

/* ===================================================================
 * Execute a known number of NOPs
 * =================================================================== */
static inline void execute_nops(unsigned count)
{
    for (volatile unsigned i = 0; i < count; i++) {
        asm volatile("nop");
    }
}

/* ===================================================================
 * Check if Smcntrpmf is implemented
 *
 * Probe by attempting to write/read mcyclecfg. If it traps or
 * reads back zero for all xINH bits after writing all ones,
 * the extension is likely not implemented.
 * =================================================================== */
static inline bool smcntrpmf_implemented(void)
{
    trap_expect_begin();
    uintptr_t orig = mcyclecfg_read();
    if (trap_was_triggered()) {
        trap_expect_end();
        return false;
    }
    trap_expect_end();

    /* Try writing MINH bit and reading back */
    trap_expect_begin();
    mcyclecfg_write(CYCLECFG_MINH);
    if (trap_was_triggered()) {
        trap_expect_end();
        mcyclecfg_write(orig);
        return false;
    }
    uintptr_t val = mcyclecfg_read();
    trap_expect_end();

    mcyclecfg_write(orig);
    return (val & CYCLECFG_MINH) != 0;
}

/* ===================================================================
 * Check if S-mode is supported (misa.S bit)
 * =================================================================== */
static inline bool has_smode(void)
{
    uintptr_t misa = CSRR(misa);
    return (misa & (1UL << ('S' - 'A'))) != 0;
}

/* ===================================================================
 * Check if U-mode is supported (misa.U bit)
 * =================================================================== */
static inline bool has_umode(void)
{
    uintptr_t misa = CSRR(misa);
    return (misa & (1UL << ('U' - 'A'))) != 0;
}

/* ===================================================================
 * Check if H extension is supported (misa.H bit)
 * =================================================================== */
static inline bool has_hext(void)
{
    uintptr_t misa = CSRR(misa);
    return (misa & (1UL << ('H' - 'A'))) != 0;
}

/* ===================================================================
 * Check if cycle counter is functional
 * =================================================================== */
static inline bool cycle_counter_functional(void)
{
    uint64_t start = read_mcycle();
    execute_nops(100);
    uint64_t end = read_mcycle();
    return (end > start);
}

/* ===================================================================
 * Check if instret counter is functional
 * =================================================================== */
static inline bool instret_counter_functional(void)
{
    uint64_t start = read_minstret();
    execute_nops(100);
    uint64_t end = read_minstret();
    return (end > start);
}

#endif /* SMCNTRPMF_HELPERS_H */
