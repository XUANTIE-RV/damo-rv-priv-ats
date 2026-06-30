/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Ssstateen test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * S-mode tests only (Groups 1-6).
 * H-mode tests (Groups 7-12) have been extracted to Hypervisor_Ssstateen/.
 */

#ifndef SSSTATEEN_TEST_HELPERS_H
#define SSSTATEEN_TEST_HELPERS_H

#include "test_framework.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_trap.h"

/* ===================================================================
 * Feature detection
 * =================================================================== */

#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

#define HAS_MISA_F() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('F' - 'A'))) != 0; \
})

#define REQUIRE_H_EXT() do { \
    if (!HAS_H_EXT()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * sstateen direct CSR access (inline asm, CSR 0x10C-0x10F)
 *
 * These duplicate the hyp_csr.h wrappers but are static inline so
 * they can be used from any privilege level within test code.
 * =================================================================== */

static inline uintptr_t sstateen0_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x10C" : "=r"(v) :: "memory");
    return v;
}

static inline void sstateen0_write(uintptr_t v) {
    asm volatile("csrw 0x10C, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sstateen1_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x10D" : "=r"(v) :: "memory");
    return v;
}

static inline void sstateen1_write(uintptr_t v) {
    asm volatile("csrw 0x10D, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sstateen2_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x10E" : "=r"(v) :: "memory");
    return v;
}

static inline void sstateen2_write(uintptr_t v) {
    asm volatile("csrw 0x10E, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sstateen3_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x10F" : "=r"(v) :: "memory");
    return v;
}

static inline void sstateen3_write(uintptr_t v) {
    asm volatile("csrw 0x10F, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * S-mode test helpers
 * =================================================================== */

/**
 * SS_TEST_SMODE_BLOCKED - Test that S-mode CSR access triggers
 *                         illegal-instruction trap.
 */
#define SS_TEST_SMODE_BLOCKED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_TRAP(msg, CAUSE_ILLEGAL_INST); \
} while (0)

/**
 * SS_TEST_SMODE_ALLOWED - Test that S-mode CSR access succeeds.
 */
#define SS_TEST_SMODE_ALLOWED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_NO_TRAP(msg); \
} while (0)

/**
 * SS_TEST_UMODE_BLOCKED - Test that U-mode CSR access triggers
 *                         illegal-instruction trap (cause=2).
 */
#define SS_TEST_UMODE_BLOCKED(msg, csr_stmt) do { \
    goto_priv(PRIV_U); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_TRAP(msg, CAUSE_ILLEGAL_INST); \
} while (0)

/**
 * SS_TEST_UMODE_ALLOWED - Test that U-mode CSR access succeeds.
 */
#define SS_TEST_UMODE_ALLOWED(msg, csr_stmt) do { \
    goto_priv(PRIV_U); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_NO_TRAP(msg); \
} while (0)

/* ===================================================================
 * VS-mode helpers for sstateen access (used by SS-UCTL-09, SS-EXC-04)
 * =================================================================== */

static uintptr_t _vs_read_sstateen0(uintptr_t arg) {
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, 0x10C" : "=r"(val));
    return val;
}

#endif /* SSSTATEEN_TEST_HELPERS_H */
