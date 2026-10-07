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

/* srmcfg RCID/MCID field masks are in common/ss_defs.h */

/* srmcfg_read/write are provided by common/csr_ops.h (via hyp/hyp_csr.h) */

/* ===================================================================
 * mstateen0 CSR access helpers (CSR 0x30C)
 * =================================================================== */

#define MSTATEEN0_BIT55     (1UL << 55)

/* mstateen0 accessors are provided by common/hyp/hyp_csr.h as
 * mstateen_read/write/set_bits/clear_bits(idx=0, ...). */

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
