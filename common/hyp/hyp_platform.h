/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYP_PLATFORM_H
#define HYP_PLATFORM_H

/* ===================================================================
 * Platform capability matrix for H sub-extension tests
 *
 * Provides a unified, cached capability snapshot that all H
 * sub-extension tests can query to determine:
 *   - Which virtual memory modes are supported (satp/vsatp/hgatp)
 *   - Which sub-extensions are available (Ssstateen, vstvec Vectored)
 *   - Counter implementation details
 *   - ASID/VMID bit widths
 *
 * Design principle (per "H子扩展通用测试框架设计原则"):
 *   - Probe once, cache results, avoid repeated probing overhead
 *   - Each sub-extension test uses REQUIRE_* macros that consult
 *     this capability matrix
 *   - New sub-extensions add fields without modifying existing code
 * =================================================================== */

#include "types.h"
#include "encoding.h"

typedef struct {
    /* ----- Translation mode support ----- */

    /* satp supported modes */
    bool satp_sv39;           /* satp supports Sv39 */
    bool satp_sv48;           /* satp supports Sv48 */
    bool satp_sv57;           /* satp supports Sv57 */

    /* vsatp supported modes (should mirror satp per shvsatpa) */
    bool vsatp_sv39;
    bool vsatp_sv48;
    bool vsatp_sv57;

    /* hgatp supported modes */
    bool hgatp_sv39x4;        /* hgatp supports Sv39x4 */
    bool hgatp_sv48x4;        /* hgatp supports Sv48x4 */
    bool hgatp_sv57x4;        /* hgatp supports Sv57x4 */

    /* ----- Address space ID widths ----- */
    unsigned satp_asid_bits;   /* satp ASID width (0..16) */
    unsigned vsatp_asid_bits;  /* vsatp ASID width (0..16) */
    unsigned hgatp_vmid_bits;  /* hgatp VMID width (0..14) */

    /* ----- Sub-extension availability ----- */
    bool ssstateen;            /* Smstateen/Ssstateen implemented */
    bool vstvec_vectored;     /* vstvec supports Vectored mode */
    bool shcounterenw;        /* hcounteren is writable (non-zero readback) */

    /* ----- Counter information ----- */
    uint32_t counters_implemented;  /* bitmap: bit N set => HPM counter N exists */

    /* ----- Probing status ----- */
    bool probed;              /* true after platform_probe() completes */
} platform_caps_t;

/* ===================================================================
 * Primary API
 * =================================================================== */

/**
 * platform_probe - Perform one-time platform capability detection.
 *
 * Probes all relevant CSRs and populates the cached capability
 * structure. Safe to call multiple times (no-op after first call).
 *
 * Must be called from M-mode (needs access to all CSRs).
 *
 * Returns pointer to the cached, immutable capability structure.
 */
const platform_caps_t *platform_probe(void);

/**
 * platform_caps_get - Get the cached capability structure.
 *
 * Returns NULL if platform_probe() has not been called yet.
 * Prefer platform_probe() which auto-initializes.
 */
const platform_caps_t *platform_caps_get(void);

/* ===================================================================
 * Convenience: sub-extension gate macros
 *
 * Use these at the top of test functions to skip tests when the
 * required capability is not present.
 * =================================================================== */

#define REQUIRE_SATP_SV39() do { \
    if (!platform_probe()->satp_sv39) \
        TEST_SKIP("satp Sv39 not supported"); \
} while (0)

#define REQUIRE_SATP_SV48() do { \
    if (!platform_probe()->satp_sv48) \
        TEST_SKIP("satp Sv48 not supported"); \
} while (0)

#define REQUIRE_SATP_SV57() do { \
    if (!platform_probe()->satp_sv57) \
        TEST_SKIP("satp Sv57 not supported"); \
} while (0)

#define REQUIRE_VSATP_SV39() do { \
    if (!platform_probe()->vsatp_sv39) \
        TEST_SKIP("vsatp Sv39 not supported"); \
} while (0)

#define REQUIRE_VSATP_SV48() do { \
    if (!platform_probe()->vsatp_sv48) \
        TEST_SKIP("vsatp Sv48 not supported"); \
} while (0)

#define REQUIRE_VSATP_SV57() do { \
    if (!platform_probe()->vsatp_sv57) \
        TEST_SKIP("vsatp Sv57 not supported"); \
} while (0)

#define REQUIRE_SSSTATEEN() do { \
    if (!platform_probe()->ssstateen) \
        TEST_SKIP("Ssstateen not available"); \
} while (0)

#define REQUIRE_VSTVEC_VECTORED() do { \
    if (!platform_probe()->vstvec_vectored) \
        TEST_SKIP("vstvec Vectored mode not supported"); \
} while (0)

#define REQUIRE_SHCOUNTERENW() do { \
    if (!platform_probe()->shcounterenw) \
        TEST_SKIP("hcounteren not writable (Shcounterenw)"); \
} while (0)

/**
 * REQUIRE_SHA - Gate for Sha composite extension.
 *
 * Sha = H + Ssstateen + Shcounterenw + Shvstvala + Shtvala +
 *       Shvstvecd + Shvsatpa + Shgatpa
 *
 * This checks the detectable requirements. Shvstvala/Shtvala/Shvsatpa/
 * Shgatpa presence is inferred from the H-extension being present.
 */
#define REQUIRE_SHA() do { \
    const platform_caps_t *_p = platform_probe(); \
    if (!_p->ssstateen) \
        TEST_SKIP("Sha requires Ssstateen"); \
    if (!_p->shcounterenw) \
        TEST_SKIP("Sha requires Shcounterenw"); \
    if (!_p->vstvec_vectored) \
        TEST_SKIP("Sha requires Shvstvecd (vectored vstvec)"); \
} while (0)

/* ===================================================================
 * shgatpa consistency check macro
 *
 * Per shgatpa: for each satp SvNN, hgatp SvNNx4 must be supported.
 * =================================================================== */

#define CHECK_SHGATPA_CONSISTENCY() do { \
    const platform_caps_t *_p = platform_probe(); \
    if (_p->satp_sv39) \
        TEST_ASSERT("shgatpa: Sv39 -> Sv39x4", _p->hgatp_sv39x4); \
    if (_p->satp_sv48) \
        TEST_ASSERT("shgatpa: Sv48 -> Sv48x4", _p->hgatp_sv48x4); \
    if (_p->satp_sv57) \
        TEST_ASSERT("shgatpa: Sv57 -> Sv57x4", _p->hgatp_sv57x4); \
} while (0)

/* ===================================================================
 * shvsatpa consistency check macro
 *
 * Per shvsatpa: vsatp must support exactly the same modes as satp.
 * =================================================================== */

#define CHECK_SHVSATPA_CONSISTENCY() do { \
    const platform_caps_t *_p = platform_probe(); \
    TEST_ASSERT_EQ("shvsatpa: Sv39 satp==vsatp", \
                   (int)_p->satp_sv39, (int)_p->vsatp_sv39); \
    TEST_ASSERT_EQ("shvsatpa: Sv48 satp==vsatp", \
                   (int)_p->satp_sv48, (int)_p->vsatp_sv48); \
    TEST_ASSERT_EQ("shvsatpa: Sv57 satp==vsatp", \
                   (int)_p->satp_sv57, (int)_p->vsatp_sv57); \
} while (0)

#endif /* HYP_PLATFORM_H */
