/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HZPM_SMMPM_TEST_HELPERS_H
#define HZPM_SMMPM_TEST_HELPERS_H

/* ===================================================================
 * Shared helpers for Hypervisor x Smmpm cross tests
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 7.
 * =================================================================== */

#include "test_framework.h"
#include "encoding.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/hyp_platform.h"
#include "pm/pm_cfg.h"
#include "pm/pm_addr.h"

/* ===================================================================
 * Capability gates
 * =================================================================== */

static bool check_h_extension(void) {
    uint64_t misa_val = CSRR(misa);
    return (misa_val & (1UL << ('H' - 'A'))) != 0;
}

#define H_REQUIRED_OR_SKIP() do { \
    if (!check_h_extension()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

#define SMMPM_REQUIRED_OR_SKIP() do { \
    H_REQUIRED_OR_SKIP(); \
    if (!detect_smmpm()) { \
        TEST_SKIP("Smmpm (mseccfg.PMM) not implemented"); \
    } \
} while (0)

/* ===================================================================
 * Scratch data / magics
 * =================================================================== */

#define HZPM_DATA1      (TEST_REGION_BASE + 0x0200)

#define HZPM_MAGIC1     0xDEADBEEFCAFE1234ULL
#define HZPM_MAGIC2     0x1234567890ABCDEFULL
#define HZPM_MAGIC3     0x0F0F0F0FF0F0F0F0ULL

/* ===================================================================
 * MPRV load helper
 *
 * Performs a single 8-byte load from @addr with mstatus.MPRV=1 and
 * the given MPV/MPP, bracketed entirely in inline asm so the
 * compiler cannot insert any load/store inside the MPRV window.
 * Traps are captured via trap_expect (armed inside); *trapped
 * reports whether the load faulted. mstatus is always restored.
 * =================================================================== */

static uint64_t hzpm_mprv_load(uintptr_t addr, unsigned mpp, int mpv,
                               bool *trapped) {
    uintptr_t saved = CSRR(mstatus);
    uintptr_t newms = saved;
    newms |= MSTATUS_MPRV_BIT;
    newms = (newms & ~MSTATUS_MPP_MASK)
            | ((uintptr_t)(mpp & 0x3) << MSTATUS_MPP_OFF);
    if (mpv) newms |=  MSTATUS_MPV;
    else     newms &= ~MSTATUS_MPV;

    uint64_t val;
    trap_expect_begin();
    asm volatile(
        "csrw mstatus, %[new_ms]\n\t"   /* MPRV=1, MPV/MPP set */
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[val], 0(%[addr])\n\t"      /* load via effective mode */
        ".option pop\n\t"
        "csrw mstatus, %[old_ms]\n\t"    /* restore: MPRV=0 */
        : [val] "=r"(val)
        : [new_ms] "r"(newms),
          [old_ms] "r"(saved),
          [addr] "r"(addr)
        : "memory"
    );
    *trapped = trap_was_triggered();
    trap_expect_end();

    CSRW(mstatus, saved);
    return val;
}

#endif /* HZPM_SMMPM_TEST_HELPERS_H */
