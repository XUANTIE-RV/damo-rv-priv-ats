/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Hypervisor x Zicfilp cross tests
 *
 * Provides H/Zicfilp extension detection, CSR access helpers,
 * VS/VU-mode trampolines for indirect jump and LPAD testing, and
 * two-stage page table setup utilities.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 */

#ifndef HYPERVISOR_ZICFILP_TEST_HELPERS_H
#define HYPERVISOR_ZICFILP_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_ldst.h"
#include "hyp/hyp_fence.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/test_vs_helpers.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/hyp_platform.h"

/* ===================================================================
 * Linker-provided symbols
 * =================================================================== */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t test_exec_page[];
extern uint8_t test_exec_target[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];
extern char  __cfi_test_code_start[];
extern char  __cfi_test_code_end[];

#define TEST_REGION_BASE   ((uintptr_t)__vm_test_region_start)

/* ===================================================================
 * Exception cause codes
 * =================================================================== */
#define CAUSE_VIRTUAL_INSTRUCTION      22

/* ===================================================================
 * CSR Access Helpers
 *
 * Note: menvcfg_read/write, henvcfg_read/write, hedeleg_read/write,
 * hstatus_read/write are provided by hyp_csr.h. Only define helpers
 * not already in the framework.
 * =================================================================== */

static inline void menvcfg_set(uintptr_t mask) {
    asm volatile("csrs 0x30A, %0" :: "r"(mask) : "memory");
}

static inline void menvcfg_clear(uintptr_t mask) {
    asm volatile("csrc 0x30A, %0" :: "r"(mask) : "memory");
}

static inline uintptr_t mseccfg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x747" : "=r"(v));
    return v;
}

static inline void mseccfg_write(uintptr_t val) {
    asm volatile("csrw 0x747, %0" :: "r"(val) : "memory");
}

static inline void mseccfg_set(uintptr_t mask) {
    asm volatile("csrs 0x747, %0" :: "r"(mask) : "memory");
}

static inline void mseccfg_clear(uintptr_t mask) {
    asm volatile("csrc 0x747, %0" :: "r"(mask) : "memory");
}

static inline uintptr_t senvcfg_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x10A" : "=r"(v));
    return v;
}

static inline void senvcfg_write(uintptr_t val) {
    asm volatile("csrw 0x10A, %0" :: "r"(val) : "memory");
}

static inline void senvcfg_set(uintptr_t mask) {
    asm volatile("csrs 0x10A, %0" :: "r"(mask) : "memory");
}

static inline void senvcfg_clear(uintptr_t mask) {
    asm volatile("csrc 0x10A, %0" :: "r"(mask) : "memory");
}

static inline uintptr_t mstatus_read(void) {
    return CSRR(mstatus);
}

static inline uintptr_t vsstatus_read(void) {
    return CSRR(CSR_VSSTATUS);
}

static inline void vsstatus_write(uintptr_t val) {
    CSRW(CSR_VSSTATUS, val);
}

/* ===================================================================
 * H extension detection
 * =================================================================== */
static bool check_h_extension(void) {
    uint64_t misa = CSRR(misa);
    return (misa & (1UL << ('H' - 'A'))) != 0;
}

#define H_REQUIRED_OR_SKIP() do { \
    if (!check_h_extension()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * Zicfilp detection (via henvcfg.LPE writability)
 *
 * Attempts to set henvcfg.LPE. If it sticks, Zicfilp is considered
 * implemented for the Hypervisor context.
 * =================================================================== */
static bool zicfilp_detected = false;
static bool zicfilp_detection_done = false;

static bool detect_zicfilp(void) {
    if (zicfilp_detection_done)
        return zicfilp_detected;

    /* Try setting henvcfg.LPE */
    uintptr_t orig = henvcfg_read();
    henvcfg_write(orig | HENVCFG_LPE);
    uintptr_t val = henvcfg_read();
    if (val & HENVCFG_LPE) {
        henvcfg_write(orig);
        zicfilp_detected = true;
        zicfilp_detection_done = true;
        return true;
    }
    henvcfg_write(orig);

    /* Also try menvcfg.LPE as fallback */
    orig = menvcfg_read();
    menvcfg_set(MENVCFG_LPE);
    val = menvcfg_read();
    if (val & MENVCFG_LPE) {
        menvcfg_clear(MENVCFG_LPE);
        zicfilp_detected = true;
        zicfilp_detection_done = true;
        return true;
    }

    zicfilp_detected = false;
    zicfilp_detection_done = true;
    return false;
}

#define ZICFILP_REQUIRED_OR_SKIP() do { \
    if (!detect_zicfilp()) { \
        TEST_SKIP("Zicfilp not implemented"); \
    } \
} while (0)

