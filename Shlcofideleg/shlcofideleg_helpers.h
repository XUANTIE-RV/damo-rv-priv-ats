/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHLCOFIDELEG_HELPERS_H
#define SHLCOFIDELEG_HELPERS_H

/* ===================================================================
 * Shlcofideleg test helpers
 *
 * Shared constants, forward declarations, VS-mode trampolines, and
 * platform capability checks for the Shlcofideleg compliance tests.
 * =================================================================== */

#include "test_framework.h"
#include "encoding.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_reset.h"

/* Dynamic CSR read/write (defined in common/csr_accessors.c) */
extern uintptr_t csr_read(uint16_t csr);
extern void csr_write(uint16_t csr, uintptr_t val);

/* ===================================================================
 * LCOFI bit constant
 * =================================================================== */
#define LCOFI_BIT  (1UL << IRQ_LCOFI)   /* bit 13 */

/* ===================================================================
 * Platform capability check
 *
 * Returns true if:
 *   1. Sscofpmf is implemented (mip/mie bit 13 is accessible)
 *   2. Shlcofideleg is implemented (hideleg[13] is writable)
 *
 * When returning false, the caller should TEST_SKIP.
 * =================================================================== */
bool shlcofideleg_check_available(void);

/* ===================================================================
 * VS-mode trampoline functions
 *
 * These are passed to run_in_vs_mode(). In V=1, S-level CSR names
 * (sip, sie) map to their VS counterparts (vsip, vsie) automatically.
 * =================================================================== */

/* Read sip (actually vsip when V=1). */
uintptr_t vs_read_sip(uintptr_t arg);

/* Read sie (actually vsie when V=1). */
uintptr_t vs_read_sie(uintptr_t arg);

/* Set sie LCOFI bit via csrs (actually vsie when V=1). Returns sie value. */
uintptr_t vs_set_sie_lcofi(uintptr_t arg);

#endif /* SHLCOFIDELEG_HELPERS_H */
