/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zacas suite.
 *
 * All test files are #included into test_register.c, so the static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Spec: SPEC/riscv-isa-manual/src/unpriv/zacas.adoc plus the Hypervisor
 *       bindings in priv/hypervisor.adoc and priv/machine.adoc
 *       (norm:mcause_exccode_st_sc_amo: amocas -> store/AMO class) and
 *       norm:Zacas_amocas_w_permission (amocas ALWAYS requires write
 *       permission, regardless of whether the comparison matches).
 *
 * amocas.w/d/q use funct5=00101. amocas.q (RV64) uses register pairs
 * (rd/rd+1 compare/load, rs2/rs2+1 swap). Two-stage infrastructure is
 * reused from common/hyp/two_stage_helpers.h; htinst golden values and the
 * implicit-walk victim builder come from common/hyp/hyp_test_helpers.h.
 */

#ifndef HYPERVISOR_ZACAS_TEST_HELPERS_H
#define HYPERVISOR_ZACAS_TEST_HELPERS_H

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
 * Zacas support is config-DECLARATION driven: every case gates with
 *   if (!H_AVAILABLE) TEST_SKIP("H extension not available");
 *   if (!ZACAS_AVAILABLE) TEST_SKIP("Zacas not implemented");
 * using the compile-time ZACAS_AVAILABLE macro normalized in
 * common/capabilities.h from ZACAS_SUPPORTED. No wrapper macro is
 * provided. Only the first case (HZACAS-01) additionally probes the
 * DUT (trap-armed amocas.w in M-mode) to verify alignment with the
 * config declaration; no other case probes.
 * =================================================================== */

#define HZACAS_SMP_SKIP_REASON \
    "multi-hart: common/entry.S parks every hart except hart 0, so no " \
    "secondary-hart bring-up or inter-hart synchronization exists"

/* ===================================================================
 * amocas operands. The VS probe signature is fn(uintptr_t arg), so the
 * compare/swap values are passed through these globals.
 *   hz_cas_cmp   - compare value loaded into rd before the CAS;
 *                  rd receives the loaded memory value afterwards.
 *   hz_cas_swap  - value stored to memory if the comparison matches.
 * =================================================================== */
static volatile uintptr_t hz_cas_cmp;
static volatile uintptr_t hz_cas_swap;
static volatile uintptr_t hz_cas_loaded;
#if __riscv_xlen == 64
static volatile uintptr_t hz_cas_cmp_hi;
static volatile uintptr_t hz_cas_swap_hi;
static volatile uintptr_t hz_cas_loaded_hi;
#endif

/* amocas.w probe: rd=cmp (in) -> loaded value (out); rs2=swap. */
static uintptr_t hz_vs_amocas_w_probe(uintptr_t addr)
{
    uintptr_t rd = hz_cas_cmp;
    uintptr_t sw = hz_cas_swap;
    asm volatile(
        ".option push\n\t.option norvc\n\t"
        "amocas.w %0, %2, (%1)\n\t"
        ".option pop\n\t"
        : "+r"(rd) : "r"(addr), "r"(sw) : "memory");
    hz_cas_loaded = rd;
    return rd;
}

#if __riscv_xlen == 64
/* amocas.d probe (single 64-bit register on RV64). */
static uintptr_t hz_vs_amocas_d_probe(uintptr_t addr)
{
    uintptr_t rd = hz_cas_cmp;
    uintptr_t sw = hz_cas_swap;
    asm volatile(
        ".option push\n\t.option norvc\n\t"
        "amocas.d %0, %2, (%1)\n\t"
        ".option pop\n\t"
        : "+r"(rd) : "r"(addr), "r"(sw) : "memory");
    hz_cas_loaded = rd;
    return rd;
}

/* amocas.q probe: register pair a0/a1 (compare/load), a2/a3 (swap).
 * Even pairs avoid the trap-handler-saved registers. */