/* ===================================================================
 * LPAD instruction encoding
 *
 * LPAD is encoded as: AUIPC x0, imm
 * With label=0: 0x00000017
 * =================================================================== */
#define LPAD_INSN_WORD  0x00000017

static inline void emit_lpad(void *addr) {
    *(volatile uint32_t *)addr = LPAD_INSN_WORD;
}

/* RET instruction: jalr x0, x1, 0 */
#define RET_INSN_WORD   0x00008067

static inline void emit_ret(void *addr) {
    *(volatile uint32_t *)addr = RET_INSN_WORD;
}

/* NOP instruction: addi x0, x0, 0 */
#define NOP_INSN_WORD   0x00000013

static inline void emit_nop(void *addr) {
    *(volatile uint32_t *)addr = NOP_INSN_WORD;
}

/* ECALL instruction */
#define ECALL_INSN_WORD 0x00000073

static inline void emit_ecall(void *addr) {
    *(volatile uint32_t *)addr = ECALL_INSN_WORD;
}

/* SD zero, 0(t0) instruction: store to the address in t0 */
#define SD_ZERO_T0_INSN_WORD  0x0002B023

static inline void emit_sd_zero_t0(void *addr) {
    *(volatile uint32_t *)addr = SD_ZERO_T0_INSN_WORD;
}

/* ===================================================================
 * VS-mode trampoline: simple NOP function (returns immediately).
 * =================================================================== */
static uintptr_t vs_nop_fn(uintptr_t arg) {
    (void)arg;
    return 0;
}

/* ===================================================================
 * VS-mode trampoline: indirect jump (JALR) to a target address.
 *
 * This function runs inside VS-mode. It performs an indirect jump
 * (jalr ra, addr) to the target. If the target starts with LPAD and
 * LPE=1, execution continues normally. If the target is not LPAD and
 * LPE=1, a software-check exception (cause=18) is triggered.
 *
 * Returns 0 on success, or the trap cause if a fault occurred.
 * =================================================================== */
/* Address of the JALR instruction inside vs_jalr_to_target, recorded
 * at runtime so tests can compare it against vsepc/sepc after a trap. */
static volatile uintptr_t g_jalr_addr;

