/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zalrsc suite.
 *
 * All test files are #included into test_register.c, so the static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Spec: SPEC/riscv-isa-manual/src/unpriv/zalrsc.adoc plus the
 *       Hypervisor bindings in priv/hypervisor.adoc, priv/machine.adoc
 *       (norm:mcause_exccode_ld_ldrsv / norm:mcause_exccode_st_sc_amo)
 *       and priv/supervisor.adoc (norm:load_page_fault_no_r /
 *       norm:store_page_fault_no_w).
 *
 * Two-stage infrastructure (VS-stage Sv39 + G-stage Sv39x4) is reused
 * from common/hyp/two_stage_helpers.h; htinst golden values and the
 * implicit-walk victim builder come from common/hyp/hyp_test_helpers.h.
 */

#ifndef HYPERVISOR_ZALRSC_TEST_HELPERS_H
#define HYPERVISOR_ZALRSC_TEST_HELPERS_H

#include "test_framework.h"
#include "cause_defs.h"
#include "sm_defs.h"
#include "sh_defs.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/hyp_ldst.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/hyp_test_helpers.h"

/* Suite two-stage modes (default Sv39 + Sv39x4; Makefile may override). */
#ifndef SUITE_VSATP_MODE
#define SUITE_VSATP_MODE   SATP_MODE_SV39
#endif
#ifndef SUITE_HGATP_MODE
#define SUITE_HGATP_MODE   HGATP_MODE_SV39X4
#endif
#define HZ_VSMODE   SUITE_VSATP_MODE
#define HZ_GMODE    SUITE_HGATP_MODE

/* ===================================================================
 * Feature availability
 *
 * Zalrsc support is config-DECLARATION driven: every case gates with
 *   if (!H_AVAILABLE) TEST_SKIP("H extension not available");
 *   if (!ZALRSC_AVAILABLE) TEST_SKIP("Zalrsc not implemented");
 * using the compile-time ZALRSC_AVAILABLE macro normalized in
 * common/capabilities.h from ZALRSC_SUPPORTED (or A_SUPPORTED, since
 * the A extension implies Zalrsc). No wrapper macro is provided. Only
 * the first case (HZLRSC-01) additionally probes the DUT (trap-armed
 * lr.w/sc.w in M-mode) to verify alignment with the config declaration;
 * no other case probes.
 * =================================================================== */

/* Multi-hart limitation: common/entry.S parks every hart except hart 0. */
#define HZLRSC_SMP_SKIP_REASON \
    "multi-hart: common/entry.S parks every hart except hart 0, so no " \
    "secondary-hart bring-up or inter-hart synchronization exists"

/* ===================================================================
 * LR/SC primitives (mnemonics; A extension is in the base MARCH).
 *
 * lann / sann are ordering suffixes ("", ".aq", ".rl", ".aqrl"). They
 * must be string literals so the assembler template concatenates.
 * .option norvc keeps each instruction 4 bytes wide so the trap handler
 * can skip a faulting LR/SC with sepc += 4.
 * =================================================================== */

#define HZ_LR_W(lann, out_rd, addr) \
    asm volatile( \
        ".option push\n\t.option norvc\n\t" \
        "lr.w" lann " %0, (%1)\n\t" \
        ".option pop\n\t" \
        : "=r"(out_rd) : "r"(addr) : "memory")

#define HZ_SC_W(sann, out_rd, addr, val) \
    asm volatile( \
        ".option push\n\t.option norvc\n\t" \
        "sc.w" sann " %0, %2, (%1)\n\t" \
        ".option pop\n\t" \
        : "=r"(out_rd) : "r"(addr), "r"(val) : "memory")

#define HZ_PAIR_W(lann, sann, out_lr, out_sc, addr, val) \
    asm volatile( \
        ".option push\n\t.option norvc\n\t" \
        "lr.w" lann " %0, (%2)\n\t" \
        "sc.w" sann " %1, %3, (%2)\n\t" \
        ".option pop\n\t" \
        : "=&r"(out_lr), "=&r"(out_sc) \
        : "r"(addr), "r"(val) : "memory")

#if __riscv_xlen == 64
#define HZ_LR_D(lann, out_rd, addr) \
    asm volatile( \
        ".option push\n\t.option norvc\n\t" \
        "lr.d" lann " %0, (%1)\n\t" \
        ".option pop\n\t" \
        : "=r"(out_rd) : "r"(addr) : "memory")

#define HZ_SC_D(sann, out_rd, addr, val) \
    asm volatile( \
        ".option push\n\t.option norvc\n\t" \
        "sc.d" sann " %0, %2, (%1)\n\t" \
        ".option pop\n\t" \
        : "=r"(out_rd) : "r"(addr), "r"(val) : "memory")

