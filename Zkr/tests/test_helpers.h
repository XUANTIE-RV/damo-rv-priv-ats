/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Zkr test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 */

#ifndef ZKR_TEST_HELPERS_H
#define ZKR_TEST_HELPERS_H

#include "test_framework.h"

/* ===================================================================
 * seed CSR definitions (address 0x015)
 * =================================================================== */

#define CSR_SEED        0x015

/* OPST field: bits [31:30] */
#define SEED_OPST_SHIFT     30
#define SEED_OPST_MASK      (0x3UL << SEED_OPST_SHIFT)
#define SEED_OPST_BIST      (0x0UL << SEED_OPST_SHIFT)  /* 00 */
#define SEED_OPST_WAIT      (0x1UL << SEED_OPST_SHIFT)  /* 01 */
#define SEED_OPST_ES16      (0x2UL << SEED_OPST_SHIFT)  /* 10 */
#define SEED_OPST_DEAD      (0x3UL << SEED_OPST_SHIFT)  /* 11 */

/* Entropy field: bits [15:0] */
#define SEED_ENTROPY_MASK   0xFFFFUL

/* Reserved/custom bits: [29:24] */
#define SEED_RESERVED_MASK  (0x3FUL << 24)

/* ===================================================================
 * mseccfg SSEED/USEED field definitions
 * =================================================================== */

#define MSECCFG_USEED       (1UL << 8)
#define MSECCFG_SSEED       (1UL << 9)

/* ===================================================================
 * seed CSR access helpers (M-mode)
 *
 * IMPORTANT: seed CSR can ONLY be accessed with read-write CSR
 * instructions (csrrw). Using read-only instructions (csrrs/csrrc
 * with rs1=x0, or csrrsi/csrrci with uimm=0) raises an
 * illegal-instruction exception per SPEC.
 * =================================================================== */

/* Read seed CSR using csrrw (the only safe way to read) */
static inline uintptr_t seed_read(void)
{
    uintptr_t v;
    asm volatile("csrrw %0, 0x015, x0" : "=r"(v) :: "memory");
    return v;
}

/* Write seed CSR using csrrw with a value, return old value */
static inline uintptr_t seed_write(uintptr_t val)
{
    uintptr_t old;
    asm volatile("csrrw %0, 0x015, %1" : "=r"(old) : "r"(val) : "memory");
    return old;
}

/* Read seed CSR using csrrs with non-zero rs1 (valid read-write access) */
static inline uintptr_t seed_read_csrrs(uintptr_t dummy)
{
    uintptr_t v;
    asm volatile("csrrs %0, 0x015, %1" : "=r"(v) : "r"(dummy) : "memory");
    return v;
}

/* Read seed CSR using csrrc with non-zero rs1 (valid read-write access) */
static inline uintptr_t seed_read_csrrc(uintptr_t dummy)
{
    uintptr_t v;
    asm volatile("csrrc %0, 0x015, %1" : "=r"(v) : "r"(dummy) : "memory");
    return v;
}

/* Read seed CSR using csrrsi with non-zero uimm (valid access) */
static inline uintptr_t seed_read_csrrsi(void)
{
    uintptr_t v;
    asm volatile("csrrsi %0, 0x015, 1" : "=r"(v) :: "memory");
    return v;
}

/* ===================================================================
 * Read-only access to seed (WILL trigger illegal-instruction)
 * These are used in exception tests.
 * =================================================================== */

/* csrrs rd, seed, x0 - read-only access (triggers exception) */
static inline uintptr_t seed_read_ro_csrrs(void)
{
    uintptr_t v;
    asm volatile("csrrs %0, 0x015, x0" : "=r"(v) :: "memory");
    return v;
}

/* csrrc rd, seed, x0 - read-only access (triggers exception) */
static inline uintptr_t seed_read_ro_csrrc(void)
{
    uintptr_t v;
    asm volatile("csrrc %0, 0x015, x0" : "=r"(v) :: "memory");
    return v;
}

/* csrrsi rd, seed, 0 - read-only access (triggers exception) */
static inline uintptr_t seed_read_ro_csrrsi(void)
{
    uintptr_t v;
    asm volatile(".insn i 0x73, 0x6, %0, x0, 0x015" : "=r"(v) :: "memory");
    return v;
}

/* csrrci rd, seed, 0 - read-only access (triggers exception) */
static inline uintptr_t seed_read_ro_csrrci(void)
{
    uintptr_t v;
    asm volatile(".insn i 0x73, 0x7, %0, x0, 0x015" : "=r"(v) :: "memory");
    return v;
}

/* ===================================================================
 * mseccfg access helpers
 * =================================================================== */

static inline uintptr_t mseccfg_read_zkr(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x747" : "=r"(v) :: "memory");
    return v;
}

