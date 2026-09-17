/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHLCOFIDELEG_HELPERS_H
#define SHLCOFIDELEG_HELPERS_H

/* ===================================================================
 * Shlcofideleg test helpers
 *
 * Shared constants, forward declarations, and VS-mode trampolines for
 * the Shlcofideleg compliance tests. Extension availability is gated
 * inline via the compile-time SHLCOFIDELEG_AVAILABLE macro from
 * capabilities.h.
 * =================================================================== */

#include "test_framework.h"
#include "encoding.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test_helpers.h"   /* shared VS trampolines (vs_read_sip/sie, ...) */

/* Dynamic CSR read/write (defined in common/csr_accessors.c) */
extern uintptr_t csr_read(uint16_t csr);
extern void csr_write(uint16_t csr, uintptr_t val);

/* ===================================================================
 * LCOFI bit constant
 * =================================================================== */
#define LCOFI_BIT  (1UL << IRQ_LCOFI)   /* bit 13 */

/* ===================================================================
 * VS-mode trampoline functions
 *
 * These are passed to run_in_vs_mode(). In V=1, S-level CSR names
 * (sip, sie) map to their VS counterparts (vsip, vsie) automatically.
 * vs_read_sip / vs_read_sie are provided by common/hyp/hyp_test_helpers.c.
 * =================================================================== */

/* Set sie LCOFI bit via csrs (actually vsie when V=1). Returns sie value. */
uintptr_t vs_set_sie_lcofi(uintptr_t arg);

#endif /* SHLCOFIDELEG_HELPERS_H */