#define HZ_PAIR_D(lann, sann, out_lr, out_sc, addr, val) \
    asm volatile( \
        ".option push\n\t.option norvc\n\t" \
        "lr.d" lann " %0, (%2)\n\t" \
        "sc.d" sann " %1, %3, (%2)\n\t" \
        ".option pop\n\t" \
        : "=&r"(out_lr), "=&r"(out_sc) \
        : "r"(addr), "r"(val) : "memory")
#endif /* __riscv_xlen == 64 */

/* .w rd sign-extension rule (norm:lr_sc_rv64). */
#define HZ_W_SIGNEXT(word) ((uintptr_t)(intptr_t)(int32_t)(word))

/* Retry budget for a single-shot pair whose success is asserted. */
#define HZLRSC_PAIR_TRIES   1000

/* ===================================================================
 * VS/VU-mode LR/SC probe functions
 *
 * Each runs inside VS/VU-mode via two_stage_run_in_vs / _in_vu. The
 * argument is the target VA. On a fault the (M-mode or delegated VS)
 * handler skips the 4-byte instruction and records the cause; the probe
 * then returns normally through the trampoline ecall.
 * =================================================================== */

/* Single lr.w (LR-class fault / LR-only-permission probes). */
static uintptr_t hz_vs_lr_w(uintptr_t addr)
{
    uintptr_t rd;
    HZ_LR_W("", rd, addr);
    return rd;
}

/* Single sc.w with NO preceding lr: the reservation is absent so the SC
 * must fail, yet it is still subject to the store permission check
 * (norm:sc_retire_permission + norm:sc_failed_as_store). */
static uintptr_t hz_vs_sc_w(uintptr_t addr)
{
    uintptr_t rd;
    HZ_SC_W("", rd, addr, 0xA5A5A5A5UL);
    return rd;
}

/* lr.w + sc.w pair; returns the SC rd (0 == success). */
static uintptr_t hz_vs_lrsc_w(uintptr_t addr)
{
    uintptr_t lr, sc;
    HZ_PAIR_W("", "", lr, sc, addr, 0x12345678UL);
    return sc;
}

/* Address holding a writable reservation for the "failed SC is still
 * permission-checked" probes. Mirrors Zalrsc ZLRSC-26 phase 2: the LR
 * establishes a reservation on a DIFFERENT writable page, so the SC to
 * the target must fail on address mismatch yet still undergo the
 * target's store permission check (removes any doubt about an
 * implementation short-cutting an SC that holds no reservation at all). */
static uintptr_t hz_resv_other_va;

static uintptr_t hz_vs_sc_w_holding_other(uintptr_t target)
{
    uintptr_t lr, sc;
    HZ_LR_W("", lr, hz_resv_other_va);
    HZ_SC_W("", sc, target, 0xA5A5A5A5UL);
    return sc;
}

/* Bounded-retry lr.w/sc.w pair; returns 0 once an SC succeeds. Used by
 * the normal-execution cases so a spurious single SC failure (allowed by
 * SPEC) does not masquerade as a functional fault. */
static uintptr_t hz_vs_lrsc_w_retry(uintptr_t addr)
{
    uintptr_t lr, sc = 1;
    for (int i = 0; i < HZLRSC_PAIR_TRIES && sc != 0; i++)
        HZ_PAIR_W("", "", lr, sc, addr, 0x12345678UL);
    return sc;
}

#if __riscv_xlen == 64
static uintptr_t hz_vs_lrsc_d_retry(uintptr_t addr)
{
    uintptr_t lr, sc = 1;
    for (int i = 0; i < HZLRSC_PAIR_TRIES && sc != 0; i++)
        HZ_PAIR_D("", "", lr, sc, addr, 0x0F1E2D3C4B5A6978ULL);
    return sc;
}
#endif /* __riscv_xlen == 64 */

/* aq/rl annotated probes (HZLRSC-17 htinst bit retention). The LR forms
 * fault on their own (G-stage invalid); the SC forms use a preceding LR
 * so the SC holds a reservation and reliably attempts the write (and
 * thus faults at a W=0 G-stage page), making its aq/rl bits observable
 * in htinst. */
static uintptr_t hz_vs_lr_w_aq(uintptr_t addr)
{
    uintptr_t rd; HZ_LR_W(".aq", rd, addr); return rd;
}
static uintptr_t hz_vs_lr_w_aqrl(uintptr_t addr)
{
    uintptr_t rd; HZ_LR_W(".aqrl", rd, addr); return rd;
}
static uintptr_t hz_vs_lr_sc_w_rl(uintptr_t addr)
{
    uintptr_t lr, sc; HZ_PAIR_W("", ".rl", lr, sc, addr, 0xA5A5A5A5UL);
    return sc;
}
static uintptr_t hz_vs_lr_sc_w_aqrl(uintptr_t addr)
{
    uintptr_t lr, sc; HZ_PAIR_W("", ".aqrl", lr, sc, addr, 0xA5A5A5A5UL);
    return sc;
}

