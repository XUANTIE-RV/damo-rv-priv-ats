/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Ssqosid test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 */

#ifndef HYP_SSQOSID_TEST_HELPERS_H
#define HYP_SSQOSID_TEST_HELPERS_H

#include "test_framework.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_priv.h"

/* ===================================================================
 * srmcfg CSR access helpers (CSR 0x181)
 *
 * srmcfg format (SXLEN=64):
 *   [11:0]  RCID  (WARL)
 *   [15:12] WPRI
 *   [27:16] MCID  (WARL)
 *   [63:28] WPRI
 * =================================================================== */

#define CSR_SRMCFG          0x181

#define SRMCFG_RCID_MASK    0xFFFUL
#define SRMCFG_RCID_SHIFT   0
#define SRMCFG_MCID_MASK    0xFFFUL
#define SRMCFG_MCID_SHIFT   16

static inline uintptr_t srmcfg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_SRMCFG) : "=r"(v) :: "memory");
    return v;
}

static inline void srmcfg_write(uintptr_t v) {
    asm volatile("csrw " CSR_STR(CSR_SRMCFG) ", %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * mstateen0 CSR access helpers (CSR 0x30C)
 * =================================================================== */

#define CSR_MSTATEEN0       0x30C
#define MSTATEEN0_BIT55     (1UL << 55)

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

/* ===================================================================
 * Smstateen / Ssqosid availability
 *
 * Both are config-declaration driven: gate on the compile-time
 * SMSTATEEN_AVAILABLE / SSQOSID_AVAILABLE macros (normalized in
 * common/capabilities.h from *_SUPPORTED in rvtest_config.h). Do NOT
 * trap-probe mstateen0 / srmcfg at runtime.
 * =================================================================== */

/* ===================================================================
 * Helper: test that S-mode CSR access succeeds (no trap)
 * =================================================================== */
#define SSQOSID_TEST_SMODE_ALLOWED(msg, csr_stmt) do { \
    goto_priv(PRIV_S); \
    PRIV_DO(csr_stmt); \
    goto_priv(PRIV_M); \
    CHECK_NO_TRAP(msg); \
} while (0)

#endif /* HYP_SSQOSID_TEST_HELPERS_H */
