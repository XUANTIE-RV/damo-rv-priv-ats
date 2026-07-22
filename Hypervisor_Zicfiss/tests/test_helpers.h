/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Hypervisor x Zicfiss cross tests
 *
 * Provides H/Zicfiss extension detection, CSR access helpers,
 * shadow stack instruction trampolines, SS page type management,
 * and two-stage page table setup utilities.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 */

#ifndef HYPERVISOR_ZICFISS_TEST_HELPERS_H
#define HYPERVISOR_ZICFISS_TEST_HELPERS_H

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
extern uint8_t ss_page[];
extern uint8_t rw_page[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];
extern char  __shadow_stack_start[];
extern char  __shadow_stack_end[];

#define TEST_REGION_BASE   ((uintptr_t)__vm_test_region_start)
#define SS_PAGE_ADDR       ((uintptr_t)ss_page)
#define RW_PAGE_ADDR       ((uintptr_t)rw_page)

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
 * ssp CSR access (CSR 0x011)
 * =================================================================== */
static inline uintptr_t ssp_read(void) {
    uintptr_t v;
    asm volatile("csrr %0, 0x011" : "=r"(v));
    return v;
}

static inline void ssp_write(uintptr_t val) {
    asm volatile("csrw 0x011, %0" :: "r"(val) : "memory");
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
 * Zicfiss detection (via henvcfg.SSE writability)
 * =================================================================== */
static bool zicfiss_detected = false;
static bool zicfiss_detection_done = false;

static bool detect_zicfiss(void) {
    if (zicfiss_detection_done)
        return zicfiss_detected;

    /* Try setting henvcfg.SSE */
    uintptr_t orig = henvcfg_read();
    henvcfg_write(orig | HENVCFG_SSE);
    uintptr_t val = henvcfg_read();
    if (val & HENVCFG_SSE) {
        henvcfg_write(orig);
        zicfiss_detected = true;
        zicfiss_detection_done = true;
        return true;
    }
    henvcfg_write(orig);

    /* Also try menvcfg.SSE as fallback */
    orig = menvcfg_read();
    menvcfg_set(MENVCFG_SSE);
    val = menvcfg_read();
    if (val & MENVCFG_SSE) {
        menvcfg_clear(MENVCFG_SSE);
        zicfiss_detected = true;
        zicfiss_detection_done = true;
        return true;
    }

    zicfiss_detected = false;
    zicfiss_detection_done = true;
    return false;
}

#define ZICFISS_REQUIRED_OR_SKIP() do { \
    if (!detect_zicfiss()) { \
        TEST_SKIP("Zicfiss not implemented"); \
    } \
} while (0)

/* ===================================================================
 * Shadow Stack instruction encodings (Zicfiss)
 *
 * SSPUSH x1:     0xCE104073
 * SSPOPCHK x1:   0xCDC0C073
 * SSAMOSWAP.W:   rs1=src, rd=dest, encoding: 0x18xxxx33 with funct7=1101000
 *   ssamoswap.w rd, rs2, (rs1)  =>  funct7=1101000, rs2, rs1, funct3=010, rd, opcode=0101111
 *   With rd=x0, rs2=x1, rs1=x10: 0xCD5A52F (approximate, see spec)
 * SSAMOSWAP.D:   same but funct3=011
 *
 * For testing, we use .word to emit these instructions.
 * =================================================================== */
#define INSN_SSPUSH_X1     0xCE104073
#define INSN_SSPOPCHK_X1   0xCDC0C073

/* SSAMOSWAP.W/D test values loaded into x1 (rs2) so tests can verify
 * the swapped-in data on the memory side. */
#define SSAMOSWAP_W_VAL   0x1234
#define SSAMOSWAP_D_VAL   0x12345678

/* ===================================================================
 * PTE Shadow Stack page encoding
 *
 * Per norm:ss_page_enc: R=0, W=1, X=0 represents an SS page.
 * In the framework's PTE flags: PTE_W without PTE_R or PTE_X.
 * =================================================================== */
#define PTE_SS_PAGE_FLAGS  (PTE_V | PTE_W | PTE_A | PTE_D)

/* ===================================================================
 * Helper: configure CFI environment for VS-mode Zicfiss tests.
 * =================================================================== */