/* ===================================================================
 * VS-mode trap handler (for hedeleg -> VS-mode delivery cases).
 *
 * Records vscause / vsepc / vstval, advances sepc by 4 (norvc LR/SC),
 * forces SPP=1 so sret returns to VS-mode (a VU-origin trap would
 * otherwise drop back to VU), then returns. Installed via
 * vs_trap_setup_direct() before entering VS-mode.
 * =================================================================== */

static volatile uintptr_t g_hz_vs_cause;
static volatile uintptr_t g_hz_vs_epc;
static volatile uintptr_t g_hz_vs_tval;
static volatile bool      g_hz_vs_triggered;

static void hz_vs_handler(void) __attribute__((naked, aligned(4)));
static void hz_vs_handler(void)
{
    asm volatile (
        "addi   sp, sp, -40\n\t"
        "sd     ra, 0(sp)\n\t"
        "sd     t0, 8(sp)\n\t"
        "sd     t1, 16(sp)\n\t"
        "sd     t2, 24(sp)\n\t"
        "csrr   t0, scause\n\t"
        "la     t2, g_hz_vs_cause\n\t"
        "sd     t0, 0(t2)\n\t"
        "csrr   t0, sepc\n\t"
        "la     t2, g_hz_vs_epc\n\t"
        "sd     t0, 0(t2)\n\t"
        "csrr   t0, stval\n\t"
        "la     t2, g_hz_vs_tval\n\t"
        "sd     t0, 0(t2)\n\t"
        "li     t0, 1\n\t"
        "la     t2, g_hz_vs_triggered\n\t"
        "sb     t0, 0(t2)\n\t"
        /* advance sepc past the 4-byte faulting LR/SC */
        "csrr   t0, sepc\n\t"
        "addi   t0, t0, 4\n\t"
        "csrw   sepc, t0\n\t"
        /* clear SIE(1)+SPIE(5); force SPP(8)=1 -> return to VS-mode */
        "li     t0, 0x22\n\t"
        "csrc   sstatus, t0\n\t"
        "li     t0, 0x100\n\t"
        "csrs   sstatus, t0\n\t"
        "ld     ra, 0(sp)\n\t"
        "ld     t0, 8(sp)\n\t"
        "ld     t1, 16(sp)\n\t"
        "ld     t2, 24(sp)\n\t"
        "addi   sp, sp, 40\n\t"
        "sret\n\t"
    );
}

/* Install the VS handler and clear the record. Caller must have set up
 * delegation via hyp_delegate_to_vs() beforehand. */
static inline void hz_vs_handler_install(void)
{
    g_hz_vs_cause = 0;
    g_hz_vs_epc = 0;
    g_hz_vs_tval = 0;
    g_hz_vs_triggered = false;
    vs_trap_setup_direct((uintptr_t)hz_vs_handler);
}

/* ===================================================================
 * HS-mode routing for GVA/SPV verification
 *
 * Hardware writes hstatus.GVA/SPV only when a trap is TAKEN INTO
 * HS-mode, not M-mode. The framework's M-mode capture path cannot
 * observe them, so any case asserting GVA/SPV must first delegate the
 * relevant cause to HS-mode via medeleg (hedeleg[21/23] are read-only-0,
 * so guest-page faults still stop at HS-mode and never reach VS-mode).
 * The framework's HS handler then snapshots hstatus.GVA/SPV plus
 * htval/htinst into the trap record (trap_get_gva/spv/htval/htinst).
 * =================================================================== */
static inline void hz_route_to_hs(uintptr_t mask)   { CSRS(medeleg, mask); }
static inline void hz_unroute_from_hs(uintptr_t mask){ CSRC(medeleg, mask); }

/* ===================================================================
 * VS-stage leaf PTE flag presets (A/D set unless a case needs A=0/D=0).
 * =================================================================== */

#define HZ_VS_RWX     (PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D)         /* S */
#define HZ_VS_R       (PTE_V|PTE_R          |PTE_A|PTE_D)            /* R=1 W=0 */
#define HZ_VS_XONLY   (PTE_V|PTE_X          |PTE_A|PTE_D)            /* R=0 (load faults) */
#define HZ_VS_RO_NOAD (PTE_V|PTE_R|PTE_W|PTE_X)                      /* A=0 */
#define HZ_VS_RW_ANOD (PTE_V|PTE_R|PTE_W|PTE_X|PTE_A)                /* A=1 D=0 */
#define HZ_VS_INV     (0)
#define HZ_VS_RWXU    (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)    /* VU */

#define HZ_G_RWXU     (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)
#define HZ_G_RU       (PTE_V|PTE_R          |PTE_U|PTE_A|PTE_D)      /* no W */
#define HZ_G_RO_NOAD  (PTE_V|PTE_R|PTE_X|PTE_U)                      /* A=0 */
#define HZ_G_INV      (0)

#endif /* HYPERVISOR_ZALRSC_TEST_HELPERS_H */
