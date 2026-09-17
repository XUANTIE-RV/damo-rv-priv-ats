/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * shlcofideleg_helpers.c - Shlcofideleg test helper implementations
 *
 * Provides VS-mode trampoline functions for the Shlcofideleg
 * compliance tests. Extension availability is gated inline at each
 * call site via the compile-time SHLCOFIDELEG_AVAILABLE macro.
 * =================================================================== */

#include "shlcofideleg_helpers.h"

/* ===================================================================
 * VS-mode trampoline functions
 *
 * vs_read_sip / vs_read_sie are provided by the shared helpers in
 * common/hyp/hyp_test_helpers.c (identical semantics).
 * =================================================================== */

uintptr_t vs_set_sie_lcofi(uintptr_t arg) {
    (void)arg;
    asm volatile("csrs sie, %0" :: "r"(LCOFI_BIT));
    uintptr_t val;
    asm volatile("csrr %0, sie" : "=r"(val));
    return val;
}
