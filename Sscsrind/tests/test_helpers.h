/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_helpers.h - Common helpers for the Sscsrind test suite.
 *
 * H-extension dependent helpers have been removed; see
 * Hypervisor_Sscsrind/tests/test_helpers.h for VS/VU-mode helpers.
 */

#ifndef SSCSRIND_TEST_HELPERS_H
#define SSCSRIND_TEST_HELPERS_H

#include "test_framework.h"

/* ===================================================================
 * CSR addresses
 * =================================================================== */

/* S-mode CSRs (Sscsrind) */
#define CSR_SISELECT_ADDR   0x150
#define CSR_SIREG_ADDR      0x151
#define CSR_SIREG2_ADDR     0x152
#define CSR_SIREG3_ADDR     0x153
/* 0x154 = siph */
#define CSR_SIREG4_ADDR     0x155
#define CSR_SIREG5_ADDR     0x156
#define CSR_SIREG6_ADDR     0x157

/* M-mode CSRs (for setup/control) */
#define CSR_MISELECT_ADDR   0x350
#define CSR_MSTATEEN0_ADDR  0x30C

/* menvcfg.CDE bit */
#define MENVCFG_CDE         (1ULL << 12)

/* mcounteren bits */
#define MCOUNTEREN_CY       (1ULL << 0)
#define MCOUNTEREN_TM       (1ULL << 1)
#define MCOUNTEREN_IR       (1ULL << 2)

/* ===================================================================
 * S-mode CSR access helpers (siselect/sireg*)
 * =================================================================== */

static inline uintptr_t siselect_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x150" : "=r"(v) :: "memory");
    return v;
}

static inline void siselect_write(uintptr_t v)
{
    asm volatile("csrw 0x150, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x151" : "=r"(v) :: "memory");
    return v;
}

static inline void sireg_write(uintptr_t v)
{
    asm volatile("csrw 0x151, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg2_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x152" : "=r"(v) :: "memory");
    return v;
}

static inline void sireg2_write(uintptr_t v)
{
    asm volatile("csrw 0x152, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg3_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x153" : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t sireg4_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x155" : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t sireg5_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x156" : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t sireg6_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x157" : "=r"(v) :: "memory");
    return v;
}

/* ===================================================================
 * M-mode CSR access helpers (for setup)
 * =================================================================== */

static inline uintptr_t miselect_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x350" : "=r"(v) :: "memory");
    return v;
}

static inline void miselect_write(uintptr_t v)
{
    asm volatile("csrw 0x350, %0" :: "r"(v) : "memory");
}

static inline uintptr_t mstateen0_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x30C" : "=r"(v) :: "memory");
    return v;
}

static inline void mstateen0_write(uintptr_t v)
{
    asm volatile("csrw 0x30C, %0" :: "r"(v) : "memory");
}

static inline void mstateen0_set(uintptr_t bits)
{
    asm volatile("csrs 0x30C, %0" :: "r"(bits) : "memory");
}

static inline void mstateen0_clear(uintptr_t bits)
{
    asm volatile("csrc 0x30C, %0" :: "r"(bits) : "memory");
}

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

static inline uintptr_t mcounteren_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x306" : "=r"(v) :: "memory");
    return v;
}

static inline void mcounteren_write(uintptr_t v)
{
    asm volatile("csrw 0x306, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * Feature detection
 * =================================================================== */

/* Check if Sscsrind is implemented (siselect CSR accessible from M-mode) */
static inline bool platform_has_sscsrind(void)
{
    trap_expect_begin();
    siselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/* Check if Smstateen is implemented */
static inline bool platform_has_smstateen(void)
{
    trap_expect_begin();
    mstateen0_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/* ===================================================================
 * Convenience macros for privilege mode testing
 * =================================================================== */

#define TEST_SMODE_BLOCKED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_TRAP(msg, CAUSE_ILLEGAL_INST); \
} while (0)

#define TEST_SMODE_ALLOWED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_NO_TRAP(msg); \
} while (0)

#endif /* SSCSRIND_TEST_HELPERS_H */