static inline void mseccfg_write_zkr(uintptr_t v)
{
    asm volatile("csrw 0x747, %0" :: "r"(v) : "memory");
}

static inline void mseccfg_set_bits(uintptr_t bits)
{
    asm volatile("csrs 0x747, %0" :: "r"(bits) : "memory");
}

static inline void mseccfg_clear_bits(uintptr_t bits)
{
    asm volatile("csrc 0x747, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * Helper: detect if Zkr (seed CSR) is implemented
 *
 * Non-asserting probe: attempts csrrw to seed in M-mode.
 * If it traps, Zkr is absent.
 * =================================================================== */
static inline bool check_zkr(void)
{
    trap_expect_begin();
    clear_mdt();
    uintptr_t v;
    asm volatile("csrrw %0, 0x015, x0" : "=r"(v) :: "memory");
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/* ===================================================================
 * Helper: detect if SSEED field is writable
 * =================================================================== */
static inline bool sseed_writable(void)
{
    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);
    uintptr_t val = mseccfg_read_zkr();
    mseccfg_write_zkr(orig);
    return (val & MSECCFG_SSEED) != 0;
}

/* ===================================================================
 * Helper: detect if USEED field is writable
 * =================================================================== */
static inline bool useed_writable(void)
{
    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_USEED);
    uintptr_t val = mseccfg_read_zkr();
    mseccfg_write_zkr(orig);
    return (val & MSECCFG_USEED) != 0;
}

/* ===================================================================
 * Helper: poll seed until OPST=ES16 or max_attempts reached
 *
 * Returns the seed value if ES16 obtained, 0 otherwise.
 * =================================================================== */
static inline uintptr_t seed_poll_es16(unsigned int max_attempts)
{
    for (unsigned int i = 0; i < max_attempts; i++) {
        uintptr_t val = seed_read();
        if ((val & SEED_OPST_MASK) == SEED_OPST_ES16)
            return val;
    }
    return 0;
}

/* ===================================================================
 * Test helper macros for S/U-mode seed access
 *
 * These use GCC statement expressions ({...}) so they can be used
 * as expressions inside PRIV_DO().
 * =================================================================== */

/* S/U-mode: execute csrrw to seed (read-write access) */
#define SEED_ACCESS_CSRRW() ({ \
    uintptr_t _v; \
    asm volatile("csrrw %0, 0x015, x0" : "=r"(_v) :: "memory"); \
    _v; \
})

/* S/U-mode: execute csrrs rd, seed, x0 (read-only, always traps) */
#define SEED_ACCESS_CSRRS_RO() ({ \
    uintptr_t _v; \
    asm volatile("csrrs %0, 0x015, x0" : "=r"(_v) :: "memory"); \
    _v; \
})

/* S/U-mode: execute csrrsi rd, seed, 0 (read-only, always traps) */
#define SEED_ACCESS_CSRRSI_RO() ({ \
    uintptr_t _v; \
    asm volatile(".insn i 0x73, 0x6, %0, x0, 0x015" : "=r"(_v) :: "memory"); \
    _v; \
})

/* S/U-mode: execute csrrci rd, seed, 0 (read-only, always traps) */
#define SEED_ACCESS_CSRRCI_RO() ({ \
    uintptr_t _v; \
    asm volatile(".insn i 0x73, 0x7, %0, x0, 0x015" : "=r"(_v) :: "memory"); \
    _v; \
})

/* S/U-mode: execute csrrs rd, seed, rs1 (rs1!=x0, read-write access) */
#define SEED_ACCESS_CSRRS_RW() ({ \
    uintptr_t _v, _dummy = 1; \
    asm volatile("csrrs %0, 0x015, %1" : "=r"(_v) : "r"(_dummy) : "memory"); \
    _v; \
})

/* Aliases for backward compatibility */
#define SMODE_SEED_CSRRW()      SEED_ACCESS_CSRRW()
#define SMODE_SEED_CSRRS_RO()   SEED_ACCESS_CSRRS_RO()
#define SMODE_SEED_CSRRSI_RO()  SEED_ACCESS_CSRRSI_RO()
#define SMODE_SEED_CSRRCI_RO()  SEED_ACCESS_CSRRCI_RO()
#define SMODE_SEED_CSRRS_RW()   SEED_ACCESS_CSRRS_RW()
#define UMODE_SEED_CSRRW()      SEED_ACCESS_CSRRW()
#define UMODE_SEED_CSRRS_RO()   SEED_ACCESS_CSRRS_RO()
#define UMODE_SEED_CSRRSI_RO()  SEED_ACCESS_CSRRSI_RO()
#define UMODE_SEED_CSRRS_RW()   SEED_ACCESS_CSRRS_RW()

#endif /* ZKR_TEST_HELPERS_H */
