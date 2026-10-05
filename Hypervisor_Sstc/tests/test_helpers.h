/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Sstc cross test suite.
 *
 * All test files (test_hcross_sstc_stce.c / test_hcross_sstc_acc.c /
 * test_hcross_sstc_vs.c) are #included into test_register.c,
 * so static functions and globals defined here are visible across the
 * whole compilation unit.
 */

#ifndef HYPERVISOR_SSTC_TEST_HELPERS_H
#define HYPERVISOR_SSTC_TEST_HELPERS_H

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_csr.h"

/* ===================================================================
 * Sstc CSR accessors
 *
 * All CSR accessors used by this suite (stimecmp / vstimecmp / time /
 * hip / htimedelta / menvcfg / henvcfg / mcounteren / hcounteren / hvip)
 * are provided by common/hyp/hyp_csr.h. No suite-local CSR wrappers
 * remain here.
 * =================================================================== */

/* ===================================================================
 * Delay loop
 * =================================================================== */
#define DELAY_LOOP(n) do { \
    for (volatile int _dl = 0; _dl < (n); _dl++) { } \
} while (0)

#define HCROSS_SSTC_DELAY  100

/* ===================================================================
 * Globals provided by tests/sstc_strap.S
 * =================================================================== */
extern volatile uintptr_t g_sstc_trap_cause;
extern void               sstc_trap_entry(void);
extern unsigned long      sstc_trap_scratch[];

/* ===================================================================
 * VS-mode trampoline functions for run_in_vs_mode()
 * =================================================================== */

/* VS-mode: read stimecmp and return trap cause (0 if no trap) */
static uintptr_t _vs_stimecmp_read(uintptr_t arg)
{
    (void)arg;
    stimecmp_read();
    return trap_get_cause();
}

/* VS-mode: write stimecmp with given value */
static uintptr_t _vs_stimecmp_write(uintptr_t arg)
{
    stimecmp_write(arg);
    return 0;
}

/* VS-mode: trigger timer interrupt and capture via VS trap handler */
static uintptr_t _vs_timer_interrupt_test(uintptr_t arg)
{
    (void)arg;
    /* Enable sstatus.SIE (seen as vsstatus.SIE in VS-mode) */
    CSRS(sstatus, MSTATUS_SIE_BIT);

    /* Trigger VS timer: write stimecmp (= vstimecmp) to past value */
    uintptr_t now = time_read();
    stimecmp_write(now > 0 ? now - 1 : 0);

    /* Wait for interrupt */
    DELAY_LOOP(1000);

    CSRC(sstatus, MSTATUS_SIE_BIT);
    return 0;
}

#endif /* HYPERVISOR_SSTC_TEST_HELPERS_H */
