/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_helpers.h - Common helpers for the Smcsrind test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 */

#ifndef SMCSRIND_TEST_HELPERS_H
#define SMCSRIND_TEST_HELPERS_H

#include "test_framework.h"

/* ===================================================================
 * Smcsrind M-mode CSR addresses
 * =================================================================== */
#define CSR_MISELECT_ADDR   0x350
#define CSR_MIREG_ADDR      0x351
#define CSR_MIREG2_ADDR     0x352
#define CSR_MIREG3_ADDR     0x353
/* 0x354 = miph, not mireg4 */
#define CSR_MIREG4_ADDR     0x355
#define CSR_MIREG5_ADDR     0x356
#define CSR_MIREG6_ADDR     0x357

/* ===================================================================
 * Sscsrind S-mode CSR addresses
 * =================================================================== */
#define CSR_SISELECT_ADDR   0x150
#define CSR_SIREG_ADDR      0x151
#define CSR_SIREG2_ADDR     0x152
#define CSR_SIREG3_ADDR     0x153
/* 0x154 = siph */
#define CSR_SIREG4_ADDR     0x155
#define CSR_SIREG5_ADDR     0x156
#define CSR_SIREG6_ADDR     0x157

/* ===================================================================
 * VS-mode CSR addresses (H extension)
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
 * menvcfg.CDE bit (bit 12) for Smcdeleg
 * =================================================================== */
#define MENVCFG_CDE         (1ULL << 12)

/* ===================================================================
 * mcounteren bits
 * =================================================================== */
#define MCOUNTEREN_CY       (1ULL << 0)
#define MCOUNTEREN_TM       (1ULL << 1)
#define MCOUNTEREN_IR       (1ULL << 2)

/* ===================================================================
 * M-mode CSR access helpers (miselect/mireg*)
 * =================================================================== */

static inline uintptr_t miselect_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x350" : "=r"(v) :: "memory");
    return v;
}

static inline void miselect_write(uintptr_t v) {
    asm volatile("csrw 0x350, %0" :: "r"(v) : "memory");
}

static inline uintptr_t mireg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x351" : "=r"(v) :: "memory");
    return v;
}

static inline void mireg_write(uintptr_t v) {
    asm volatile("csrw 0x351, %0" :: "r"(v) : "memory");
}

static inline uintptr_t mireg2_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x352" : "=r"(v) :: "memory");
    return v;
}

static inline void mireg2_write(uintptr_t v) {
    asm volatile("csrw 0x352, %0" :: "r"(v) : "memory");
}

static inline uintptr_t mireg3_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x353" : "=r"(v) :: "memory");
    return v;
}

static inline void mireg3_write(uintptr_t v) {
    asm volatile("csrw 0x353, %0" :: "r"(v) : "memory");
}

static inline uintptr_t mireg4_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x355" : "=r"(v) :: "memory");
    return v;
}

static inline void mireg4_write(uintptr_t v) {
    asm volatile("csrw 0x355, %0" :: "r"(v) : "memory");
}

static inline uintptr_t mireg5_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x356" : "=r"(v) :: "memory");
    return v;
}

static inline void mireg5_write(uintptr_t v) {
    asm volatile("csrw 0x356, %0" :: "r"(v) : "memory");
}

static inline uintptr_t mireg6_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x357" : "=r"(v) :: "memory");
    return v;
}

static inline void mireg6_write(uintptr_t v) {
    asm volatile("csrw 0x357, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * S-mode CSR access helpers (siselect/sireg*)
 * =================================================================== */

static inline uintptr_t siselect_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x150" : "=r"(v) :: "memory");
    return v;
}

static inline void siselect_write(uintptr_t v) {
    asm volatile("csrw 0x150, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x151" : "=r"(v) :: "memory");
    return v;
}

static inline void sireg_write(uintptr_t v) {
    asm volatile("csrw 0x151, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg2_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x152" : "=r"(v) :: "memory");
    return v;
}

static inline void sireg2_write(uintptr_t v) {
    asm volatile("csrw 0x152, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg3_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x153" : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t sireg4_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x155" : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t sireg5_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x156" : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t sireg6_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x157" : "=r"(v) :: "memory");
    return v;
}

/* ===================================================================
 * mstateen0 access helpers (CSR 0x30C)
 * =================================================================== */

static inline uintptr_t mstateen0_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x30C" : "=r"(v) :: "memory");
    return v;
}

static inline void mstateen0_write(uintptr_t v) {
    asm volatile("csrw 0x30C, %0" :: "r"(v) : "memory");
}

static inline void mstateen0_set(uintptr_t bits) {
    asm volatile("csrs 0x30C, %0" :: "r"(bits) : "memory");
}

static inline void mstateen0_clear(uintptr_t bits) {
    asm volatile("csrc 0x30C, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * hstateen0 access helpers (CSR 0x60C, H extension)
 * =================================================================== */

static inline uintptr_t hstateen0_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x60C" : "=r"(v) :: "memory");
    return v;
}

static inline void hstateen0_write(uintptr_t v) {
    asm volatile("csrw 0x60C, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * menvcfg access helpers (CSR 0x30A)
 * =================================================================== */

static inline uintptr_t menvcfg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x30A" : "=r"(v) :: "memory");
    return v;
}

static inline void menvcfg_write(uintptr_t v) {
    asm volatile("csrw 0x30A, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * mcounteren access helpers (CSR 0x306)
 * =================================================================== */

static inline uintptr_t mcounteren_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x306" : "=r"(v) :: "memory");
    return v;
}

static inline void mcounteren_write(uintptr_t v) {
    asm volatile("csrw 0x306, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * Feature detection
 * =================================================================== */

/* Check if Smcsrind is implemented (miselect CSR accessible) */
static inline bool platform_has_smcsrind(void) {
    clear_mdt();
    trap_expect_begin();
    miselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/* Check if Smstateen is implemented (mstateen0 CSR accessible) */
static inline bool platform_has_smstateen(void) {
    clear_mdt();
    trap_expect_begin();
    mstateen0_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/* Check if H extension is available via misa */
#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

/* Check if Smaia is implemented (miselect 0x30-0x3F range accessible) */
static inline bool platform_has_smaia(void) {
    if (!platform_has_smcsrind()) return false;
    uintptr_t orig = miselect_read();
    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    miselect_write(orig);
    return rb == 0x30;
}

/* ===================================================================
 * Convenience macros for S-mode access testing
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

#endif /* SMCSRIND_TEST_HELPERS_H */
