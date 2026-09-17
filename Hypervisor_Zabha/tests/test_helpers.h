/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zabha suite.
 *
 * All test files are #included into test_register.c, so the static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Spec: SPEC/riscv-isa-manual/src/unpriv/zabha.adoc plus the Hypervisor
 *       bindings. Byte/half AMOs use funct3=000/001. Key Zabha-specific
 *       points: htinst retains funct3 (access width); byte AMOs are never
 *       misaligned (1-byte alignment always holds); reserved byte/half
 *       lr/sc encodings are not HS-qualified -> illegal-instruction
 *       (cause=2), NOT virtual-instruction (cause=22); amocas.b/h ignore
 *       rd high bits (norm:Zabha_amocas-BH_ignore_bits).
 *
 * Two-stage infrastructure is reused from common/hyp/two_stage_helpers.h;
 * htinst golden values and the implicit-walk victim builder come from
 * common/hyp/hyp_test_helpers.h.
 */

#ifndef HYPERVISOR_ZABHA_TEST_HELPERS_H
#define HYPERVISOR_ZABHA_TEST_HELPERS_H

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
 * Zabha support is config-DECLARATION driven: every case gates with
 *   if (!H_AVAILABLE) TEST_SKIP("H extension not available");
 *   if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
 * using the compile-time ZABHA_AVAILABLE macro normalized in
 * common/capabilities.h from ZABHA_SUPPORTED. The amocas.b/h sub-group
 * (HZABHA-32~36) additionally gates on ZACAS_AVAILABLE (Zacas is a
 * config declaration too). No wrapper macro is provided. Only the first
 * case (HZABHA-01) additionally probes the DUT (trap-armed amoadd.b in
 * M-mode) to verify alignment with the config declaration; no other
 * case probes.
 * =================================================================== */

#define HZABHA_SMP_SKIP_REASON \
    "multi-hart: common/entry.S parks every hart except hart 0, so no " \
    "secondary-hart bring-up or inter-hart synchronization exists"

/* ===================================================================
 * Byte/half AMO probes. rd receives the sign-extended old value
 * (norm:Zabha_rd_sign_extension: byte -> 8-bit, half -> 16-bit).
 * =================================================================== */

static uintptr_t hz_vs_amo_add_b(uintptr_t addr)
{
    uintptr_t r;
    asm volatile(".option push\n\t.option norvc\n\t"
        "amoadd.b %0, %2, (%1)\n\t.option pop\n\t"
        : "=r"(r) : "r"(addr), "r"(1UL) : "memory");
    return r;
}
static uintptr_t hz_vs_amo_add_h(uintptr_t addr)
{
    uintptr_t r;
    asm volatile(".option push\n\t.option norvc\n\t"
        "amoadd.h %0, %2, (%1)\n\t.option pop\n\t"
        : "=r"(r) : "r"(addr), "r"(1UL) : "memory");
    return r;
}

/* Execute the full 9-op set for both .b and .h (HZABHA-01/02/03).
 * NOTE: rd uses early-clobber ("=&r") so the compiler does not alias it
 * with the address register; otherwise the first AMO overwrites rd with
 * the loaded value and the next AMO would use it as the address. */
static uintptr_t hz_vs_all_amo_bh(uintptr_t addr)
{
    uintptr_t r;
    asm volatile(".option push\n\t.option norvc\n\t"
        "amoadd.b  %0, %2, (%1)\n\t"
        "amoand.b  %0, %2, (%1)\n\t"
        "amoor.b   %0, %2, (%1)\n\t"
        "amoxor.b  %0, %2, (%1)\n\t"
        "amoswap.b %0, %2, (%1)\n\t"
        "amomin.b  %0, %2, (%1)\n\t"
        "amomax.b  %0, %2, (%1)\n\t"
        "amominu.b %0, %2, (%1)\n\t"
        "amomaxu.b %0, %2, (%1)\n\t"
        ".option pop\n\t"
        : "=&r"(r) : "r"(addr), "r"(1UL) : "memory");
    asm volatile(".option push\n\t.option norvc\n\t"
        "amoadd.h  %0, %2, (%1)\n\t"
        "amoand.h  %0, %2, (%1)\n\t"
        "amoor.h   %0, %2, (%1)\n\t"
        "amoxor.h  %0, %2, (%1)\n\t"
        "amoswap.h %0, %2, (%1)\n\t"
        "amomin.h  %0, %2, (%1)\n\t"
        "amomax.h  %0, %2, (%1)\n\t"
        "amominu.h %0, %2, (%1)\n\t"
        "amomaxu.h %0, %2, (%1)\n\t"
        ".option pop\n\t"
        : "=&r"(r) : "r"(addr), "r"(1UL) : "memory");
    return 0;
}

