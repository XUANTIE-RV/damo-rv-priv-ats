/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zca suite.
 *
 * All test files are #included into test_register.c, so the static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Spec: SPEC/riscv-isa-manual/src/unpriv/zca.adoc plus the Hypervisor
 *       bindings in priv/hypervisor.adoc and priv/machine.adoc
 *       (norm:H_trap_xtinst family, htinst_transformed_compressed /
 *       htinst_transformed_load / htinst_transformed_store /
 *       htinst_addr_offset).
 *
 * Two-stage infrastructure (VS-stage Sv39 + G-stage Sv39x4) is reused
 * from common/hyp/two_stage_helpers.h; the transform engine
 * hyp_transform_mem_inst() and the trap-record accessors come from
 * common/hyp/hyp_test_helpers.h and common/trap.c.
 *
 * Compressed transformation recap (htinst_transformed_compressed):
 *   (1) expand the 16-bit instruction to its 32-bit equivalent,
 *   (2) transform that 32-bit load/store (hyp_transform_mem_inst),
 *   (3) clear bit 1 -> bits[1:0] = 01 mark a compressed source.
 * The rs1 field (bits 19:15) of the expanded instruction is replaced by
 * Addr. Offset, so a compressed transformed value does NOT depend on
 * which base register the instruction used (c.lw and c.lwsp with the
 * same rd share one golden value for an aligned access).
 */

#ifndef HYPERVISOR_ZCA_TEST_HELPERS_H
#define HYPERVISOR_ZCA_TEST_HELPERS_H

#include "test_framework.h"
#include "cause_defs.h"
#include "sm_defs.h"
#include "sh_defs.h"
#include "mem_ops.h"
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
 * Capability gating
 *
 * Every case opens with two inline guards instead of a wrapper macro:
 *   if (!H_AVAILABLE)   TEST_SKIP("H extension not available");
 *   if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
 * Both come from common/capabilities.h. ZCA_AVAILABLE honors
 * ZCA_SUPPORTED or C_SUPPORTED (C implies Zca), so the platform
 * declaration is the sole authority for the extension's presence and no
 * runtime compressed-instruction probe is used. Cases exercising the
 * RV64-only doubleword forms (c.ld/c.sd/c.ldsp/c.sdsp, reserved on RV32)
 * add a third inline guard:
 *   if (__riscv_xlen != 64)
 *       TEST_SKIP("RV64-only compressed doubleword instruction");
 * =================================================================== */

/* ===================================================================
 * G-stage / VS-stage leaf PTE flag presets
 * =================================================================== */

#define HZ_G_RWXU     (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)
#define HZ_G_RU       (PTE_V|PTE_R          |PTE_U|PTE_A|PTE_D)  /* no W */
#define HZ_G_WNXU     (PTE_V|PTE_R|PTE_W    |PTE_U|PTE_A|PTE_D)  /* no X */
#define HZ_G_INV      (0)

#define HZ_VS_RWX     (PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D)
#define HZ_VS_RWXU    (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)

/* ===================================================================
 * HS-mode routing (htinst vs mtinst observation)
 *
 * A guest-page fault from VS/VU-mode is taken into HS-mode when
 * medeleg[cause]=1 (hedeleg[20/21/23] are read-only-0, so it never
 * reaches VS-mode) and into M-mode when medeleg[cause]=0. Hardware
 * writes htinst on the HS-mode entry and mtinst on the M-mode entry;
 * the framework snapshots whichever applies into trap_record.htinst
 * (hyp_capture_s / hyp_capture_m), read back via trap_get_htinst().
 * =================================================================== */
static inline void hz_route_to_hs(uintptr_t mask)    { CSRS(medeleg, mask); }
static inline void hz_unroute_from_hs(uintptr_t mask) { CSRC(medeleg, mask); }

