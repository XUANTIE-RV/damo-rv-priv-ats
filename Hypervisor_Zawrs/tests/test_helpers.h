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
 * Feature availability
 *
 * Zawrs support is config-DECLARATION driven: every case gates with
 *   if (!ZAWRS_AVAILABLE) TEST_SKIP("Zawrs not implemented");
 * using the compile-time ZAWRS_AVAILABLE macro normalized in
 * common/capabilities.h from ZAWRS_SUPPORTED (rvtest_config.h). No
 * wrapper macro is provided. Only the first case (HZWRS-01)
 * additionally probes the DUT (trap-armed wrs.sto in M-mode) to verify
 * the runtime behavior is aligned with the config declaration; no
 * other case probes.
 * =================================================================== */

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

/* Write MTIMECMP safely across XLENs. On RV64 a single 64-bit store
 * is atomic. On RV32 a naive low-then-high halfword split can leave
 * an intermediate value below mtime and raise a spurious MTIP; park
 * the high half at the maximum first (same sequence as
 * sm_mtimecmp_write in Sm_Interrupts), then set the low half and the
 * real high half so every intermediate value stays above mtime.
 *
 * The machine timer addresses come from the platform config
 * (PLATFORM_MTIMECMP_ADDR / PLATFORM_MTIME_ADDR). Every config/
 * entry defines them, so no local fallback is provided: a platform
 * missing the macros fails the build instead of silently using the
 * legacy centralized CLINT layout (per-hart MTIMER platforms do not
 * follow that layout). */
static inline void hz_write_mtimecmp(uint64_t v)
{
#if __riscv_xlen == 64
    *(volatile uint64_t *)PLATFORM_MTIMECMP_ADDR = v;
#else
    *(volatile uint32_t *)(PLATFORM_MTIMECMP_ADDR + 4) = 0xFFFFFFFFu;
    *(volatile uint32_t *)PLATFORM_MTIMECMP_ADDR = (uint32_t)v;
    *(volatile uint32_t *)(PLATFORM_MTIMECMP_ADDR + 4) = (uint32_t)(v >> 32);
    asm volatile("fence" ::: "memory");
#endif
}

/* ===================================================================
 * Interrupt quiescing (for VTW/TW illegal/virtual-instruction cases)
 *
 * The VTW/TW timeout exceptions only fire when wrs.nto does not
 * complete within the bounded time limit, i.e. no locally enabled
 * interrupt may be pending at any level. Clear every source and
 * enable, and return the previous mie for restoration.
 * =================================================================== */

static inline uintptr_t hz_quiet_interrupts(void)
{
    /* mie/mip are M-level CSRs: under S-mode-normal proxy them via
     * tsbi_csr_*. hvip/vsie are HS-level and stay direct. */
    uintptr_t saved_mie = tsbi_csr_read(CSR_MIE);
    tsbi_csr_write(CSR_MIE, 0);
    tsbi_csr_clear(CSR_MIP, (1UL << 1));   /* SSIP */
    hvip_write(0);
    vsie_write(0);
    hz_write_mtimecmp((uint64_t)-1);
    return saved_mie;
}

/* Restore mie after hz_quiet_interrupts(): call sites use
 * tsbi_csr_write(CSR_MIE, saved_mie) (mie is M-level). */

/* Force the HS/VS global interrupt enables off (sstatus.SIE,
 * vsstatus.SIE). Wake sources used by these tests must only prevent the
 * wrs stall; with a global enable left on, the pending interrupt would
 * actually be taken mid-test and corrupt the trap record.
 *
 * mstatus.MIE is deliberately NOT touched: it only gates interrupts while
 * the hart runs IN M-mode, whereas under S-mode-normal the wrs stimulus
 * runs in HS/VS/VU where M-interrupts are always enabled regardless of
 * MIE. M sources are quiesced via mie/mip instead, and MIE is a
 * return-path bit the tsbi_csr_* guardrail refuses to set/clear. */
