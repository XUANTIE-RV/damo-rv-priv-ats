/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zaamo suite.
 *
 * All test files are #included into test_register.c, so the static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Spec: SPEC/riscv-isa-manual/src/unpriv/zaamo.adoc plus the Hypervisor
 *       bindings in priv/hypervisor.adoc and priv/machine.adoc
 *       (norm:mcause_exccode_st_sc_amo: AMO -> store/AMO exception class)
 *       and priv/supervisor.adoc (norm:store_page_fault_no_w: an AMO on an
 *       unreadable page always raises a store page-fault).
 *
 * Two-stage infrastructure is reused from common/hyp/two_stage_helpers.h;
 * htinst golden values and the implicit-walk victim builder come from
 * common/hyp/hyp_test_helpers.h.
 */

#ifndef HYPERVISOR_ZAAMO_TEST_HELPERS_H
#define HYPERVISOR_ZAAMO_TEST_HELPERS_H

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
#include "hyp/hyp_vs_capture.h"

/* ===================================================================
 * Feature availability
 *
 * Zaamo support is config-DECLARATION driven: every case gates with
 *   if (!H_AVAILABLE) TEST_SKIP("H extension not available");
 *   if (!ZAAMO_AVAILABLE) TEST_SKIP("Zaamo not implemented");
 * using the compile-time ZAAMO_AVAILABLE macro normalized in
 * common/capabilities.h from ZAAMO_SUPPORTED (or A_SUPPORTED, since the
 * A extension implies Zaamo). No wrapper macro is provided. Only the
 * first case (HZAMO-01) additionally probes the DUT (trap-armed
 * amoadd.w in M-mode) to verify alignment with the config declaration;
 * no other case probes.
 * =================================================================== */

#define HZAMO_SMP_SKIP_REASON \
    "multi-hart: common/entry.S parks every hart except hart 0, so no " \
    "secondary-hart bring-up or inter-hart synchronization exists"

/* ===================================================================
 * AMO primitives. Base .w/.d forms come from common/mem_ops.h
 * (mem_amo_*). aq/rl annotated forms use mnemonics (the A extension is in
 * the base MARCH), with .option norvc so each instruction is 4 bytes and
 * the trap handler can skip a faulting AMO with sepc += 4.
 * =================================================================== */

#define HZ_AMO_W(mnem, aqrl, out, addr, val) \
    asm volatile( \
        ".option push\n\t.option norvc\n\t" \
        mnem ".w" aqrl " %0, %2, (%1)\n\t" \
        ".option pop\n\t" \
        : "=r"(out) : "r"(addr), "r"(val) : "memory")

#if __riscv_xlen == 64
#define HZ_AMO_D(mnem, aqrl, out, addr, val) \
    asm volatile( \
        ".option push\n\t.option norvc\n\t" \
        mnem ".d" aqrl " %0, %2, (%1)\n\t" \
        ".option pop\n\t" \
        : "=r"(out) : "r"(addr), "r"(val) : "memory")
#endif

/* ===================================================================
 * VS/VU-mode AMO probe functions (run via two_stage_run_in_vs / _in_vu).
 * The argument is the target VA. On a fault the handler skips the 4-byte
 * AMO and records the cause; the probe then returns via the trampoline.
 * =================================================================== */

static uintptr_t hz_vs_amo_add_w(uintptr_t addr)
{
    return mem_amo_add_w(addr, 0x12345678UL);
}
#if __riscv_xlen == 64
static uintptr_t hz_vs_amo_add_d(uintptr_t addr)
{
    return (uintptr_t)mem_amo_add_d(addr, 0x0F1E2D3C4B5A6978ULL);
}
#endif

/* Execute the full 9-op AMO .w set; returns 0 if all completed. */
static uintptr_t hz_vs_all_amo_w(uintptr_t addr)
{
    volatile uint32_t *p = (volatile uint32_t *)addr;
    *p = 0x1000u;
    (void)mem_amo_add_w(addr, 1u);
    (void)mem_amo_and_w(addr, 0xFFFFu);
    (void)mem_amo_or_w(addr, 0x2u);
    (void)mem_amo_xor_w(addr, 0x3u);
    (void)mem_amo_swap_w(addr, 0x40u);
    (void)mem_amo_min_w(addr, 0x10u);
    (void)mem_amo_max_w(addr, 0x10u);
    (void)mem_amo_minu_w(addr, 0x10u);
    (void)mem_amo_maxu_w(addr, 0x10u);
    return 0;
}

/* aq/rl annotated amoadd.w (HZAMO-16 htinst bit retention). */
static uintptr_t hz_vs_amo_add_w_aq(uintptr_t addr)
{
    uintptr_t r; HZ_AMO_W("amoadd", ".aq", r, addr, 0x12345678UL); return r;
}
static uintptr_t hz_vs_amo_add_w_rl(uintptr_t addr)
{
    uintptr_t r; HZ_AMO_W("amoadd", ".rl", r, addr, 0x12345678UL); return r;
}
static uintptr_t hz_vs_amo_add_w_aqrl(uintptr_t addr)
{
    uintptr_t r; HZ_AMO_W("amoadd", ".aqrl", r, addr, 0x12345678UL); return r;
}
#if __riscv_xlen == 64
static uintptr_t hz_vs_amo_swap_d_rl(uintptr_t addr)
{
    uintptr_t r; HZ_AMO_D("amoswap", ".rl", r, addr, 0x123456789ABCDEF0ULL);
    return r;
}
#endif

/* VS-mode trap capture (g_hz_vs_* state, hz_vs_handler,
 * hz_vs_handler_install, hz_clear_gva_spv) is provided by
 * common/hyp/hyp_vs_capture.h. */
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

#endif /* HYPERVISOR_ZAAMO_TEST_HELPERS_H */