/* ===================================================================
 * Compressed instruction encodings
 *
 * ENC_* : the exact 16-bit encoding of the trapping instruction (used
 *         to sanity-check the halfword fetched at xEPC).
 * EXP_* : the 32-bit equivalent the instruction expands to (per
 *         norm:c-*_op); fed to hyp_transform_mem_inst() then bit 1 is
 *         cleared to form the compressed transformed golden value.
 * All probes use a0 (x10, inside the x8..x15 compressed window) as the
 * base/rd/rs2, and offset 0, so these constants are fixed.
 * =================================================================== */

/* Register-based (rs1' = a0). */
#define HZCA_ENC_C_LW_A0    0x4108UL   /* c.lw  a0, 0(a0) */
#define HZCA_ENC_C_SW_A0    0xC108UL   /* c.sw  a0, 0(a0) */
#define HZCA_EXP_LW_A0_A0   0x00052503UL   /* lw a0, 0(a0) */
#define HZCA_EXP_SW_A0_A0   0x00A52023UL   /* sw a0, 0(a0) */
#if __riscv_xlen == 64
#define HZCA_ENC_C_LD_A0    0x6108UL   /* c.ld  a0, 0(a0) */
#define HZCA_ENC_C_SD_A0    0xE108UL   /* c.sd  a0, 0(a0) */
#define HZCA_EXP_LD_A0_A0   0x00053503UL   /* ld a0, 0(a0) */
#define HZCA_EXP_SD_A0_A0   0x00A53023UL   /* sd a0, 0(a0) */
#endif

/* Stack-pointer-based (base fixed to x2 = sp, offset 0). */
#define HZCA_ENC_C_LWSP_A0  0x4502UL   /* c.lwsp a0, 0(sp) */
#define HZCA_ENC_C_SWSP_A0  0xC02AUL   /* c.swsp a0, 0(sp) */
#define HZCA_EXP_LW_A0_SP   0x00012503UL   /* lw a0, 0(x2) */
#define HZCA_EXP_SW_A0_SP   0x00A12023UL   /* sw a0, 0(x2) */
#if __riscv_xlen == 64
#define HZCA_ENC_C_LDSP_A0  0x6502UL   /* c.ldsp a0, 0(sp) */
#define HZCA_ENC_C_SDSP_A0  0xE02AUL   /* c.sdsp a0, 0(sp) */
#define HZCA_EXP_LD_A0_SP   0x00013503UL   /* ld a0, 0(x2) */
#define HZCA_EXP_SD_A0_SP   0x00A13023UL   /* sd a0, 0(x2) */
#endif

/* ===================================================================
 * VS/VU-mode compressed fault probes
 *
 * Each is a naked function that executes EXACTLY ONE compressed
 * memory instruction against the target VA passed in a0, wrapped in
 * .option rvc so the trapping instruction is 2 bytes wide. The trap
 * handler skips it with xEPC += 2 (next_instruction()). aligned(16)
 * keeps the trapping instruction 4-byte aligned for clean reads.
 * =================================================================== */

/* --- Register-based: c.lw / c.sw / c.ld / c.sd (rs1' = a0 = VA) --- */
static uintptr_t hz_vs_c_lw(uintptr_t addr) __attribute__((naked, aligned(16)));
static uintptr_t hz_vs_c_lw(uintptr_t addr)
{
    asm volatile (
        ".option push\n\t.option rvc\n\t"
        "c.lw a0, 0(a0)\n\t"
        ".option pop\n\t"
        "ret\n\t" ::: "memory");
}

static uintptr_t hz_vs_c_sw(uintptr_t addr) __attribute__((naked, aligned(16)));
static uintptr_t hz_vs_c_sw(uintptr_t addr)
{
    asm volatile (
        ".option push\n\t.option rvc\n\t"
        "c.sw a0, 0(a0)\n\t"
        ".option pop\n\t"
        "ret\n\t" ::: "memory");
}