static uintptr_t hz_vs_amocas_q_probe(uintptr_t addr)
{
    register uintptr_t r0 asm("a0") = hz_cas_cmp;
    register uintptr_t r1 asm("a1") = hz_cas_cmp_hi;
    register uintptr_t s0 asm("a2") = hz_cas_swap;
    register uintptr_t s1 asm("a3") = hz_cas_swap_hi;
    asm volatile(
        ".option push\n\t.option norvc\n\t"
        "amocas.q a0, a2, (%4)\n\t"
        ".option pop\n\t"
        : "+r"(r0), "+r"(r1)
        : "r"(s0), "r"(s1), "r"(addr)
        : "memory");
    hz_cas_loaded = r0;
    hz_cas_loaded_hi = r1;
    return r0;
}
#endif /* __riscv_xlen == 64 */

/* aq/rl annotated amocas.w (HZACAS-18 htinst bit retention). */
static uintptr_t hz_vs_amocas_w_aq(uintptr_t addr)
{
    uintptr_t rd = hz_cas_cmp, sw = hz_cas_swap;
    asm volatile(
        ".option push\n\t.option norvc\n\t"
        "amocas.w.aq %0, %2, (%1)\n\t"
        ".option pop\n\t"
        : "+r"(rd) : "r"(addr), "r"(sw) : "memory");
    hz_cas_loaded = rd;
    return rd;
}
static uintptr_t hz_vs_amocas_w_rl(uintptr_t addr)
{
    uintptr_t rd = hz_cas_cmp, sw = hz_cas_swap;
    asm volatile(
        ".option push\n\t.option norvc\n\t"
        "amocas.w.rl %0, %2, (%1)\n\t"
        ".option pop\n\t"
        : "+r"(rd) : "r"(addr), "r"(sw) : "memory");
    hz_cas_loaded = rd;
    return rd;
}
static uintptr_t hz_vs_amocas_w_aqrl(uintptr_t addr)
{
    uintptr_t rd = hz_cas_cmp, sw = hz_cas_swap;
    asm volatile(
        ".option push\n\t.option norvc\n\t"
        "amocas.w.aqrl %0, %2, (%1)\n\t"
        ".option pop\n\t"
        : "+r"(rd) : "r"(addr), "r"(sw) : "memory");
    hz_cas_loaded = rd;
    return rd;
}

/* ===================================================================
 * VS-mode trap handler (for hedeleg -> VS-mode delivery cases). Records
 * vscause/vsepc/vstval, advances sepc by 4, forces SPP=1, returns.
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
        "csrr   t0, sepc\n\t"
        "addi   t0, t0, 4\n\t"
        "csrw   sepc, t0\n\t"
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

static inline void hz_vs_handler_install(void)
{
    g_hz_vs_cause = 0;
    g_hz_vs_epc = 0;
    g_hz_vs_tval = 0;
    g_hz_vs_triggered = false;
    vs_trap_setup_direct((uintptr_t)hz_vs_handler);
}

/* ===================================================================
 * HS-mode routing for GVA/SPV verification (hstatus.GVA/SPV are written
 * only for traps taken into HS-mode; see norm:hstatus_gva_op/spv_op).
 * =================================================================== */
static inline void hz_route_to_hs(uintptr_t mask)   { CSRS(medeleg, mask); }
static inline void hz_unroute_from_hs(uintptr_t mask){ CSRC(medeleg, mask); }
static inline void hz_clear_gva_spv(void)
{
    hstatus_write(hstatus_read() & ~(HSTATUS_GVA | HSTATUS_SPV));
}

/* ===================================================================
 * VS-stage / G-stage leaf PTE flag presets.
 * =================================================================== */
#define HZ_VS_RWX     (PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D)
#define HZ_VS_R       (PTE_V|PTE_R          |PTE_A|PTE_D)   /* R=1 W=0 */
#define HZ_VS_XONLY   (PTE_V|PTE_X          |PTE_A|PTE_D)   /* R=0 */
#define HZ_VS_RW_ANOD (PTE_V|PTE_R|PTE_W|PTE_X|PTE_A)       /* A=1 D=0 */
#define HZ_VS_RWX_NOA (PTE_V|PTE_R|PTE_W|PTE_X)             /* A=0 D=0 */
#define HZ_VS_INV     (0)

#define HZ_G_RWXU     (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)
#define HZ_G_RU       (PTE_V|PTE_R          |PTE_U|PTE_A|PTE_D)  /* no W */
#define HZ_G_INV      (0)

#endif /* HYPERVISOR_ZACAS_TEST_HELPERS_H */
