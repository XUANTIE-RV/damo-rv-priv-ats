/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Smstateen test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 */

#ifndef HYPERVISOR_SMSTATEEN_TEST_HELPERS_H
#define HYPERVISOR_SMSTATEEN_TEST_HELPERS_H

#include "test_framework.h"

#ifdef ENABLE_HYP
#include "hyp/hyp_priv.h"
#endif

/* ===================================================================
 * mstateen0-3 CSR access helpers (M-mode, CSR 0x30C-0x30F)
 * =================================================================== */

static inline uintptr_t mstateen0_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_MSTATEEN0) : "=r"(v) :: "memory");
    return v;
}

static inline void mstateen0_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_MSTATEEN0) ", %0" :: "r"(v) : "memory");
}

static inline void mstateen0_set(uintptr_t bits) {
    asm volatile("csrs " CSR_STR(CSR_MSTATEEN0) ", %0" :: "r"(bits) : "memory");
}

static inline void mstateen0_clear(uintptr_t bits) {
    asm volatile("csrc " CSR_STR(CSR_MSTATEEN0) ", %0" :: "r"(bits) : "memory");
}

static inline uintptr_t mstateen1_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_MSTATEEN1) : "=r"(v) :: "memory");
    return v;
}

static inline void mstateen1_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_MSTATEEN1) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t mstateen2_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_MSTATEEN2) : "=r"(v) :: "memory");
    return v;
}

static inline void mstateen2_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_MSTATEEN2) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t mstateen3_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_MSTATEEN3) : "=r"(v) :: "memory");
    return v;
}

static inline void mstateen3_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_MSTATEEN3) ", %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * sstateen0-3 CSR access helpers (S-mode visible, CSR 0x10C-0x10F)
 * =================================================================== */

static inline uintptr_t sstateen0_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_SSTATEEN0) : "=r"(v) :: "memory");
    return v;
}

static inline void sstateen0_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_SSTATEEN0) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t sstateen1_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_SSTATEEN1) : "=r"(v) :: "memory");
    return v;
}

static inline void sstateen1_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_SSTATEEN1) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t sstateen2_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_SSTATEEN2) : "=r"(v) :: "memory");
    return v;
}

static inline void sstateen2_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_SSTATEEN2) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t sstateen3_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_SSTATEEN3) : "=r"(v) :: "memory");
    return v;
}

static inline void sstateen3_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_SSTATEEN3) ", %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * hstateen0-3 CSR access helpers (HS-mode, CSR 0x60C-0x60F)
 *
 * Always compiled (not gated by ENABLE_HYP) because tests use
 * compile-time H_AVAILABLE gating and TEST_SKIP when H ext is absent.
 * Access from M-mode uses raw CSR addresses and does not require
 * the H extension ISA string in -march.
 * =================================================================== */

static inline uintptr_t hstateen0_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_HSTATEEN0) : "=r"(v) :: "memory");
    return v;
}

static inline void hstateen0_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_HSTATEEN0) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t hstateen1_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_HSTATEEN1) : "=r"(v) :: "memory");
    return v;
}

static inline void hstateen1_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_HSTATEEN1) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t hstateen2_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_HSTATEEN2) : "=r"(v) :: "memory");
    return v;
}

static inline void hstateen2_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_HSTATEEN2) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t hstateen3_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_HSTATEEN3) : "=r"(v) :: "memory");
    return v;
}

static inline void hstateen3_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_HSTATEEN3) ", %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * senvcfg CSR access helpers (CSR 0x10A)
 * =================================================================== */

static inline uintptr_t senvcfg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_SENVCFG) : "=r"(v) :: "memory");
    return v;
}

static inline void senvcfg_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_SENVCFG) ", %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * Feature detection helpers
 * =================================================================== */

/* Check if misa.F (standard floating-point) is set */
#define HAS_MISA_F() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('F' - 'A'))) != 0; \
})

/* ===================================================================
 * Helper: check if a specific mstateen0 bit is writable
 *
 * Writes the bit, reads back, and restores original value.
 * Returns true if the bit is writable.
 * =================================================================== */
static inline bool mstateen0_bit_writable(uintptr_t bit) {
    uintptr_t orig = mstateen0_read();
    mstateen0_write(orig | bit);
    uintptr_t val = mstateen0_read();
    mstateen0_write(orig);
    return (val & bit) != 0;
}

/* ===================================================================
 * Helper: test that S-mode CSR access is blocked (illegal-instruction)
 *
 * Switches to S-mode, attempts the given CSR read statement,
 * returns to M-mode, and asserts illegal-instruction was raised.
 * =================================================================== */
#define SMSTATEEN_TEST_SMODE_BLOCKED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_TRAP(msg, CAUSE_ILLEGAL_INST); \
} while (0)

/* ===================================================================
 * Helper: test that S-mode CSR access is allowed (no trap)
 * =================================================================== */
#define SMSTATEEN_TEST_SMODE_ALLOWED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_NO_TRAP(msg); \
} while (0)

#endif /* HYPERVISOR_SMSTATEEN_TEST_HELPERS_H */
