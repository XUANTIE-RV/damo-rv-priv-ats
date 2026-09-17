/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zalasr suite.
 *
 * All test files are #included into test_register.c, so the static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Spec: SPEC/riscv-isa-manual/src/unpriv/zalasr.adoc plus the Hypervisor
 *       bindings. Zalasr provides two families of STANDALONE atomic
 *       ordered memory instructions in the AMO major opcode (0x2f):
 *
 *         load-acquire   lb/lh/lw/ld.{aq,aqrl}   funct5 = 00110
 *           norm:ldaq_atomic_load_op - pure read: loads 2^width bytes at
 *             rs1 into rd, NEVER writes memory, so it only needs READ
 *             permission and never sets a data-page D bit.
 *         store-release  sb/sh/sw/sd.{rl,aqrl}   funct5 = 00111
 *           norm:sdrl_atomic_store_op - pure write: stores the low
 *             2^width bits of rs2 to rs1, ALWAYS writes, so it needs
 *             WRITE permission and its D-bit requirement is MANDATORY.
 *
 *       Key Hypervisor cross points:
 *         - Exception-class SPLIT: load-acquire -> load class (cause
 *           4/5/13/21), store-release -> store/AMO class (cause 6/7/15/23).
 *           zalasr.adoc does NOT pin the load-acquire class (it lives in
 *           the AMO opcode space), so the load-acquire cause is treated
 *           as RECORD-AND-COMPARE (record the observation, compare with
 *           the functional load-class expectation, flag a deviation for
 *           bugs/); the store-release cause is UNAMBIGUOUS -> hard assert.
 *         - htinst goes through the transformedatomicinst format (opcode
 *           0x2F), NOT transformedload/storeinst, retaining funct5, aq/rl,
 *           funct3, rd/rs2 (only bits19:15 <- Addr.Offset, always 0 here).
 *         - Reserved encodings (load without aq, store without rl) are not
 *           HS-qualified -> illegal-instruction (cause=2), NOT
 *           virtual-instruction (cause=22).
 *
 * ENCODING INJECTION
 * ------------------
 * The build toolchain has no Zalasr mnemonics and rejects "zalasr" as an
 * -march extension, so every instruction is emitted with the ".insn r"
 * pseudo-op. The AMO R-type layout is:
 *
 *   31    27 26  25 24   20 19   15 14  12 11    7 6      0
 *  +--------+--+--+-------+-------+------+-------+--------+
 *  | funct5 |aq|rl|  rs2  |  rs1  |funct3|  rd   | opcode |
 *  +--------+--+--+-------+-------+------+-------+--------+
 *
 * ".insn r opcode, funct3, funct7, rd, rs1, rs2" takes
 * funct7 = bits31:25 = (funct5 << 2) | (aq << 1) | rl. A load-acquire
 * fixes rs2 = x0; a store-release fixes rd = x0. .option norvc keeps
 * every instruction 4 bytes wide so the trap handler skips a faulting
 * or reserved encoding with sepc += 4.
 *
 * Two-stage infrastructure is reused from common/hyp/two_stage_helpers.h;
 * htinst golden values and the implicit-walk victim builder come from
 * common/hyp/hyp_test_helpers.h. hyp_transform_mem_inst() handles the
 * opcode 0x2F atomic branch (all fields kept except bits19:15), which
 * covers funct5=00110/00111; HZLASR-18~21 verify that empirically.
 */

#ifndef HYPERVISOR_ZALASR_TEST_HELPERS_H
#define HYPERVISOR_ZALASR_TEST_HELPERS_H

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
 * Zalasr instruction field constants (string literals for .insn r).
 * =================================================================== */

/* funct3 width encodings. */
#define ZALASR_F3_B   "0"   /* byte     - lb/sb */
#define ZALASR_F3_H   "1"   /* halfword - lh/sh */
#define ZALASR_F3_W   "2"   /* word     - lw/sw */
#define ZALASR_F3_D   "3"   /* double   - ld/sd (RV64 only) */

/* funct7 = (funct5 << 2) | (aq << 1) | rl.
 *
 * load-acquire  (funct5 = 00110 -> funct5<<2 = 0x18):
 *   .aq    aq=1 rl=0 -> 0x1a      .aqrl  aq=1 rl=1 -> 0x1b
 * store-release (funct5 = 00111 -> funct5<<2 = 0x1c):
 *   .rl    aq=0 rl=1 -> 0x1d      .aqrl  aq=1 rl=1 -> 0x1f
 * RESERVED load  (aq=0):  rl=0 -> 0x18   load-release  rl=1 -> 0x19
 * RESERVED store (rl=0):  aq=0 -> 0x1c   store-acquire aq=1 -> 0x1e
 */
