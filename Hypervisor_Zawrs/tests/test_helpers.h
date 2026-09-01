/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zawrs suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Spec: SPEC/riscv-isa-manual/src/unpriv/zawrs.adoc
 *       (norm:Zawrs_virtual_instr_excp, norm:Zawrs_exec_resume_rules,
 *        norm:Zawrs_priv_illegal_instr_excp) plus the WFI-rule
 *       bindings in machine.adoc (norm:mstatus_tw_always_illegal) and
 *       hypervisor.adoc (norm:hstatus_vtw_op / norm:vtw_virtinstr).
 */

#ifndef HYPERVISOR_ZAWRS_TEST_HELPERS_H
#define HYPERVISOR_ZAWRS_TEST_HELPERS_H

#include "test_framework.h"
#include "cause_defs.h"
#include "sm_defs.h"
#include "sh_defs.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_trap.h"

/* ===================================================================
 * Encodings and raw executors
 *
 * wrs.nto / wrs.sto: SYSTEM opcode (0x73), funct3=0, rd=0, rs1=0,
 * funct12 = 0x00D / 0x01D. Injected as .word so the build march does
 * not need Zawrs mnemonic support. Statement-expression form so the
 * executors work inside PRIV_DO().
 * =================================================================== */

#define WRS_NTO_ENC     0x00D00073UL
#define WRS_STO_ENC     0x01D00073UL

#define EXEC_WRS_NTO() \
    ({ asm volatile(".word 0x00D00073" ::: "memory"); })
#define EXEC_WRS_STO() \
    ({ asm volatile(".word 0x01D00073" ::: "memory"); })

/* Shared reservation-set target for the wrs instructions. */
static volatile uintptr_t hz_wrs_slot;

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

/* Non-asserting Zawrs detection: execute wrs.sto in M-mode with a
 * locally enabled pending M-software-free wake (SSIP + SSIE) so the
 * instruction completes deterministically; an illegal-instruction
 * trap means the encoding is not implemented. */
static int hz_zawrs_cached = -1;

static inline bool hz_zawrs_present(void)
{
    if (hz_zawrs_cached < 0)
    {
        CSRS(mip, (1UL << 1));   /* SSIP */
        CSRS(mie, (1UL << 1));   /* SSIE */

        M_TRAP_EXPECT_BEGIN();
        EXEC_WRS_STO();
        bool trapped = trap_was_triggered();
        uintptr_t cause = trap_get_cause();
        trap_expect_end();

        CSRC(mip, (1UL << 1));
        CSRC(mie, (1UL << 1));

        hz_zawrs_cached =
            (trapped && cause == CAUSE_ILLEGAL_INST) ? 0 : 1;
    }
    return hz_zawrs_cached == 1;
}

#define REQUIRE_ZAWRS() do { \
    if (!hz_zawrs_present()) { \
        TEST_SKIP("Zawrs not implemented (raw wrs encoding raises " \
                  "illegal-instruction in M-mode probe)"); \
    } \
} while (0)

/* ===================================================================
 * Reservation set (Zawrs requires LR, from Zalrsc)
 * =================================================================== */

static inline uintptr_t hz_reserve(void)
{
    uintptr_t v;
#if __riscv_xlen == 64
    asm volatile("lr.d %0, (%1)" : "=r"(v) : "r"(&hz_wrs_slot) : "memory");
#else
    asm volatile("lr.w %0, (%1)" : "=r"(v) : "r"(&hz_wrs_slot) : "memory");
#endif
    return v;
}

/* ===================================================================
 * Interrupt quiescing (for VTW/TW illegal/virtual-instruction cases)
 *
 * The VTW/TW timeout exceptions only fire when wrs.nto does not
 * complete within the bounded time limit, i.e. no locally enabled
 * interrupt may be pending at any level. Clear every source and
 * enable, and return the previous mie for restoration.
 * =================================================================== */

#ifndef PLATFORM_MTIMECMP_ADDR
#define PLATFORM_MTIMECMP_ADDR  (PLATFORM_CLINT_BASE + 0x4000UL)
#endif

static inline uintptr_t hz_quiet_interrupts(void)
{
    uintptr_t saved_mie = CSRR(mie);
    CSRW(mie, 0);
    CSRC(mip, (1UL << 1));       /* SSIP */
    hvip_write(0);
    vsie_write(0);
    *(volatile uint64_t *)PLATFORM_MTIMECMP_ADDR = (uint64_t)-1;
    return saved_mie;
}

static inline void hz_restore_interrupts(uintptr_t saved_mie)
{
    CSRW(mie, saved_mie);
}

/* Force every global interrupt enable off (mstatus.MIE, HS-level
 * sstatus.SIE, vsstatus.SIE). Wake sources used by these tests must
 * only prevent the wrs stall; with a global enable left on, the
 * pending interrupt would actually be taken mid-test and corrupt the
 * trap record. */
static inline void hz_suppress_globals(void)
{
    CSRC(mstatus, MSTATUS_MIE_BIT);
    CSRC(sstatus, MSTATUS_SIE_BIT);
    CSRW(CSR_VSSTATUS, 0);
}