static inline void hz_suppress_globals(void)
{
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

static inline uintptr_t hz_set_m_soft_pending(void)
{
    /* Delegate the SSIP to S-level first: an undelegated SSIP targets
     * M-mode and is taken immediately once the hart drops below M
     * (e.g. right after the mret that enters HS-mode), and the handler
     * clears it before the wrs instruction executes (machine.adoc
     * norm:intr_mip_mie_op condition (a): privilege lower than the
     * target). Delegated, it only traps when sstatus.SIE=1, which
     * hz_suppress_globals() keeps 0, so it stays pending. */
    /* The previous mideleg is returned so the paired
     * hz_clear_m_soft_pending() can restore it, mirroring
     * zawrs_delegate_ssip()/zawrs_restore_mideleg() in
     * Zawrs/tests/zawrs_helper.h: no delegation state leaks into
     * later cases even when a case fails in between. */
    uintptr_t saved_mideleg = tsbi_csr_read(CSR_MIDELEG);
    tsbi_csr_set(CSR_MIDELEG, (1UL << 1));
    tsbi_csr_set(CSR_MIP, (1UL << 1));     /* SSIP */
    tsbi_csr_set(CSR_MIE, (1UL << 1));     /* SSIE */
    return saved_mideleg;
}

static inline void hz_clear_m_soft_pending(uintptr_t saved_mideleg)
{
    tsbi_csr_clear(CSR_MIP, (1UL << 1));
    tsbi_csr_clear(CSR_MIE, (1UL << 1));
    tsbi_csr_write(CSR_MIDELEG, saved_mideleg);
}

/* VS-level: hvip.VSSIP injected, hideleg bit 2 routes the VS software
 * interrupt to VS-level, vsie.SSIE locally enables it - a locally
 * enabled pending interrupt for VS/VU-mode per the wfi resume rules.
 * Used by the record-only VTW case (HZWRS-06).
 *
 * Bit-numbering per hypervisor.adoc: VSSIP is VS-level interrupt
 * cause 2, so the delegation bit is hideleg[2] (norm:hideleg_acc:
 * bits 10/6/2 are writable, bits 12/9/5/1 are read-only zeros).
 * With hideleg[2]=1, vsip.SSIP and vsie.SSIE (bit 1) are aliases of
 * hip.VSSIP and hie.VSSIE (norm:vsip_vsie_ssi); with hideleg[2]=0
 * they are read-only zeros, so the delegation must be programmed
 * before enabling vsie.SSIE.
 *
 * Bug fix: this constant previously used bit 1 (the S-level
 * delegation bit, read-only zero per norm:hideleg_acc), so the write
 * was silently dropped by hardware, vsie.SSIE stayed read-only zero
 * and the VS wake source never became pending - HZWRS-02 then hit a
 * legal unbounded wrs.nto stall. */

#define HZ_HVIP_VSSIP     (1UL << 2)
#define HZ_HIDELEG_VSSI   (1UL << 2)
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
 * Watchdog harness (bounded stall for the VU normal-execution cases)
 *
 * A pending locally enabled interrupt always targets VS or above and
 * is taken on VU-mode entry, so a pending-interrupt wake source cannot
 * be maintained in VU-mode. Instead arm an M-timer with MTIE (M-interrupts
 * are always enabled while the hart is in VU, below M): if the
 * implementation stalls, the watchdog
 * interrupt terminates the stall (norm:Zawrs_exec_resume_rules) and
 * the framework M-mode handler disarms the source. A recorded trap
 * holding the M-timer interrupt cause is a legitimate outcome; a
 * synchronous cause means the wrs instruction itself trapped.
 * =================================================================== */

/* HZ_WATCHDOG_TICKS: the stall bound, in timer ticks. Typical
 * timebases are 10-25 MHz, so 200000 ticks is 8-20 ms - long enough
 * that a wrs which completes without stalling finishes (and disarms
 * the watchdog) well before it fires, and short enough to bound a
 * real stall far below any test timeout. The exact value is not
 * critical: any millisecond-order bound satisfies both edges. Kept
 * in sync with WRS_WATCHDOG_TICKS in Zawrs/tests/zawrs_helper.h. */
#define HZ_WATCHDOG_TICKS 200000UL

/* Arm the M-timer watchdog. Returns the previous mie so the caller
 * can pass it to hz_disarm_watchdog(); while armed, mie is fully
 * owned by the watchdog (all other enables cleared so the armed
 * window records at most the M-timer interrupt). */
static inline uintptr_t hz_arm_watchdog(void)
{
    uintptr_t saved_mie = tsbi_csr_read(CSR_MIE);
    tsbi_csr_write(CSR_MIE, 0);
    tsbi_csr_clear(CSR_MIP, (1UL << 1));   /* SSIP */
    hvip_write(0);
    hz_write_mtimecmp(*(volatile uint64_t *)PLATFORM_MTIME_ADDR +
                      HZ_WATCHDOG_TICKS);
    tsbi_csr_set(CSR_MIE, (1UL << IRQ_M_TIMER));
    /* No mstatus.MIE set: the watchdog M-timer interrupt fires while the
     * hart is in VU (below M), where M-interrupts are always enabled
     * regardless of mstatus.MIE; MIE is also a guardrail-blocked
     * return-path bit under the tsbi_csr_* proxy. */
    return saved_mie;
}

/* Disarm the watchdog and restore the mie snapshot taken by
 * hz_arm_watchdog(). mip.MTIP is driven by the MTIMER compare: the
 * max-value MTIMECMP write below clears a pending MTIP automatically
 * (a CSRC(mip, MTIP) would be ignored - the bit is read-only while
 * an MTIMER backs it). */
static inline void hz_disarm_watchdog(uintptr_t saved_mie)
{
    /* mstatus.MIE was never set by hz_arm_watchdog (see note there). */
    tsbi_csr_write(CSR_MIE, saved_mie);
    hz_write_mtimecmp((uint64_t)-1);
}

/* True when the armed trap record is either empty or holds only the
 * watchdog M-timer interrupt (asynchronous, no epc advance). Any
 * synchronous cause - or any asynchronous cause other than the
 * M-timer - returns false and fails the test case; nothing is
 * silently accepted, so an unexpected interrupt or a corrupted
 * record surfaces as a FAIL instead of a pass. */
static inline bool hz_trap_is_watchdog_only(void)
{
    if (!trap_was_triggered())
        return true;
    uintptr_t c = trap_get_cause();
    bool watchdog = (c & CAUSE_INTERRUPT_BIT) != 0 &&
                    (c & ~CAUSE_INTERRUPT_BIT) == IRQ_M_TIMER;
    if (!watchdog)
        LOG_E("NON-WATCHDOG TRAP: cause=0x%lx epc=0x%lx tval=0x%lx\n",
              (unsigned long)c,
              (unsigned long)trap_get_epc(),
              (unsigned long)trap_get_tval());
    return watchdog;
}

/* ===================================================================
 * hstatus.VTW / mstatus.TW control
 *
 * VTW: use hstatus_set_vtw(bool) from common/hyp/hyp_csr.h.
 * TW:  call sites use tsbi_csr_set/clear(CSR_MSTATUS, MSTATUS_TW_BIT)
 *      (mstatus is M-level; TW is a config bit the proxy allows).
 * =================================================================== */

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
            LOG_E("UNEXPECTED TRAP: cause=%lu epc=0x%lx tval=0x%lx\n",
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
            LOG_E("UNEXPECTED TRAP: cause=%lu epc=0x%lx tval=0x%lx\n",
                  (unsigned long)trap_get_cause(),
                  (unsigned long)trap_get_epc(),
                  (unsigned long)trap_get_tval());
        TEST_ASSERT("VU-mode no trap", !trap_was_triggered());
    }
    trap_expect_end();
}

#endif /* HYPERVISOR_ZAWRS_TEST_HELPERS_H */