#define ZALASR_F7_LD_AQ     "0x1a"
#define ZALASR_F7_LD_AQRL   "0x1b"
#define ZALASR_F7_ST_RL     "0x1d"
#define ZALASR_F7_ST_AQRL   "0x1f"
#define ZALASR_F7_LD_NOAQ   "0x18"  /* RESERVED: load without aq, rl=0    */
#define ZALASR_F7_LD_REL    "0x19"  /* RESERVED: load-release, aq=0 rl=1  */
#define ZALASR_F7_ST_NORL   "0x1c"  /* RESERVED: store without rl, aq=0   */
#define ZALASR_F7_ST_ACQ    "0x1e"  /* RESERVED: store-acquire, rl=0 aq=1 */

/* Integer funct5 field values (for htinst field checks). */
#define ZALASR_F5_LOAD      0x06u   /* 00110 - load-acquire  */
#define ZALASR_F5_STORE     0x07u   /* 00111 - store-release */

/* ===================================================================
 * Execution primitives (raw ".insn r" injection)
 *
 * f3 and f7 must be string literals (ZALASR_F3_* / ZALASR_F7_*) so the
 * assembler template concatenates. A load-acquire writes rd and fixes
 * rs2 = x0; a store-release fixes rd = x0 and reads rs2.
 * =================================================================== */
#define ZALASR_LOAD(f3, f7, rd, addr) \
    asm volatile( \
        ".option push\n\t" \
        ".option norvc\n\t" \
        ".insn r 0x2f, " f3 ", " f7 ", %0, %1, x0\n\t" \
        ".option pop\n\t" \
        : "=r"(rd) : "r"(addr) : "memory")

#define ZALASR_STORE(f3, f7, addr, val) \
    asm volatile( \
        ".option push\n\t" \
        ".option norvc\n\t" \
        ".insn r 0x2f, " f3 ", " f7 ", x0, %0, %1\n\t" \
        ".option pop\n\t" \
        :: "r"(addr), "r"(val) : "memory")

/* rd sign-extension expectations (norm:zalasr_signext_rd). */
#define ZALASR_B_SIGNEXT(v) ((uintptr_t)(intptr_t)(int8_t)(v))
#define ZALASR_H_SIGNEXT(v) ((uintptr_t)(intptr_t)(int16_t)(v))
#define ZALASR_W_SIGNEXT(v) ((uintptr_t)(intptr_t)(int32_t)(v))

/* ===================================================================
 * Byte-wise little-endian accessors (preset / read back sub-word and
 * misaligned targets without a wider aligned access that would trap).
 * =================================================================== */