#if __riscv_xlen == 64
static uintptr_t hz_vs_c_ld(uintptr_t addr) __attribute__((naked, aligned(16)));
static uintptr_t hz_vs_c_ld(uintptr_t addr)
{
    asm volatile (
        ".option push\n\t.option rvc\n\t"
        "c.ld a0, 0(a0)\n\t"
        ".option pop\n\t"
        "ret\n\t" ::: "memory");
}

static uintptr_t hz_vs_c_sd(uintptr_t addr) __attribute__((naked, aligned(16)));
static uintptr_t hz_vs_c_sd(uintptr_t addr)
{
    asm volatile (
        ".option push\n\t.option rvc\n\t"
        "c.sd a0, 0(a0)\n\t"
        ".option pop\n\t"
        "ret\n\t" ::: "memory");
}
#endif

/* --- Stack-pointer-based: c.lwsp / c.swsp / c.ldsp / c.sdsp ---
 *
 * The base register of these instructions is fixed to x2 (sp), so the
 * probe temporarily points sp at the target VA (a0) with offset 0.
 *
 * SAFETY: the framework M/HS-mode trap entry (SAVE_CONTEXT in
 * common/trap_asm.S) uses the *incoming* sp as its register-save area
 * base (no mscratch/sscratch swap) and runs with physical addressing.
 * The target VA is test_fault_page, which is backed by real RAM -- its
 * G-stage V=0 only blocks VS/VU-mode accesses, NOT the M/HS physical
 * save. SAVE_CONTEXT then writes [sp-CONTEXT, sp), landing in the page
 * below (test_data_area), also valid RAM. The original sp is stashed in
 * g_hzca_sp_save and restored after the skipped faulting instruction,
 * so the trampoline ecall round-trip returns to M-mode with the correct
 * stack pointer. */
static uintptr_t g_hzca_sp_save __attribute__((used));

#define HZCA_SP_PROBE(name, cmn) \
static uintptr_t name(uintptr_t addr) __attribute__((naked, aligned(16))); \
static uintptr_t name(uintptr_t addr) \
{ \
    asm volatile ( \
        ".option push\n\t" \
        ".option norvc\n\t" \
        "la    t0, g_hzca_sp_save\n\t" \
        STORE " sp, 0(t0)\n\t" \
        "mv    sp, a0\n\t" \
        ".option rvc\n\t" \
        cmn "\n\t" \
        ".option norvc\n\t" \
        "la    t0, g_hzca_sp_save\n\t" \
        LOAD  " sp, 0(t0)\n\t" \
        ".option pop\n\t" \
        "ret\n\t" \
        ::: "memory"); \
}

HZCA_SP_PROBE(hz_vs_c_lwsp, "c.lwsp a0, 0(sp)")
HZCA_SP_PROBE(hz_vs_c_swsp, "c.swsp a0, 0(sp)")
#if __riscv_xlen == 64
HZCA_SP_PROBE(hz_vs_c_ldsp, "c.ldsp a0, 0(sp)")
HZCA_SP_PROBE(hz_vs_c_sdsp, "c.sdsp a0, 0(sp)")
#endif

/* --- Non-compressed contrast probe (HZCA-17): a 32-bit `lw a0,0(a0)`
 * (.option norvc) so its transformed htinst keeps bits[1:0] = 11. --- */
static uintptr_t hz_vs_lw_norvc(uintptr_t addr) __attribute__((naked, aligned(16)));
static uintptr_t hz_vs_lw_norvc(uintptr_t addr)
{
    asm volatile (
        ".option push\n\t.option norvc\n\t"
        "lw a0, 0(a0)\n\t"
        ".option pop\n\t"
        "ret\n\t" ::: "memory");
}

/* ===================================================================
 * Golden-value helpers for compressed transformed trap instructions
 * =================================================================== */

/* Compute the SPEC golden transformed value for a compressed memory
 * instruction from its 32-bit expansion. @tval is the captured faulting
 * VA (mtval/stval); @orig_va is the effective address the instruction
 * computed. Addr. Offset = @tval - @orig_va (0 for aligned accesses). */