static uintptr_t vs_jalr_to_target(uintptr_t addr) {
    trap_expect_begin();
    asm volatile(
        "la   t0, 1f\n\t"
        "la   t1, g_jalr_addr\n\t"
        "sd   t0, 0(t1)\n\t"
        "1:\n\t"
        "jalr ra, %0, 0\n\t"
        :
        : "r"(addr)
        : "t0", "t1", "ra", "memory"
    );
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode trampoline: JALR to target, followed by a single NOP.
 *
 * Used by HCFI-LP-13: after the first LP fault is handled without
 * clearing SPELP, the SRET-restored ELP=LP_EXPECTED must re-fault on
 * the NOP; skipping a NOP in the handler is always safe (unlike
 * skipping a random compiler-generated instruction).
 * =================================================================== */
static uintptr_t vs_jalr_then_nop(uintptr_t addr) {
    trap_expect_begin();
    asm volatile(
        "jalr ra, %0, 0\n\t"
        "nop\n\t"
        :
        : "r"(addr)
        : "ra", "memory"
    );
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode trampoline: SRET to VU-mode (vsstatus.SPP=0) at the given
 * address. Used by HCFI-LP-22/23 to verify ELP restore on a VS->VU
 * SRET (norm:Zicfilp_pelp_trap_return with yLPE = VULPE).
 *
 * Also loads t0 with g_vu_fault_addr: the VU target code uses it as
 * the address of a faulting store that delivers the return trap
 * (avoids relying on ecall-from-VU delegation).
 * =================================================================== */
static volatile uintptr_t g_vu_fault_addr;

/* NOTE: must be naked - this function never returns normally (it
 * srets away to VU-mode), so a compiler-generated stack frame would
 * never be unwound and would corrupt the _v_trampoline epilogue's
 * stack-relative ra restore. */
static uintptr_t vs_sret_to_vu(uintptr_t arg) __attribute__((naked));
static uintptr_t vs_sret_to_vu(uintptr_t arg) {
    (void)arg;
    asm volatile(
        "la   t0, g_vu_fault_addr\n\t"
        "ld   t0, 0(t0)\n\t"
        "csrw sepc, a0\n\t"
        "csrc sstatus, %0\n\t"
        "sret\n\t"
        :
        : "r"(0x100)
        : "t0", "memory");
}

/* Landing point (VS-mode, S-level page) where the VS-mode exception
 * handler resumes execution via g_vs_exc_recovery after a trap taken
 * in VU-mode. Simply returns to the _v_trampoline caller. */
static void vs_landing_fn(void) __attribute__((naked));
static void vs_landing_fn(void) {
    asm volatile("ret" ::: "memory");
}

/* ===================================================================
 * VS-mode trampoline: indirect jump (JALR without link) to target.
 *
 * Uses jalr x0, addr to perform a non-call indirect jump.
 * =================================================================== */
static uintptr_t vs_jr_to_target(uintptr_t addr) {
    trap_expect_begin();
    /* jalr x0 does not link, so the target's RET returns to whatever
     * ra currently holds. Load ra with the continuation address first,
     * otherwise the target's RET would jump back into this function's
     * prologue region and loop forever. */
    asm volatile(
        "la   ra, 1f\n\t"
        "jalr x0, %0, 0\n\t"
        "1:\n\t"
        :
        : "r"(addr)
        : "ra", "memory"
    );
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode trampoline: indirect jump (JALR) to an address whose
 * instruction fetch is expected to fault (e.g. unmapped page).
 *
 * Installs a recovery address via _exec_return_addr so the M-mode
 * trap handler resumes at the continuation label instead of skipping
 * ahead 4 bytes from the (unmapped) faulting PC, which would just
 * fault again. The JALR still sets ELP=LP_EXPECTED first, so this
 * exercises the access-fault vs software-check priority rule.
 * =================================================================== */
extern uintptr_t _exec_return_addr;

static uintptr_t vs_jalr_to_unmapped(uintptr_t addr) {
    trap_expect_begin();
    asm volatile(
        "la   t0, 1f\n\t"
        "la   t1, _exec_return_addr\n\t"
        "sd   t0, 0(t1)\n\t"
        "jalr ra, %0, 0\n\t"
        "1:\n\t"
        "sd   zero, 0(t1)\n\t"
        :
        : "r"(addr)
        : "t0", "t1", "ra", "memory"
    );
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode trampoline: execute LPAD instruction directly.
 * =================================================================== */
static uintptr_t vs_exec_lpad(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    asm volatile(
        ".word 0x00000017\n\t"   /* LPAD with label=0 */
        ::: "memory"
    );
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode trampoline: read senvcfg.
 * =================================================================== */
static uintptr_t vs_read_senvcfg_fn(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    uintptr_t v = senvcfg_read();
    (void)v;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode trampoline: write senvcfg.
 * =================================================================== */
static uintptr_t vs_write_senvcfg_fn(uintptr_t val) {
    trap_expect_begin();
    senvcfg_write(val);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VU-mode trampoline: indirect jump to target.
 * =================================================================== */
static uintptr_t vu_jalr_to_target(uintptr_t addr) {
    trap_expect_begin();
    asm volatile(
        "jalr ra, %0, 0\n\t"
        :
        : "r"(addr)
        : "ra", "memory"
    );
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode trampoline: C.JALR (compressed) to target.
 * =================================================================== */
static uintptr_t vs_cjalr_to_target(uintptr_t addr) {
    trap_expect_begin();
    register uintptr_t a0 asm("a0") = addr;
    asm volatile(
        ".option push\n\t"
        ".option arch,+c\n\t"
        "c.jalr %0\n\t"
        ".option pop\n\t"
        :
        : "r"(a0)
        : "ra", "memory"
    );
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode trampoline: C.JR (compressed) to target.
 * =================================================================== */
static uintptr_t vs_cjr_to_target(uintptr_t addr) {
    trap_expect_begin();
    register uintptr_t a0 asm("a0") = addr;
    /* c.jr does not link; load ra with the continuation address so the
     * target's RET returns here instead of looping into this function. */
    asm volatile(
        ".option push\n\t"
        ".option arch,+c\n\t"
        "la ra, 1f\n\t"
        "c.jr %0\n\t"
        "1:\n\t"
        ".option pop\n\t"
        :
        : "r"(a0)
        : "ra", "memory"
    );
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * Helper: set up code in the CFI test region and flush I-cache.
 *
 * Emits instructions at __cfi_test_code_start and fences the I-cache
 * so VS-mode can execute them.
 * =================================================================== */
static uint32_t *get_cfi_code_buf(void) {
    return (uint32_t *)__cfi_test_code_start;
}

static void flush_cfi_code(void) {
    asm volatile("fence.i" ::: "memory");
}

/* ===================================================================
 * Helper: configure CFI environment for VS-mode tests.
 *
 * Sets menvcfg.LPE, mseccfg.MLPE, and henvcfg.LPE as needed.
 * Returns original henvcfg value for restoration.
 * =================================================================== */
static uintptr_t cfi_setup_vs_lpe(bool henvcfg_lpe, bool menvcfg_lpe) {
    uintptr_t orig_henvcfg = henvcfg_read();
    uintptr_t orig_menvcfg = menvcfg_read();

    if (menvcfg_lpe) {
        menvcfg_set(MENVCFG_LPE);
    } else {
        menvcfg_clear(MENVCFG_LPE);
    }

    if (henvcfg_lpe) {
        henvcfg_write(orig_henvcfg | HENVCFG_LPE);
    } else {
        henvcfg_write(orig_henvcfg & ~HENVCFG_LPE);
    }

    return orig_henvcfg;
}

static void cfi_restore_henvcfg(uintptr_t orig) {
    henvcfg_write(orig);
}

/* ===================================================================
 * VS-mode exception handler for delegation tests.
 *
 * When traps are delegated to VS-mode (hedeleg), vstvec must point
 * to a valid handler. This naked handler records vscause, advances
 * sepc, and returns via SRET.
 * =================================================================== */
static volatile uintptr_t g_vs_exc_cause;
static volatile uintptr_t g_vs_exc_cause2;
static volatile uintptr_t g_vs_exc_tval;
static volatile uintptr_t g_vs_exc_epc;
static volatile uintptr_t g_vs_exc_spelp;
static volatile uintptr_t g_vs_exc_recovery; /* 0 = advance vsepc (default) */
static volatile unsigned  g_vs_exc_count;
static volatile bool g_vs_exc_triggered;

static void vs_exc_handler(void) __attribute__((naked, aligned(4)));
static void vs_exc_handler(void)
{
    asm volatile (
        "addi   sp, sp, -32\n\t"
        "sd     ra, 0(sp)\n\t"
        "sd     t0, 8(sp)\n\t"
        "sd     t1, 16(sp)\n\t"
        "sd     t2, 24(sp)\n\t"

        /* Record vscause */
        "csrr   t0, scause\n\t"
        "la     t2, g_vs_exc_cause\n\t"
        "sd     t0, 0(t2)\n\t"
        /* Record vstval */
        "csrr   t0, stval\n\t"
        "la     t2, g_vs_exc_tval\n\t"
        "sd     t0, 0(t2)\n\t"
        /* Record vsepc */
        "csrr   t0, sepc\n\t"
        "la     t2, g_vs_exc_epc\n\t"
        "sd     t0, 0(t2)\n\t"
        "li     t0, 1\n\t"
        "la     t2, g_vs_exc_triggered\n\t"
        "sb     t0, 0(t2)\n\t"

        /* Record vsstatus.SPELP (bit 23) captured at trap entry */
        "csrr   t0, sstatus\n\t"
        "srli   t0, t0, 23\n\t"
        "andi   t0, t0, 1\n\t"
        "la     t2, g_vs_exc_spelp\n\t"
        "sd     t0, 0(t2)\n\t"

        /* Recovery PC override: if g_vs_exc_recovery != 0, resume there
         * instead of advancing past the faulting instruction (used when
         * the faulting PC sits in a U-only page that VS-mode cannot
         * execute from). */
        "la     t2, g_vs_exc_recovery\n\t"
        "ld     t0, 0(t2)\n\t"
        "beqz   t0, 1f\n\t"
        "csrw   sepc, t0\n\t"
        "j      3f\n\t"
        "1:\n\t"
        /* Determine faulting instruction length and advance sepc:
         * - bits[1:0] == 0b11: 32-bit instruction, advance by 4
         * - otherwise: 16-bit compressed, advance by 2 */
        "csrr   t0, sepc\n\t"
        "lhu    t1, 0(t0)\n\t"      /* Read first 2 bytes of faulting insn */
        "andi   t2, t1, 0x3\n\t"    /* Check bits[1:0] */
        "xori   t2, t2, 0x3\n\t"    /* t2=0 if 32-bit, !=0 if 16-bit */
        "bnez   t2, 2f\n\t"         /* If compressed, jump to 2 */
        "addi   t0, t0, 4\n\t"      /* 32-bit: advance by 4 */
        "j      3f\n\t"
        "2:\n\t"
        "addi   t0, t0, 2\n\t"      /* 16-bit: advance by 2 */
        "3:\n\t"
        "csrw   sepc, t0\n\t"

        /* Clear sstatus.SPELP (bit 23) before sret. Per
         * norm:Zicfilp_pelp_trap_return, sret restores ELP from SPELP
         * when VSLPE=1; without clearing it, returning to non-LPAD
         * code would immediately re-fault with a landing pad fault
         * and the handler would march PC forward indefinitely. */
        "li     t0, 0x800000\n\t"
        "csrc   sstatus, t0\n\t"

        /* Force SPP=1 (S-mode) so sret returns to VS-mode */
        "li     t0, 0x100\n\t"
        "csrs   sstatus, t0\n\t"

        /* Restore registers */
        "ld     ra, 0(sp)\n\t"
        "ld     t0, 8(sp)\n\t"
        "ld     t1, 16(sp)\n\t"
        "ld     t2, 24(sp)\n\t"
        "addi   sp, sp, 32\n\t"

        "sret\n\t"
    );
}

/* ===================================================================
 * VS-mode exception handler variant: two-entry ELP restore check.
 *
 * First entry: record vscause and the SPELP value captured at trap
 * entry, advance vsepc, and keep SPELP intact so the sret restores
 * ELP=LP_EXPECTED (norm:Zicfilp_pelp_trap_return). The restored ELP
 * must re-fault on the next non-LPAD instruction.
 * Second (and later) entry: record vscause into g_vs_exc_cause2 and
 * clear SPELP to break the re-fault loop.
 * Used by HCFI-LP-13.
 * =================================================================== */
static void vs_exc_handler_noclear(void) __attribute__((naked, aligned(4)));
static void vs_exc_handler_noclear(void)
{
    asm volatile (
        "addi   sp, sp, -32\n\t"
        "sd     ra, 0(sp)\n\t"
        "sd     t0, 8(sp)\n\t"
        "sd     t1, 16(sp)\n\t"
        "sd     t2, 24(sp)\n\t"

        /* count++ (t1 = previous count) */
        "la     t2, g_vs_exc_count\n\t"
        "lwu    t1, 0(t2)\n\t"
        "addi   t0, t1, 1\n\t"
        "sw     t0, 0(t2)\n\t"

        "li     t0, 1\n\t"
        "la     t2, g_vs_exc_triggered\n\t"
        "sb     t0, 0(t2)\n\t"

        "bnez   t1, 1f\n\t"
        /* First entry: record cause and SPELP, keep SPELP for sret */
        "csrr   t0, scause\n\t"
        "la     t2, g_vs_exc_cause\n\t"
        "sd     t0, 0(t2)\n\t"
        "csrr   t0, sstatus\n\t"
        "srli   t0, t0, 23\n\t"
        "andi   t0, t0, 1\n\t"
        "la     t2, g_vs_exc_spelp\n\t"
        "sd     t0, 0(t2)\n\t"
        "j      2f\n\t"
        "1:\n\t"
        /* Later entries: record cause2, clear SPELP to stop re-faulting */
        "csrr   t0, scause\n\t"
        "la     t2, g_vs_exc_cause2\n\t"
        "sd     t0, 0(t2)\n\t"
        "li     t0, 0x800000\n\t"
        "csrc   sstatus, t0\n\t"
        "2:\n\t"

        /* Determine faulting instruction length and advance sepc */
        "csrr   t0, sepc\n\t"
        "lhu    t1, 0(t0)\n\t"
        "andi   t2, t1, 0x3\n\t"
        "xori   t2, t2, 0x3\n\t"
        "bnez   t2, 3f\n\t"
        "addi   t0, t0, 4\n\t"
        "j      4f\n\t"
        "3:\n\t"
        "addi   t0, t0, 2\n\t"
        "4:\n\t"
        "csrw   sepc, t0\n\t"

        /* Force SPP=1 (S-mode) so sret returns to VS-mode */
        "li     t0, 0x100\n\t"
        "csrs   sstatus, t0\n\t"

        /* Restore registers */
        "ld     ra, 0(sp)\n\t"
        "ld     t0, 8(sp)\n\t"
        "ld     t1, 16(sp)\n\t"
        "ld     t2, 24(sp)\n\t"
        "addi   sp, sp, 32\n\t"

        "sret\n\t"
    );
}

/* ===================================================================
 * VS-mode exception handler variant: force SPELP=1 before sret.
 *
 * Records vscause, advances vsepc, then SETS sstatus.SPELP=1 before
 * sret. Hardware must clear SPELP on sret (norm:Zicfilp_pelp_trap_return
 * sets xpelp to NO_LP_EXPECTED unconditionally), which the test
 * verifies by reading vsstatus.SPELP afterwards. Used by HCFI-LP-14.
 * =================================================================== */
static void vs_exc_handler_set_spelp(void) __attribute__((naked, aligned(4)));
static void vs_exc_handler_set_spelp(void)
{
    asm volatile (
        "addi   sp, sp, -32\n\t"
        "sd     ra, 0(sp)\n\t"
        "sd     t0, 8(sp)\n\t"
        "sd     t1, 16(sp)\n\t"
        "sd     t2, 24(sp)\n\t"

        /* Record vscause */
        "csrr   t0, scause\n\t"
        "la     t2, g_vs_exc_cause\n\t"
        "sd     t0, 0(t2)\n\t"
        "li     t0, 1\n\t"
        "la     t2, g_vs_exc_triggered\n\t"
        "sb     t0, 0(t2)\n\t"

        /* Determine faulting instruction length and advance sepc */
        "csrr   t0, sepc\n\t"
        "lhu    t1, 0(t0)\n\t"
        "andi   t2, t1, 0x3\n\t"
        "xori   t2, t2, 0x3\n\t"
        "bnez   t2, 1f\n\t"
        "addi   t0, t0, 4\n\t"
        "j      2f\n\t"
        "1:\n\t"
        "addi   t0, t0, 2\n\t"
        "2:\n\t"
        "csrw   sepc, t0\n\t"

        /* Set sstatus.SPELP=1 before sret */
        "li     t0, 0x800000\n\t"
        "csrs   sstatus, t0\n\t"

        /* Force SPP=1 (S-mode) so sret returns to VS-mode */
        "li     t0, 0x100\n\t"
        "csrs   sstatus, t0\n\t"

        /* Restore registers */
        "ld     ra, 0(sp)\n\t"
        "ld     t0, 8(sp)\n\t"
        "ld     t1, 16(sp)\n\t"
        "ld     t2, 24(sp)\n\t"
        "addi   sp, sp, 32\n\t"

        "sret\n\t"
    );
}

/* ===================================================================
 * Delegation helpers
 * =================================================================== */

/* Configure dual-layer delegation: medeleg + hedeleg for VS delivery,
 * with an explicit VS-mode trap handler. */
static void setup_deleg_to_vs_ex(uintptr_t exc_mask, uintptr_t handler) {
    /* Set medeleg to delegate to HS-mode */
    CSRS(medeleg, exc_mask);
    /* Set hedeleg to further delegate to VS-mode */
    CSRS(CSR_HEDELEG, exc_mask);
    /* Install VS-mode trap handler (Direct mode) */
    g_vs_exc_cause = 0;
    g_vs_exc_cause2 = 0;
    g_vs_exc_tval = 0;
    g_vs_exc_epc = 0;
    g_vs_exc_spelp = 0;
    g_vs_exc_recovery = 0;
    g_vs_exc_count = 0;
    g_vs_exc_triggered = false;
    vs_trap_setup_direct(handler);
}

/* Configure dual-layer delegation: medeleg + hedeleg for VS delivery. */
static void setup_deleg_to_vs(uintptr_t exc_mask) {
    setup_deleg_to_vs_ex(exc_mask, (uintptr_t)vs_exc_handler);
}

/* Configure single-layer delegation: medeleg only for HS delivery. */
static void setup_deleg_to_hs(uintptr_t exc_mask) {
    CSRS(medeleg, exc_mask);
    CSRC(CSR_HEDELEG, exc_mask);
}

/* Clear all delegation. */
static void clear_all_deleg(void) {
    CSRW(medeleg, 0);
    CSRW(CSR_HEDELEG, 0);
}

/* ===================================================================
 * PTE modification helper
 * =================================================================== */

static void vs_pte_modify(two_stage_ctx_t *ctx, uintptr_t va, int level,
                          uintptr_t new_flags) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, level);
    if (pte) {
        *pte = (*pte & ~(PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)) | new_flags;
    }
}

#endif /* HYPERVISOR_ZICFILP_TEST_HELPERS_H */
