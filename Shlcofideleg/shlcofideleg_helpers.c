/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * shlcofideleg_helpers.c - Shlcofideleg test helper implementations
 *
 * Provides platform capability checks and VS-mode trampoline functions
 * for the Shlcofideleg compliance tests.
 * =================================================================== */

#include "shlcofideleg_helpers.h"

/* ===================================================================
 * Platform capability check
 *
 * Verifies:
 *   1. Sscofpmf is implemented: mip bit 13 is accessible
 *   2. Shlcofideleg is implemented: hideleg[13] is writable
 * =================================================================== */
bool shlcofideleg_check_available(void) {
    /* Check 1: Sscofpmf — try to read mie bit 13 */
    uintptr_t mie_val = CSRR(mie);
    (void)mie_val;  /* Just verifying the CSR is accessible */

    /* Check 2: Shlcofideleg — try writing hideleg[13]=1 */
    uintptr_t saved_hideleg = hideleg_read();
    hideleg_write(saved_hideleg | LCOFI_BIT);
    uintptr_t readback = hideleg_read();
    hideleg_write(saved_hideleg);

    if ((readback & LCOFI_BIT) == 0) {
        return false;  /* hideleg[13] is read-only zero → not implemented */
    }

    return true;
}

/* ===================================================================
 * VS-mode trampoline functions
 * =================================================================== */

uintptr_t vs_read_sip(uintptr_t arg) {
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, sip" : "=r"(val));
    return val;
}

uintptr_t vs_read_sie(uintptr_t arg) {
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, sie" : "=r"(val));
    return val;
}

uintptr_t vs_set_sie_lcofi(uintptr_t arg) {
    (void)arg;
    asm volatile("csrs sie, %0" :: "r"(LCOFI_BIT));
    uintptr_t val;
    asm volatile("csrr %0, sie" : "=r"(val));
    return val;
}
