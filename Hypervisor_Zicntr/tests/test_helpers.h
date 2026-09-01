/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zicntr test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Counter accesses use the canonical read-only CSR form
 * (csrrs rd, csr, x0), which is exactly the rdtime/rdcycle
 * instruction encoding per Zicntr (norm:zicntr_rdtime_op).
 */

#ifndef HYPERVISOR_ZICNTR_TEST_HELPERS_H
#define HYPERVISOR_ZICNTR_TEST_HELPERS_H

#include "test_framework.h"
#include "encoding.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_trap.h"

/* Dynamic CSR read (defined in common/csr_accessors.c) */
extern uintptr_t csr_read(uint16_t csr);

/* ===================================================================
 * counteren bit definitions
 * =================================================================== */

#define CNT_BIT_CY      (1UL << 0)
#define CNT_BIT_TM      (1UL << 1)
#define CNT_BIT_IR      (1UL << 2)

/* ===================================================================
 * htimedelta test parameters
 *
 * DELTA is chosen large enough that the elapsed-time window between
 * the VS/VU read and the HS/M reference read is negligible; BOUND
 * absorbs that window in the difference-interval assertions (the
 * real time naturally advances inside the test window, so exact
 * equality must NOT be required).
 * =================================================================== */

#define HZCNT_DELTA     0x1000000UL     /* +16M ticks */
#define HZCNT_NEG_DELTA 0xFFFFFFFFFFF00000UL  /* -0x100000 ticks */
#define HZCNT_BOUND     0x100000UL      /* tolerance window */

/* ===================================================================
 * Feature detection
 * =================================================================== */

#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

#define REQUIRE_H_EXT() do { \
    if (!HAS_H_EXT()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* Non-asserting Zicntr detection: trap-armed read of the time CSR
 * from M-mode. */
static inline bool check_zicntr(void)
{
    clear_mdt();
    trap_expect_begin();
    (void)csr_read(CSR_TIME);
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

#define REQUIRE_ZICNTR() do { \
    if (!check_zicntr()) { \
        TEST_SKIP("Zicntr not implemented (time CSR traps in M-mode)"); \
    } \
} while (0)

/* ===================================================================
 * VS/VU-mode counter read payloads (invoked via run_in_vs/vu_mode)
 *
 * "csrr rd, time" assembles to csrrs rd, 0xC01, x0 -- the canonical
 * rdtime encoding; likewise cycle == rdcycle.
 * =================================================================== */

static uintptr_t _vs_read_time(uintptr_t arg)
{
    (void)arg;
    uintptr_t v;
    asm volatile("csrr %0, time" : "=r"(v) :: "memory");
    return v;
}

static uintptr_t _vs_read_cycle(uintptr_t arg)
{
    (void)arg;
    uintptr_t v;
    asm volatile("csrr %0, cycle" : "=r"(v) :: "memory");
    return v;
}

/* ===================================================================
 * HS-mode rdtime payload (PRIV_DO after goto_priv(PRIV_S))
 *
 * With H ext present and V=0, S-mode == HS-mode. htimedelta must NOT
 * be added to this read path.
 * =================================================================== */

/* Global to capture HS-mode time value (PRIV_DO discards return values) */
static uintptr_t g_hs_time_val;

#define HS_RDTIME_CAPTURE() ({ \
    asm volatile("csrr %0, time" : "=r"(g_hs_time_val) :: "memory"); \
    g_hs_time_val; \
})

/* ===================================================================
 * counteren probe helpers
 *
 * hcounteren/scounteren/mcounteren are WARL; individual bits may be
 * read-only zero (norm:hcounteren_warl). Tests that need a specific
 * bit setting must verify the write took effect.
 * =================================================================== */

/* Try to set a hcounteren bit; returns true iff it stuck. */
static inline bool hcounteren_bit_set_sticky(uintptr_t bit)
{
    hcounteren_set(bit);
    return (hcounteren_read() & bit) != 0;
}

/* Try to clear a hcounteren bit; returns true iff it stuck. */
static inline bool hcounteren_bit_clear_sticky(uintptr_t bit)
{
    hcounteren_clear(bit);
    return (hcounteren_read() & bit) == 0;
}

/* Try to set an mcounteren bit; returns true iff it stuck. */
static inline bool mcounteren_bit_set_sticky(uintptr_t bit)
{
    mcounteren_set(bit);
    return (mcounteren_read() & bit) != 0;
}

/* Try to clear an mcounteren bit; returns true iff it stuck. */
static inline bool mcounteren_bit_clear_sticky(uintptr_t bit)
{
    mcounteren_clear(bit);
    return (mcounteren_read() & bit) == 0;
}

/* Try to set an scounteren bit; returns true iff it stuck. */
static inline bool scounteren_bit_set_sticky(uintptr_t bit)
{
    scounteren_set(bit);
    return (scounteren_read() & bit) != 0;
}

/* Try to clear an scounteren bit; returns true iff it stuck. */
static inline bool scounteren_bit_clear_sticky(uintptr_t bit)
{
    scounteren_clear(bit);
    return (scounteren_read() & bit) == 0;
}

/* ===================================================================
 * htimedelta helpers
 * =================================================================== */

/* Write htimedelta and verify the value stuck (it is a 64-bit RW
 * register per norm:htimedelta_sz_acc_op; return false otherwise). */
static inline bool htimedelta_set_checked(uint64_t delta)
{
    htimedelta_write(delta);
    return csr_read(CSR_HTIMEDELTA) == (uintptr_t)delta;
}

#endif /* HYPERVISOR_ZICNTR_TEST_HELPERS_H */