/* ===================================================================
 * Wake sources
 *
 * M-level: SSIP pending + mie.SSIE - a locally enabled interrupt at
 * M level, counted by the wfi resume rules at every privilege mode.
 * Used by the HS/VS/VU normal-execution cases for a deterministic
 * immediate completion.
 * =================================================================== */

static inline void hz_set_m_soft_pending(void)
{
    CSRS(mip, (1UL << 1));       /* SSIP */
    CSRS(mie, (1UL << 1));       /* SSIE */
}

static inline void hz_clear_m_soft_pending(void)
{
    CSRC(mip, (1UL << 1));
    CSRC(mie, (1UL << 1));
}

/* VS-level: hvip.VSSIP injected, hideleg[1] routes the VS software
 * interrupt to VS-level, vsie.SSIE locally enables it - a locally
 * enabled pending interrupt for VS/VU-mode per the wfi resume rules.
 * Used by the record-only VTW case (HZWRS-06). */

#define HZ_HVIP_VSSIP     (1UL << 2)
#define HZ_HIDELEG_VSSI   (1UL << 1)
#define HZ_VSIE_SSIE      (1UL << 1)

static inline void hz_set_vs_soft_pending(void)
{
    hideleg_write(hideleg_read() | HZ_HIDELEG_VSSI);
    vsie_write(vsie_read() | HZ_VSIE_SSIE);
    hvip_write(hvip_read() | HZ_HVIP_VSSIP);
}

static inline void hz_clear_vs_soft_pending(void)
{
    hvip_write(hvip_read() & ~HZ_HVIP_VSSIP);
    vsie_write(vsie_read() & ~HZ_VSIE_SSIE);
    hideleg_write(hideleg_read() & ~HZ_HIDELEG_VSSI);
}

/* ===================================================================
 * hstatus.VTW / mstatus.TW helpers
 * =================================================================== */

static inline void hz_set_vtw(void)
{
    hstatus_write(hstatus_read() | HSTATUS_VTW);
}

static inline void hz_clear_vtw(void)
{
    hstatus_write(hstatus_read() & ~HSTATUS_VTW);
}

static inline void hz_set_tw(void)  { CSRS(mstatus, MSTATUS_TW_BIT); }
static inline void hz_clear_tw(void) { CSRC(mstatus, MSTATUS_TW_BIT); }

/* ===================================================================
 * VS/VU-mode wrs callbacks (invoked via run_in_vs/vu_mode)
 *
 * hyp_reset_state() leaves vsatp/hgatp in Bare mode, so the callbacks
 * run on physical addresses with no translation. When the instruction
 * traps, the (delegated or M-mode) handler skips it and records the
 * cause; the callback then returns normally via the trampoline ecall.
 * =================================================================== */

static uintptr_t _vs_wrs_nto(uintptr_t arg)
{
    (void)arg;
    EXEC_WRS_NTO();
    return 0;
}

static uintptr_t _vs_wrs_sto(uintptr_t arg)
{
    (void)arg;
    EXEC_WRS_STO();
    return 0;
}

static uintptr_t _vu_wrs_nto(uintptr_t arg)
{
    (void)arg;
    EXEC_WRS_NTO();
    return 0;
}

static uintptr_t _vu_wrs_sto(uintptr_t arg)
{
    (void)arg;
    EXEC_WRS_STO();
    return 0;
}

/* Run a wrs callback in VS/VU mode with trap arming and assert the
 * expected outcome (cause match, or no trap). */
static void hz_vs_expect(uintptr_t (*fn)(uintptr_t),
                         bool expect_trap, uintptr_t cause)
{
    trap_expect_begin();
    (void)run_in_vs_mode(fn, 0);
    if (expect_trap) {
        TEST_ASSERT("VS-mode trap triggered", trap_was_triggered());
        if (trap_was_triggered())
            TEST_ASSERT_EQ("VS-mode cause", trap_get_cause(), cause);
    } else {
        if (trap_was_triggered())
            printf("  UNEXPECTED TRAP: cause=%lu epc=0x%lx tval=0x%lx\n",
                   (unsigned long)trap_get_cause(),
                   (unsigned long)trap_get_epc(),
                   (unsigned long)trap_get_tval());
        TEST_ASSERT("VS-mode no trap", !trap_was_triggered());
    }
    trap_expect_end();
}

static void hz_vu_expect(uintptr_t (*fn)(uintptr_t),
                         bool expect_trap, uintptr_t cause)
{
    trap_expect_begin();
    (void)run_in_vu_mode(fn, 0);
    if (expect_trap) {
        TEST_ASSERT("VU-mode trap triggered", trap_was_triggered());
        if (trap_was_triggered())
            TEST_ASSERT_EQ("VU-mode cause", trap_get_cause(), cause);
    } else {
        if (trap_was_triggered())
            printf("  UNEXPECTED TRAP: cause=%lu epc=0x%lx tval=0x%lx\n",
                   (unsigned long)trap_get_cause(),
                   (unsigned long)trap_get_epc(),
                   (unsigned long)trap_get_tval());
        TEST_ASSERT("VU-mode no trap", !trap_was_triggered());
    }
    trap_expect_end();
}

#endif /* HYPERVISOR_ZAWRS_TEST_HELPERS_H */