static inline uintptr_t hzca_golden_from(uintptr_t expanded,
                                         uintptr_t tval, uintptr_t orig_va)
{
    uintptr_t off = (tval != 0 && tval >= orig_va) ? (tval - orig_va) : 0;
    return hyp_transform_mem_inst(expanded, off) & ~2UL;
}

/* Assert a trap-instruction register value (htinst when routed to
 * HS-mode, mtinst when trapped into M-mode) is 0 (always allowed for an
 * explicit access fault) or EXACTLY the compressed transformed golden.
 * Returns the golden for further field checks. */
static inline uintptr_t hzca_assert_xtinst_compressed(const char *msg,
                                                      uintptr_t xtinst,
                                                      uintptr_t expanded,
                                                      uintptr_t tval,
                                                      uintptr_t orig_va)
{
    uintptr_t golden = hzca_golden_from(expanded, tval, orig_va);
    TEST_ASSERT("golden transformed value computable", golden != 0);
    if (!(xtinst == 0 || xtinst == golden)) {
        printf("  [INFO] %s: trap-inst-reg=0x%lx golden=0x%lx tval=0x%lx\n",
               msg, (unsigned long)xtinst, (unsigned long)golden,
               (unsigned long)tval);
    }
    TEST_ASSERT(msg, xtinst == 0 || xtinst == golden);
    return golden;
}

/* Verify the trapping instruction fetched at xEPC is the expected 16-bit
 * compressed encoding (bits[1:0] != 11 confirm a compressed source). */
static inline void hzca_check_trap_inst_compressed(uintptr_t epc,
                                                   uintptr_t expect_half)
{
    uint16_t half = *(volatile uint16_t *)epc;
    TEST_ASSERT("trapping inst is compressed (bits[1:0] != 11)",
                (half & 3UL) != 3UL);
    TEST_ASSERT_EQ("trapping inst == expected compressed encoding",
                   (uintptr_t)half, expect_half);
}

/* ===================================================================
 * Captured-trap bundle for a compressed memory fault
 * =================================================================== */
typedef struct {
    bool      fired;
    uintptr_t cause;
    uintptr_t xtinst;   /* htinst (HS route) or mtinst (M route) */
    uintptr_t htval;
    uintptr_t tval;
    uintptr_t epc;
} hzca_trap_t;

/* Fire a compressed memory probe against a G-stage victim page at @va
 * (leaf flags @g_flags) and capture the trap. When @to_hs is true the
 * @exp_cause fault is routed into HS-mode (medeleg bit set) so hardware
 * writes htinst; otherwise it traps into M-mode and hardware writes
 * mtinst. Both are read back through trap_get_htinst(). All fields are
 * snapshotted before teardown so the caller can compute golden values
 * afterwards without a live trap record. */
static inline hzca_trap_t hzca_fire_mem_fault(uintptr_t (*probe)(uintptr_t),
                                              uintptr_t va, uintptr_t g_flags,
                                              uintptr_t exp_cause, bool to_hs)
{
    hzca_trap_t r;
    r.fired = false; r.cause = 0; r.xtinst = 0;
    r.htval = 0; r.tval = 0; r.epc = 0;

    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, va, g_flags);
    if (to_hs)
        hz_route_to_hs(1UL << exp_cause);   /* HS entry -> htinst */
    else
        hz_unroute_from_hs(1UL << exp_cause); /* M entry -> mtinst */

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, probe, va);
    r.fired  = trap_was_triggered();
    if (r.fired) {
        r.cause  = trap_get_cause();
        r.xtinst = trap_get_htinst();
        r.htval  = trap_get_htval();
        r.tval   = trap_get_tval();
        r.epc    = trap_get_epc();
    }
    trap_expect_end();

    hz_unroute_from_hs(1UL << exp_cause);
    ts2_finish(&ctx);
    return r;
}

#endif /* HYPERVISOR_ZCA_TEST_HELPERS_H */
