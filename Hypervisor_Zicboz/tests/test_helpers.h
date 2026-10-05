/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Hypervisor x Zicboz cross tests
 *
 * Provides henvcfg CMO field accessors, cbo.zero trampoline for
 * VS/VU-mode, and extension detection.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 */

#ifndef HYPERVISOR_ZICBOZ_TEST_HELPERS_H
#define HYPERVISOR_ZICBOZ_TEST_HELPERS_H

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
#include "cmo.h"

/* ===================================================================
 * Linker-provided symbols
 * =================================================================== */
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];
extern char __cmo_test_data_start[];

/* henvcfg CMO field accessors (henvcfg_get/set_cbze) are provided by
 * common/cmo/cmo.h (CSR 0x60A). */

/* ===================================================================
 * H extension detection
 * =================================================================== */
/* H gate is inlined per case as:
 *   if (!H_AVAILABLE) TEST_SKIP("H extension not available"); */

/* ===================================================================
 * Zicboz availability
 *
 * Zicboz support is config-declaration driven: gate on the compile-time
 * ZICBOZ_AVAILABLE macro (normalized in common/capabilities.h from
 * ZICBOZ_SUPPORTED in rvtest_config.h). Do NOT probe menvcfg.CBZE
 * writability at runtime.
 * =================================================================== */

/* ===================================================================
 * cbo.zero trampoline for VS/VU-mode
 * =================================================================== */

static uintptr_t vs_cbo_zero(uintptr_t arg)
{
    CBO_ZERO(arg);
    return 0;
}

/* ===================================================================
 * htinst standard transformation value for cbo.zero
 * =================================================================== */
#define HTINST_CBO_ZERO    0x0040200FUL

/* stval_is_cbo_insn() is provided by common/cmo/cmo.h. */

#endif /* HYPERVISOR_ZICBOZ_TEST_HELPERS_H */