static uintptr_t cfi_setup_vs_sse(bool henvcfg_sse, bool menvcfg_sse) {
    uintptr_t orig_henvcfg = henvcfg_read();

    if (menvcfg_sse) {
        menvcfg_set(MENVCFG_SSE);
    } else {
        menvcfg_clear(MENVCFG_SSE);
    }

    if (henvcfg_sse) {
        henvcfg_write(orig_henvcfg | HENVCFG_SSE);
    } else {
        henvcfg_write(orig_henvcfg & ~HENVCFG_SSE);
    }

    return orig_henvcfg;
}

static void cfi_restore_henvcfg(uintptr_t orig) {
    henvcfg_write(orig);
}

/* ===================================================================
 * VS-mode trampolines for ssp CSR access
 * =================================================================== */

/* VS-mode: read ssp CSR */
static uintptr_t vs_read_ssp(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    uintptr_t v = ssp_read();
    (void)v;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: write ssp CSR */
static uintptr_t vs_write_ssp(uintptr_t val) {
    trap_expect_begin();
    ssp_write(val);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* Value read from senvcfg inside VS-mode, for test assertions. */
static volatile uintptr_t g_vs_senvcfg_val;

/* VS-mode: read senvcfg */
static uintptr_t vs_read_senvcfg_fn(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    uintptr_t v = senvcfg_read();
    g_vs_senvcfg_val = v;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: write senvcfg */
static uintptr_t vs_write_senvcfg_fn(uintptr_t val) {
    trap_expect_begin();
    senvcfg_write(val);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode trampolines for shadow stack instructions
 * =================================================================== */

/* VS-mode: execute SSPUSH x1 */
static uintptr_t vs_exec_sspush(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    asm volatile(".word 0xCE104073" ::: "memory");
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* Address of the SSPOPCHK instruction inside vs_exec_sspopchk, recorded
 * at runtime so tests can compare it against vsepc after a trap. */
static volatile uintptr_t g_sspopchk_addr;

/* VS-mode: execute SSPOPCHK x1 */
static uintptr_t vs_exec_sspopchk(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    asm volatile(
        "la    t0, 1f\n\t"
        "la    t1, g_sspopchk_addr\n\t"
        "sd    t0, 0(t1)\n\t"
        "1:\n\t"
        ".word 0xCDC0C073\n\t"
        :
        :
        : "t0", "t1", "memory");
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: execute SSAMOSWAP.W (a0 = address, x1 = value)
 * Encoding: ssamoswap.w x0, x1, (a0)
 *   funct7=0100100 rs2=00001 rs1=01010 funct3=010 rd=00000 opcode=0101111
 *   = 0x4815202F
 * x1 is loaded with SSAMOSWAP_W_VAL so tests can verify the swapped-in
 * data on the memory side; ra is saved/restored via t3.
 *
 * Recovery address: QEMU (cskysim) has a translation-block bug that
 * reports mepc as the TB start instead of the SSAMOSWAP address when
 * a virtual-instruction trap is taken.  Without _exec_return_addr the
 * M-mode handler would advance the wrong mepc, mret to a stale address
 * and re-trigger the trap (armed=false -> UNEXPECTED TRAP crash).
 * Setting _exec_return_addr lets the handler resume at label 1f
 * regardless of the bogus mepc. */
extern uintptr_t _exec_return_addr;

static uintptr_t vs_exec_ssamoswap_w(uintptr_t addr) {
    trap_expect_begin();
    register uintptr_t a0 asm("a0") = addr;
    asm volatile(
        "la   t1, _exec_return_addr\n\t"
        "la   t0, 1f\n\t"
        "sd   t0, 0(t1)\n\t"
        "mv   t3, ra\n\t"
        "li   ra, %0\n\t"
        ".word 0x4815202F\n\t"   /* ssamoswap.w x0, x1, (a0) */
        "1:\n\t"
        "mv   ra, t3\n\t"
        "sd   zero, 0(t1)\n\t"
        :
        : "i"(SSAMOSWAP_W_VAL), "r"(a0)
        : "t0", "t1", "t3", "ra", "memory"
    );
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: execute SSAMOSWAP.D (a0 = address, x1 = value)
 * Encoding: ssamoswap.d x0, x1, (a0)
 *   funct7=0100100 rs2=00001 rs1=01010 funct3=011 rd=00000 opcode=0101111
 *   = 0x4815302F
 * See vs_exec_ssamoswap_w comment for _exec_return_addr rationale. */
static uintptr_t vs_exec_ssamoswap_d(uintptr_t addr) {
    trap_expect_begin();
    register uintptr_t a0 asm("a0") = addr;
    asm volatile(
        "la   t1, _exec_return_addr\n\t"
        "la   t0, 1f\n\t"
        "sd   t0, 0(t1)\n\t"
        "mv   t3, ra\n\t"
        "li   ra, %0\n\t"
        ".word 0x4815302F\n\t"   /* ssamoswap.d x0, x1, (a0) */
        "1:\n\t"
        "mv   ra, t3\n\t"
        "sd   zero, 0(t1)\n\t"
        :
        : "i"(SSAMOSWAP_D_VAL), "r"(a0)
        : "t0", "t1", "t3", "ra", "memory"
    );
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode trampolines for memory access
 * =================================================================== */

/* VS-mode: store to address */
static uintptr_t vs_store(uintptr_t arg) {
    trap_expect_begin();
    *(volatile uintptr_t *)arg = 0xDEADBEEF;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: load from address */
static uintptr_t vs_load(uintptr_t arg) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: simple NOP function */
static uintptr_t vs_nop_fn(uintptr_t arg) {
    (void)arg;
    return 0;
}

/* ===================================================================
 * VU-mode trampolines
 * =================================================================== */

/* VU-mode: read ssp CSR */
static uintptr_t vu_read_ssp(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    uintptr_t v = ssp_read();
    (void)v;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: write ssp CSR */
static uintptr_t vu_write_ssp(uintptr_t val) {
    trap_expect_begin();
    ssp_write(val);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: execute SSPUSH */
static uintptr_t vu_exec_sspush(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    asm volatile(".word 0xCE104073" ::: "memory");
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: execute SSPOPCHK */
static uintptr_t vu_exec_sspopchk(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    asm volatile(".word 0xCDC0C073" ::: "memory");
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: store to address */
static uintptr_t vu_store(uintptr_t arg) {
    trap_expect_begin();
    *(volatile uintptr_t *)arg = 0xDEADBEEF;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: load from address */
static uintptr_t vu_load(uintptr_t arg) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode exception handler for delegation tests.
 *
 * When traps are delegated to VS-mode (hedeleg), vstvec must point
 * to a valid handler. This naked handler records vscause, advances
 * sepc, and returns via SRET.
 * =================================================================== */
static volatile uintptr_t g_vs_exc_cause;
static volatile uintptr_t g_vs_exc_tval;
static volatile uintptr_t g_vs_exc_epc;
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

static void setup_deleg_to_vs(uintptr_t exc_mask) {
    CSRS(medeleg, exc_mask);
    CSRS(CSR_HEDELEG, exc_mask);
    /* Install VS-mode trap handler (Direct mode) */
    g_vs_exc_cause = 0;
    g_vs_exc_tval = 0;
    g_vs_exc_epc = 0;
    g_vs_exc_triggered = false;
    vs_trap_setup_direct((uintptr_t)vs_exc_handler);
}

static void setup_deleg_to_hs(uintptr_t exc_mask) {
    CSRS(medeleg, exc_mask);
    CSRC(CSR_HEDELEG, exc_mask);
}

static void clear_all_deleg(void) {
    CSRW(medeleg, 0);
    CSRW(CSR_HEDELEG, 0);
}

/* ===================================================================
 * PTE inspection / modification helpers
 * =================================================================== */

static uintptr_t vs_pte_read(two_stage_ctx_t *ctx, uintptr_t va, int level) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, level);
    return pte ? *pte : 0;
}

static uintptr_t g_pte_read(two_stage_ctx_t *ctx, uintptr_t gpa, int level) {
    uintptr_t *pte = gpt_get_pte(&ctx->g_ctx, gpa, level);
    return pte ? *pte : 0;
}

static void vs_pte_modify(two_stage_ctx_t *ctx, uintptr_t va, int level,
                          uintptr_t new_flags) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, level);
    if (pte) {
        *pte = (*pte & ~(PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)) | new_flags;
    }
}

static void g_pte_modify(two_stage_ctx_t *ctx, uintptr_t gpa, int level,
                         uintptr_t new_flags) {
    uintptr_t *pte = gpt_get_pte(&ctx->g_ctx, gpa, level);
    if (pte) {
        *pte = (*pte & ~(PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)) | new_flags;
    }
}

#endif /* HYPERVISOR_ZICFISS_TEST_HELPERS_H */
