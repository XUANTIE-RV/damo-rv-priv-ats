/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor × Smcsrind test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 */

#ifndef HYPERVISOR_SMCSRIND_TEST_HELPERS_H
#define HYPERVISOR_SMCSRIND_TEST_HELPERS_H

#include "test_framework.h"

#ifdef ENABLE_HYP
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#endif

/* ===================================================================
 * VS-mode CSR addresses (vsiselect/vsireg*)
 * Accessed from HS-mode or M-mode via CSR addresses 0x250-0x257
 * =================================================================== */
#define CSR_VSISELECT_ADDR  0x250
#define CSR_VSIREG_ADDR     0x251
#define CSR_VSIREG2_ADDR    0x252
#define CSR_VSIREG3_ADDR    0x253
/* 0x254 = vsiph */
#define CSR_VSIREG4_ADDR    0x255
#define CSR_VSIREG5_ADDR    0x256
#define CSR_VSIREG6_ADDR    0x257

/* ===================================================================
 * M-mode CSR addresses (miselect/mireg* for setup)
 * =================================================================== */
#define CSR_MISELECT_ADDR   0x350
#define CSR_MIREG_ADDR      0x351

/* ===================================================================
 * VS-mode CSR access helpers (vsiselect/vsireg*)
 * =================================================================== */

static inline uintptr_t vsiselect_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x250" : "=r"(v) :: "memory");
    return v;
}

static inline void vsiselect_write(uintptr_t v)
{
    asm volatile("csrw 0x250, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x251" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg_write(uintptr_t v)
{
    asm volatile("csrw 0x251, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg2_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x252" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg2_write(uintptr_t v)
{
    asm volatile("csrw 0x252, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg3_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x253" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg3_write(uintptr_t v)
{
    asm volatile("csrw 0x253, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg4_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x255" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg4_write(uintptr_t v)
{
    asm volatile("csrw 0x255, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg5_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x256" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg5_write(uintptr_t v)
{
    asm volatile("csrw 0x256, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg6_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x257" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg6_write(uintptr_t v)
{
    asm volatile("csrw 0x257, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * M-mode CSR access helpers (miselect/mireg* for setup)
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

static inline uintptr_t mireg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x351" : "=r"(v) :: "memory");
    return v;
}

static inline void mireg_write(uintptr_t v)
{
    asm volatile("csrw 0x351, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * State-enable CSR helpers (mstateen0)
 * =================================================================== */

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

/* mstateen0 CSRIND bit (bit 60) */
#define MSTATEEN0_CSRIND    (1ULL << 60)

/* ===================================================================
 * Platform detection
 * =================================================================== */

/* Check if H extension is present */
#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

/* Check if Smcsrind is implemented (miselect accessible) */
static inline bool platform_has_smcsrind(void)
{
    trap_expect_begin();
    miselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/* Check if Smstateen is implemented (mstateen0 accessible) */
static inline bool platform_has_smstateen(void)
{
    trap_expect_begin();
    mstateen0_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/* ===================================================================
 * Convenience macros for S-mode (HS-mode) access testing
 * =================================================================== */

/* Execute csr_stmt in S-mode and verify it traps with illegal-instruction */
#define TEST_SMODE_BLOCKED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_TRAP(msg, CAUSE_ILLEGAL_INST); \
} while (0)

/* Execute csr_stmt in S-mode and verify it does NOT trap */
#define TEST_SMODE_ALLOWED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_NO_TRAP(msg); \
} while (0)

#endif /* HYPERVISOR_SMCSRIND_TEST_HELPERS_H */
