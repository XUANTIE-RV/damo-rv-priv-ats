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
#include "csr_ops.h"

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

/* menvcfg.CDE bit */
#define MENVCFG_CDE         (1ULL << 12)

/* mcounteren bits */
#define MCOUNTEREN_CY       (1ULL << 0)
#define MCOUNTEREN_TM       (1ULL << 1)
#define MCOUNTEREN_IR       (1ULL << 2)

/* ===================================================================
 * S-mode CSR access helpers (siselect/sireg*)
 * =================================================================== */

/* ===================================================================
 * M-mode CSR access helpers (for setup)
 * =================================================================== */

/* mstateen0 accessors delegate to common/csr_ops.h mstateen_*(idx=0) */
static inline uintptr_t mstateen0_read(void) { return mstateen_read(0); }
static inline void mstateen0_write(uintptr_t v) { mstateen_write(0, v); }
static inline void mstateen0_set(uintptr_t bits) { mstateen_set_bits(0, bits); }
static inline void mstateen0_clear(uintptr_t bits) { mstateen_clear_bits(0, bits); }

/* ===================================================================
 * Feature detection
 * =================================================================== */

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