/* aq/rl annotated byte/half AMOs (HZABHA-17 htinst bit retention). */
static uintptr_t hz_vs_amo_add_b_aq(uintptr_t addr)
{
    uintptr_t r;
    asm volatile(".option push\n\t.option norvc\n\t"
        "amoadd.b.aq %0, %2, (%1)\n\t.option pop\n\t"
        : "=r"(r) : "r"(addr), "r"(1UL) : "memory");
    return r;
}
static uintptr_t hz_vs_amo_add_h_rl(uintptr_t addr)
{
    uintptr_t r;
    asm volatile(".option push\n\t.option norvc\n\t"
        "amoadd.h.rl %0, %2, (%1)\n\t.option pop\n\t"
        : "=r"(r) : "r"(addr), "r"(1UL) : "memory");
    return r;
}
static uintptr_t hz_vs_amo_add_b_aqrl(uintptr_t addr)
{
    uintptr_t r;
    asm volatile(".option push\n\t.option norvc\n\t"
        "amoadd.b.aqrl %0, %2, (%1)\n\t.option pop\n\t"
        : "=r"(r) : "r"(addr), "r"(1UL) : "memory");
    return r;
}

/* ===================================================================
 * Reserved byte/half lr/sc encodings (HZABHA-37). Zabha omits byte/half
 * lr/sc, so funct3=000/001 with funct5=00010(lr)/00011(sc) are reserved.
 * They are not HS-qualified, so V=1 execution raises illegal-instruction
 * (cause=2), NOT virtual-instruction (cause=22). Injected via .insn r.
 * =================================================================== */

static uintptr_t hz_vs_lr_b_reserved(uintptr_t addr)
{
    uintptr_t r;
    asm volatile(".option push\n\t.option norvc\n\t"
        ".insn r 0x2f, 0x0, 0x08, %0, %1, x0\n\t"
        ".option pop\n\t"
        : "=r"(r) : "r"(addr) : "memory");
    return r;
}
static uintptr_t hz_vs_sc_b_reserved(uintptr_t addr)
{
    uintptr_t r;
    asm volatile(".option push\n\t.option norvc\n\t"
        ".insn r 0x2f, 0x0, 0x0c, %0, %1, %2\n\t"
        ".option pop\n\t"
        : "=r"(r) : "r"(addr), "r"(1UL) : "memory");
    return r;
}
static uintptr_t hz_vs_lr_h_reserved(uintptr_t addr)
{
    uintptr_t r;
    asm volatile(".option push\n\t.option norvc\n\t"
        ".insn r 0x2f, 0x1, 0x08, %0, %1, x0\n\t"
        ".option pop\n\t"
        : "=r"(r) : "r"(addr) : "memory");
    return r;
}
static uintptr_t hz_vs_sc_h_reserved(uintptr_t addr)
{
    uintptr_t r;
    asm volatile(".option push\n\t.option norvc\n\t"
        ".insn r 0x2f, 0x1, 0x0c, %0, %1, %2\n\t"
        ".option pop\n\t"
        : "=r"(r) : "r"(addr), "r"(1UL) : "memory");
    return r;
}

/* ===================================================================
 * amocas.b/h probes (Zabha x Zacas, HZABHA-32~36). rd holds the compare
 * value (in) and receives the loaded value (out); rs2 holds the swap.
 * norm:Zabha_amocas-BH_ignore_bits: only rd[7:0]/rd[15:0] are compared.
 * =================================================================== */
static volatile uintptr_t hz_cas_cmp;
static volatile uintptr_t hz_cas_swap;
static volatile uintptr_t hz_cas_loaded;

static uintptr_t hz_vs_amocas_b_probe(uintptr_t addr)
{
    uintptr_t rd = hz_cas_cmp, sw = hz_cas_swap;
    asm volatile(".option push\n\t.option norvc\n\t"
        "amocas.b %0, %2, (%1)\n\t.option pop\n\t"
        : "+r"(rd) : "r"(addr), "r"(sw) : "memory");
    hz_cas_loaded = rd;
    return rd;
}
static uintptr_t hz_vs_amocas_h_probe(uintptr_t addr)
{
    uintptr_t rd = hz_cas_cmp, sw = hz_cas_swap;
    asm volatile(".option push\n\t.option norvc\n\t"
        "amocas.h %0, %2, (%1)\n\t.option pop\n\t"
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
 * HS-mode routing for GVA/SPV verification.
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
#define HZ_VS_INV     (0)

#define HZ_G_RWXU     (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)
#define HZ_G_RU       (PTE_V|PTE_R          |PTE_U|PTE_A|PTE_D)  /* no W */
#define HZ_G_INV      (0)

#endif /* HYPERVISOR_ZABHA_TEST_HELPERS_H */
