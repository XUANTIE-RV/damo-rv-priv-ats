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
#include "cmo.h"

/* ===================================================================
 * Linker-provided symbols
 * =================================================================== */
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];
extern char __cmo_test_data_start[];

/* henvcfg CMO field accessors (henvcfg_set_cbie/cbcfe/cbze) are
 * provided by common/cmo/cmo.h (CSR 0x60A). */

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
