/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zkr test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 */

#ifndef HYPERVISOR_ZKR_TEST_HELPERS_H
#define HYPERVISOR_ZKR_TEST_HELPERS_H

#include "test_framework.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_trap.h"

/* ===================================================================
 * seed CSR definitions (address 0x015)
 * =================================================================== */

#define CSR_SEED        0x015

/* OPST field: bits [31:30] */
#define SEED_OPST_SHIFT     30
#define SEED_OPST_MASK      (0x3UL << SEED_OPST_SHIFT)
#define SEED_OPST_BIST      (0x0UL << SEED_OPST_SHIFT)
#define SEED_OPST_WAIT      (0x1UL << SEED_OPST_SHIFT)
#define SEED_OPST_ES16      (0x2UL << SEED_OPST_SHIFT)
#define SEED_OPST_DEAD      (0x3UL << SEED_OPST_SHIFT)

/* ===================================================================
 * mseccfg SSEED/USEED field definitions (CSR 0x747)
 * =================================================================== */

#define MSECCFG_USEED       (1UL << 8)
#define MSECCFG_SSEED       (1UL << 9)

/* ===================================================================
 * Feature detection
 * =================================================================== */

#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

#define REQUIRE_H_EXT() do { \
    if (!HAS_H_EXT()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * M-mode seed CSR access helpers
 *
 * IMPORTANT: seed CSR can ONLY be accessed with read-write CSR
 * instructions (csrrw). Read-only instructions (csrrs/csrrc with
 * rs1=x0, csrrsi/csrrci with uimm=0) raise illegal-instruction.
 * =================================================================== */

static inline uintptr_t seed_read_m(void)
{
    uintptr_t v;
    asm volatile("csrrw %0, 0x015, x0" : "=r"(v) :: "memory");
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
 * Detection helpers
 * =================================================================== */

/* Non-asserting Zkr detection: probe seed CSR in M-mode. */
static inline bool check_zkr(void)
{
    clear_mdt();
    trap_expect_begin();
    uintptr_t v;
    asm volatile("csrrw %0, 0x015, x0" : "=r"(v) :: "memory");
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

#define REQUIRE_ZKR() do { \
    if (!check_zkr()) { \
        TEST_SKIP("Zkr not implemented (seed CSR traps in M-mode)"); \
    } \
} while (0)

/* Check if mseccfg.SSEED is writable. */
static inline bool sseed_writable(void)
{
    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);
    uintptr_t val = mseccfg_read_zkr();
    mseccfg_write_zkr(orig);
    return (val & MSECCFG_SSEED) != 0;
}

#define REQUIRE_SSEED() do { \
    if (!sseed_writable()) { \
        TEST_SKIP("mseccfg.SSEED is read-only zero"); \
    } \
} while (0)

/* ===================================================================
 * VS/VU-mode seed access callbacks (invoked via run_in_vs/vu_mode)
 *
 * Each performs a single CSR instruction on seed (0x015). When the
 * instruction traps, the M-mode trap handler skips it and records the
 * cause; the callback then returns normally via the trampoline ecall.
 * =================================================================== */

/* csrrw rd, seed, x0  (read-write access) */
static uintptr_t _vs_seed_csrrw(uintptr_t arg)
{
    (void)arg;
    uintptr_t v;
    asm volatile("csrrw %0, 0x015, x0" : "=r"(v) :: "memory");
    return v;
}

/* csrrs rd, seed, x0  (read-only access -> always illegal) */
static uintptr_t _vs_seed_csrrs_ro(uintptr_t arg)
{
    (void)arg;
    uintptr_t v;
    asm volatile("csrrs %0, 0x015, x0" : "=r"(v) :: "memory");
    return v;
}

/* csrrsi rd, seed, 0  (read-only access -> always illegal) */
static uintptr_t _vs_seed_csrrsi_ro(uintptr_t arg)
{
    (void)arg;
    uintptr_t v;
    asm volatile(".insn i 0x73, 0x6, %0, x0, 0x015" : "=r"(v) :: "memory");
    return v;
}

/* csrrci rd, seed, 0  (read-only access -> always illegal) */
static uintptr_t _vs_seed_csrrci_ro(uintptr_t arg)
{
    (void)arg;
    uintptr_t v;
    asm volatile(".insn i 0x73, 0x7, %0, x0, 0x015" : "=r"(v) :: "memory");
    return v;
}

/* csrrs rd, seed, rs1 (rs1!=x0 -> read-write access) */
static uintptr_t _vs_seed_csrrs_rw(uintptr_t arg)
{
    (void)arg;
    uintptr_t v, dummy = 1;
    asm volatile("csrrs %0, 0x015, %1" : "=r"(v) : "r"(dummy) : "memory");
    return v;
}

/* ===================================================================
 * HS-mode seed access macros (used with PRIV_DO after goto_priv(PRIV_S))
 *
 * With H ext present and V=0, S-mode == HS-mode. These use GCC
 * statement expressions so they work inside PRIV_DO().
 * =================================================================== */

/* Global to capture HS-mode seed value (PRIV_DO discards return values) */
static uintptr_t g_hs_seed_val;

#define HS_SEED_CSRRW() ({ \
    uintptr_t _v; \
    asm volatile("csrrw %0, 0x015, x0" : "=r"(_v) :: "memory"); \
    _v; \
})

#define HS_SEED_CSRRW_CAPTURE() ({ \
    asm volatile("csrrw %0, 0x015, x0" : "=r"(g_hs_seed_val) :: "memory"); \
    g_hs_seed_val; \
})

#define HS_SEED_CSRRS_RO() ({ \
    uintptr_t _v; \
    asm volatile("csrrs %0, 0x015, x0" : "=r"(_v) :: "memory"); \
    _v; \
})

#endif /* HYPERVISOR_ZKR_TEST_HELPERS_H */