static inline uint8_t hzlasr_load8(uintptr_t addr)
{
    return ((volatile uint8_t *)addr)[0];
}
static inline void hzlasr_store8(uintptr_t addr, uint8_t val)
{
    ((volatile uint8_t *)addr)[0] = val;
}
static inline uint16_t hzlasr_load16(uintptr_t addr)
{
    volatile uint8_t *p = (volatile uint8_t *)addr;
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static inline void hzlasr_store16(uintptr_t addr, uint16_t val)
{
    volatile uint8_t *p = (volatile uint8_t *)addr;
    p[0] = (uint8_t)(val);
    p[1] = (uint8_t)(val >> 8);
}
static inline uint32_t hzlasr_load_le32(uintptr_t addr)
{
    volatile uint8_t *p = (volatile uint8_t *)addr;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline void hzlasr_store_le32(uintptr_t addr, uint32_t val)
{
    volatile uint8_t *p = (volatile uint8_t *)addr;
    p[0] = (uint8_t)(val);
    p[1] = (uint8_t)(val >> 8);
    p[2] = (uint8_t)(val >> 16);
    p[3] = (uint8_t)(val >> 24);
}
#if __riscv_xlen == 64
static inline uint64_t hzlasr_load_le64(uintptr_t addr)
{
    return (uint64_t)hzlasr_load_le32(addr) |
           ((uint64_t)hzlasr_load_le32(addr + 4) << 32);
}
static inline void hzlasr_store_le64(uintptr_t addr, uint64_t val)
{
    hzlasr_store_le32(addr, (uint32_t)val);
    hzlasr_store_le32(addr + 4, (uint32_t)(val >> 32));
}
#endif

/* ===================================================================
 * Zalasr availability.
 *
 * Zalasr support is config-DECLARATION driven: every case gates with
 *   if (!H_AVAILABLE) TEST_SKIP("H extension not available");
 *   if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");
 * using the compile-time ZALASR_AVAILABLE macro normalized in
 * common/capabilities.h from ZALASR_SUPPORTED. Zalasr may be
 * implemented independently of Zaamo/Zalrsc/Zabha
 * (norm:zalasr_builds_on_amo), so the gate does NOT depend on any
 * A-extension macro (unlike Groups 2/3/4). No wrapper macro is
 * provided. Only the first case (HZLASR-01) additionally probes the
 * DUT (trap-armed load-acquire in M-mode) to verify alignment with the
 * config declaration; no other case probes.
 * =================================================================== */

#define HZLASR_SMP_SKIP_REASON \
    "multi-hart: common/entry.S parks every hart except hart 0, so no " \
    "secondary-hart bring-up or inter-hart synchronization exists"

/* ===================================================================
 * VS/VU-mode Zalasr probes. Each is a uintptr_t(*)(uintptr_t) so it can
 * be passed to two_stage_run_in_vs / _vu and run_in_priv. Load-acquire
 * probes return the (sign-extended) loaded value; store-release probes
 * store the global hz_st_val and return 0.
 * =================================================================== */
static volatile uintptr_t hz_st_val;

/* --- load-acquire (funct5=00110, aq=1) --- */
static inline uintptr_t hz_vs_lb_aq(uintptr_t addr)
{
    uintptr_t r; ZALASR_LOAD(ZALASR_F3_B, ZALASR_F7_LD_AQ, r, addr); return r;
}
static inline uintptr_t hz_vs_lh_aq(uintptr_t addr)
{
    uintptr_t r; ZALASR_LOAD(ZALASR_F3_H, ZALASR_F7_LD_AQ, r, addr); return r;
}
static inline uintptr_t hz_vs_lw_aq(uintptr_t addr)
{
    uintptr_t r; ZALASR_LOAD(ZALASR_F3_W, ZALASR_F7_LD_AQ, r, addr); return r;
}
static inline uintptr_t hz_vs_lw_aqrl(uintptr_t addr)
{
    uintptr_t r; ZALASR_LOAD(ZALASR_F3_W, ZALASR_F7_LD_AQRL, r, addr); return r;
}
#if __riscv_xlen == 64
static inline uintptr_t hz_vs_ld_aq(uintptr_t addr)
{
    uintptr_t r; ZALASR_LOAD(ZALASR_F3_D, ZALASR_F7_LD_AQ, r, addr); return r;
}
#endif

/* --- store-release (funct5=00111, rl=1) --- */
static inline uintptr_t hz_vs_sb_rl(uintptr_t addr)
{
    ZALASR_STORE(ZALASR_F3_B, ZALASR_F7_ST_RL, addr, hz_st_val); return 0;
}
static inline uintptr_t hz_vs_sh_rl(uintptr_t addr)
{
    ZALASR_STORE(ZALASR_F3_H, ZALASR_F7_ST_RL, addr, hz_st_val); return 0;
}
static inline uintptr_t hz_vs_sw_rl(uintptr_t addr)
{
    ZALASR_STORE(ZALASR_F3_W, ZALASR_F7_ST_RL, addr, hz_st_val); return 0;
}
static inline uintptr_t hz_vs_sw_aqrl(uintptr_t addr)
{
    ZALASR_STORE(ZALASR_F3_W, ZALASR_F7_ST_AQRL, addr, hz_st_val); return 0;
}
#if __riscv_xlen == 64
static inline uintptr_t hz_vs_sd_rl(uintptr_t addr)
{
    ZALASR_STORE(ZALASR_F3_D, ZALASR_F7_ST_RL, addr, hz_st_val); return 0;
}
#endif

/* Execute the sampled load-acquire set (all widths) then store-release
 * set (all widths). Used by the normal-execution / cause=22 cases. */
static inline uintptr_t hz_all_load_acq(uintptr_t addr)
{
    uintptr_t r = 0;
    ZALASR_LOAD(ZALASR_F3_B, ZALASR_F7_LD_AQ, r, addr);
    ZALASR_LOAD(ZALASR_F3_H, ZALASR_F7_LD_AQ, r, addr);
    ZALASR_LOAD(ZALASR_F3_W, ZALASR_F7_LD_AQ, r, addr);
#if __riscv_xlen == 64
    ZALASR_LOAD(ZALASR_F3_D, ZALASR_F7_LD_AQ, r, addr);
#endif
    return r;
}
static inline uintptr_t hz_all_store_rel(uintptr_t addr)
{
    ZALASR_STORE(ZALASR_F3_B, ZALASR_F7_ST_RL, addr, hz_st_val);
    ZALASR_STORE(ZALASR_F3_H, ZALASR_F7_ST_RL, addr, hz_st_val);
    ZALASR_STORE(ZALASR_F3_W, ZALASR_F7_ST_RL, addr, hz_st_val);
#if __riscv_xlen == 64
    ZALASR_STORE(ZALASR_F3_D, ZALASR_F7_ST_RL, addr, hz_st_val);
#endif
    return 0;
}
static inline uintptr_t hz_all_zalasr(uintptr_t addr)
{
    (void)hz_all_load_acq(addr);
    (void)hz_all_store_rel(addr);
    return 0;
}

/* --- reserved encodings (HZLASR-34): load without aq, store without rl --- */
static inline uintptr_t hz_vs_ld_noaq_rsv(uintptr_t addr)
{
    uintptr_t r; ZALASR_LOAD(ZALASR_F3_W, ZALASR_F7_LD_NOAQ, r, addr); return r;
}
static inline uintptr_t hz_vs_ld_rel_rsv(uintptr_t addr)
{
    uintptr_t r; ZALASR_LOAD(ZALASR_F3_W, ZALASR_F7_LD_REL, r, addr); return r;
}
static inline uintptr_t hz_vs_st_norl_rsv(uintptr_t addr)
{
    ZALASR_STORE(ZALASR_F3_W, ZALASR_F7_ST_NORL, addr, hz_st_val); return 0;
}
static inline uintptr_t hz_vs_st_acq_rsv(uintptr_t addr)
{
    ZALASR_STORE(ZALASR_F3_W, ZALASR_F7_ST_ACQ, addr, hz_st_val); return 0;
}

/* ===================================================================
 * Exception-class helpers.
 *
 * store-release is a store, so it is reported in the store/AMO class
 * (unambiguous -> hard assert). load-acquire is functionally a load but
 * lives in the AMO opcode space and zalasr.adoc does not pin its class,
 * so the load flavour is RECORD-AND-COMPARE.
 * =================================================================== */
static inline bool hzlasr_load_page_cause(uintptr_t c)
{
    return c == CAUSE_LOAD_PAGE_FAULT || c == CAUSE_LOAD_ACCESS_FAULT ||
           c == CAUSE_LOAD_ADDR_MISALIGN || c == CAUSE_LOAD_GUEST_PAGE_FAULT;
}
static inline bool hzlasr_store_page_cause(uintptr_t c)
{
    return c == CAUSE_STORE_PAGE_FAULT || c == CAUSE_STORE_ACCESS_FAULT ||
           c == CAUSE_STORE_ADDR_MISALIGN || c == CAUSE_STORE_GUEST_PAGE_FAULT;
}
static inline bool hzlasr_load_align_cause(uintptr_t c)
{
    return c == CAUSE_LOAD_ADDR_MISALIGN || c == CAUSE_LOAD_ACCESS_FAULT;
}
static inline bool hzlasr_store_align_cause(uintptr_t c)
{
    return c == CAUSE_STORE_ADDR_MISALIGN || c == CAUSE_STORE_ACCESS_FAULT;
}

/* Record-and-compare for the SPEC-ambiguous load-acquire cause class.
 * Does NOT hard-assert: prints whether the observation matches the
 * functional (load-class) expectation and flags a deviation for bugs/. */
static inline void hzlasr_record_load_cause(const char *tag, uintptr_t cause)
{
    if (hzlasr_load_page_cause(cause))
        printf("  [RECORD] %s: cause=%lu in load class {4,5,13,21} - "
               "consistent with the functional classification\n",
               tag, (unsigned long)cause);
    else
        printf("  [DEVIATION] %s: cause=%lu NOT in load class {4,5,13,21} - "
               "zalasr.adoc does not pin the load-acquire class (AMO opcode "
               "space); record to bugs/ for SPEC clarification\n",
               tag, (unsigned long)cause);
}

/* ===================================================================
 * Misaligned atomicity granule (MAG). Keyed on Zama16b, which pins the
 * granule of coherent + cacheable main memory at 16 bytes
 * (norm:zama16b_mag), the same key Zaamo/Zacas/Zabha use.
 * =================================================================== */
#ifdef ZAMA16B_SUPPORTED
#define HZLASR_MAG_DECLARED  1
#define HZLASR_MAG_GRANULE   16
#else
#define HZLASR_MAG_DECLARED  0
#define HZLASR_MAG_GRANULE   16
#endif

/* Word (4-byte) misaligned offsets: 2-byte aligned (never 4-byte aligned).
 *   INTRA    = 2  -> bytes [2,6)   inside granule [0,16)  (MAG-relaxed)
 *   STRADDLE = 14 -> bytes [14,18) cross the 16-byte boundary (fault) */
#define HZLASR_W_MIS_INTRA     2UL
#define HZLASR_W_MIS_STRADDLE  14UL

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
#define HZ_VS_XONLY   (PTE_V|PTE_X          |PTE_A|PTE_D)   /* R=0     */
#define HZ_VS_INV     (0)

#define HZ_G_RWXU     (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)
#define HZ_G_RU       (PTE_V|PTE_R          |PTE_U|PTE_A|PTE_D)  /* no W */
#define HZ_G_INV      (0)

#endif /* HYPERVISOR_ZALASR_TEST_HELPERS_H */
