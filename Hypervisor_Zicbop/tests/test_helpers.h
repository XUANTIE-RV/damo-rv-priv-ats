/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Hypervisor x Zicbop cross tests
 *
 * Provides prefetch instruction trampolines for VS/VU-mode and
 * extension detection.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 */

#ifndef HYPERVISOR_ZICBOP_TEST_HELPERS_H
#define HYPERVISOR_ZICBOP_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_fence.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/test_vs_helpers.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/hyp_platform.h"
#include "cmo.h"

/* ===================================================================
 * Linker-provided symbols
 * =================================================================== */
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];
extern char __cmo_test_data_start[];

#ifndef TEST_REGION_BASE
#define TEST_REGION_BASE  ((uintptr_t)__vm_test_region_start)
#endif

/* ===================================================================
 * henvcfg CMO field accessors (CSR 0x60A)
 * =================================================================== */

static inline void henvcfg_set_cbie(unsigned cbie)
{
    uintptr_t val = henvcfg_read();
    val = (val & ~ENVCFG_CBIE_MASK) | ((uintptr_t)cbie << ENVCFG_CBIE_SHIFT);
    henvcfg_write(val);
}

static inline void henvcfg_set_cbcfe(unsigned en)
{
    uintptr_t val = henvcfg_read();
    if (en)
        val |= ENVCFG_CBCFE;
    else
        val &= ~ENVCFG_CBCFE;
    henvcfg_write(val);
}

static inline void henvcfg_set_cbze(unsigned en)
{
    uintptr_t val = henvcfg_read();
    if (en)
        val |= ENVCFG_CBZE;
    else
        val &= ~ENVCFG_CBZE;
    henvcfg_write(val);
}

/* ===================================================================
 * H extension detection
 * =================================================================== */
static bool check_h_extension(void)
{
    uint64_t misa_val = CSRR(misa);
    return (misa_val & (1UL << ('H' - 'A'))) != 0;
}

#define H_REQUIRED_OR_SKIP() do { \
    if (!check_h_extension()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * Prefetch instruction trampolines for VS/VU-mode
 *
 * These functions run inside VS/VU-mode. They execute a single
 * prefetch instruction on the address passed as arg. Prefetch should
 * NEVER trap regardless of configuration.
 * =================================================================== */

static uintptr_t vs_prefetch_r(uintptr_t arg)
{
    PREFETCH_R(arg);
    return 0;
}

static uintptr_t vs_prefetch_w(uintptr_t arg)
{
    PREFETCH_W(arg);
    return 0;
}

static uintptr_t vs_prefetch_i(uintptr_t arg)
{
    PREFETCH_I(arg);
    return 0;
}

#endif /* HYPERVISOR_ZICBOP_TEST_HELPERS_H */
