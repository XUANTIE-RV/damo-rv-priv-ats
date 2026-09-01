/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Zihintntl suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Core assertion strategy (plan Group 2 / zihintntl_test_plan.md):
 * the "HINT no-side-effect comparison" - the architecturally visible
 * behavior of an NTL-prefixed sequence (results, exceptions and trap
 * report) must be identical to the same sequence without the prefix.
 *
 * Spec: zihintntl.adoc norm:NTL_target_definition / norm:NTL_range.
 */

#ifndef HYPERVISOR_ZIHINTNTL_TEST_HELPERS_H
#define HYPERVISOR_ZIHINTNTL_TEST_HELPERS_H

#include "test_framework.h"
#include "cause_defs.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_fence.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/test_vs_helpers.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/hyp_platform.h"
#include "hyp/hyp_ldst.h"
#include "cmo.h"

/* ===================================================================
 * NTL hint encodings (ADD with rd=x0, raw .word injection so the
 * build march needs no Zihintntl mnemonic support)
 * =================================================================== */

#define NTL_P1_ENC      0x00200033UL
#define NTL_PALL_ENC    0x00300033UL
#define NTL_S1_ENC      0x00400033UL
#define NTL_ALL_ENC     0x00500033UL

#define EXEC_NTL_P1()   ({ asm volatile(".word 0x00200033" ::: "memory"); })
#define EXEC_NTL_ALL()  ({ asm volatile(".word 0x00500033" ::: "memory"); })

/* ===================================================================
 * Shared comparison buffers
 * =================================================================== */

#define NTL_MAGIC       0xA5A5A5A5A5A5A5A5UL
#define NTL_ALT_MAGIC   0x5A5A5A5A5A5A5A5AUL

static volatile uint64_t ntl_mem;       /* load target */
static volatile uint64_t ntl_gva_mem;   /* HLV/HSV guest area */
static volatile uint64_t ntl_gva_aux;   /* HSV comparison slot */

/* ===================================================================
 * H extension detection
 * =================================================================== */

static bool check_h_extension(void)
{
    uintptr_t misa_val = CSRR(misa);
    return (misa_val & (1UL << ('H' - 'A'))) != 0;
}

#define H_REQUIRED_OR_SKIP() do { \
    if (!check_h_extension()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * Zicbom detection (NTL-HYP-05): probe menvcfg.CBIE writability
 * =================================================================== */

static bool ntl_zicbom_detected = false;
static bool ntl_zicbom_detection_done = false;

static bool check_zicbom_extension(void)
{
    if (ntl_zicbom_detection_done)
        return ntl_zicbom_detected;

    uintptr_t orig = menvcfg_read();
    menvcfg_set_cbie(CBIE_INVAL);
    uintptr_t val = menvcfg_get_cbie();
    ntl_zicbom_detected = (val == CBIE_INVAL);
    menvcfg_write(orig);
    ntl_zicbom_detection_done = true;
    return ntl_zicbom_detected;
}

#define ZICBOM_REQUIRED_OR_SKIP() do { \
    if (!check_zicbom_extension()) { \
        TEST_SKIP("Zicbom not available (menvcfg.CBIE read-only)"); \
    } \
} while (0)

/* ===================================================================
 * henvcfg CMO field accessors (bit positions shared with menvcfg)
 * =================================================================== */

static inline void ntl_henvcfg_set_cbie(unsigned cbie)
{
    uintptr_t val = henvcfg_read();
    val = (val & ~ENVCFG_CBIE_MASK) | ((uintptr_t)cbie << ENVCFG_CBIE_SHIFT);
    henvcfg_write(val);
}

static inline void ntl_henvcfg_set_cbcfe(unsigned en)
{
    uintptr_t val = henvcfg_read();
    if (en)
        val |= ENVCFG_CBCFE;
    else
        val &= ~ENVCFG_CBCFE;
    henvcfg_write(val);
}

/* ===================================================================
 * HS-mode executors: NTL prefix + load / HLV / HSV / HLVX
 * =================================================================== */

static inline uint64_t hs_ntl_all_ld(volatile void *addr)
{
    uint64_t v;
    asm volatile(
        ".word 0x00500033\n\t"     /* ntl.all */
        "ld %0, (%1)"
        : "=r"(v) : "r"(addr) : "memory");
    return v;
}

static inline uint64_t hs_plain_ld(volatile void *addr)
{
    uint64_t v;
    asm volatile("ld %0, (%1)" : "=r"(v) : "r"(addr) : "memory");
    return v;
}

/* NTL range covers the H-extension virtual-machine load/store
 * instructions (norm:NTL_range): ntl.all + hlv.d */
static inline uint64_t hs_ntl_all_hlv_d(uintptr_t addr)
{
    uint64_t v;
    asm volatile(
        ".word 0x00500033\n\t"     /* ntl.all */
        "hlv.d %0, (%1)"
        : "=r"(v) : "r"(addr) : "memory");
    return v;
}

/* ntl.all + hsv.d */
static inline void hs_ntl_all_hsv_d(uintptr_t addr, uint64_t val)
{
    asm volatile(
        ".word 0x00500033\n\t"     /* ntl.all */
        "hsv.d %1, (%0)"
        :: "r"(addr), "r"(val) : "memory");
}

/* ntl.all + hlvx.wu (execute-permission load variant) */
static inline uint32_t hs_ntl_all_hlvx_wu(uintptr_t addr)
{
    uint32_t v;
    asm volatile(
        ".word 0x00500033\n\t"     /* ntl.all */
        "hlvx.wu %0, (%1)"
        : "=r"(v) : "r"(addr) : "memory");
    return v;
}

/* ===================================================================
 * VS/VU-mode callbacks (run_in_vs_mode / run_in_vu_mode)
 * =================================================================== */

static uintptr_t vs_ntl_all_ld(uintptr_t arg)
{
    volatile uint64_t *p = (volatile uint64_t *)arg;
    uint64_t v;
    asm volatile(
        ".word 0x00500033\n\t"     /* ntl.all */
        "ld %0, (%1)"
        : "=r"(v) : "r"(p) : "memory");
    return (uintptr_t)v;
}

static uintptr_t vs_plain_ld(uintptr_t arg)
{
    volatile uint64_t *p = (volatile uint64_t *)arg;
    return (uintptr_t)*p;
}

static uintptr_t vu_ntl_all_ld(uintptr_t arg)
{
    volatile uint64_t *p = (volatile uint64_t *)arg;
    uint64_t v;
    asm volatile(
        ".word 0x00500033\n\t"     /* ntl.all */
        "ld %0, (%1)"
        : "=r"(v) : "r"(p) : "memory");
    return (uintptr_t)v;
}

static uintptr_t vu_plain_ld(uintptr_t arg)
{
    volatile uint64_t *p = (volatile uint64_t *)arg;
    return (uintptr_t)*p;
}

/* ===================================================================
 * Two-stage callbacks for NTL-HYP-06 (VS-mode ld on a G-invalid page)
 * =================================================================== */

static uintptr_t vs_ntl_p1_ld(uintptr_t arg)
{
    volatile uint64_t *p = (volatile uint64_t *)arg;
    uint64_t v;
    asm volatile(
        ".word 0x00200033\n\t"     /* ntl.p1 */
        "ld %0, (%1)"
        : "=r"(v) : "r"(p) : "memory");
    return (uintptr_t)v;
}

static uintptr_t vs_ld_only(uintptr_t arg)
{
    volatile uint64_t *p = (volatile uint64_t *)arg;
    uint64_t v;
    asm volatile("ld %0, (%1)" : "=r"(v) : "r"(p) : "memory");
    return (uintptr_t)v;
}

/* ===================================================================
 * CBO sequences with trapping-instruction PC capture (NTL-HYP-05)
 *
 * cbo_pc_out receives the address of the CBO instruction itself
 * (the ntl prefix, when present, precedes it). The M-mode handler
 * advances epc past the CBO instruction when the trap fires.
 * =================================================================== */

#define NTL_CBO_INVAL_SEQ(cbo_pc_out) do { \
    asm volatile( \
        ".balign 4\n\t" \
        "la %0, 990f\n\t" \
        ".word 0x00500033\n\t"     /* ntl.all */ \
        ".balign 4\n\t" \
        "990:\n\t" \
        ".insn i 0x0F, 0x2, x0, %1, 0x000\n\t"  /* cbo.inval */ \
        : "=&r"(cbo_pc_out) : "r"(&ntl_mem) : "memory"); \
} while (0)

#define NTL_CBO_CLEAN_SEQ(cbo_pc_out) do { \
    asm volatile( \
        ".balign 4\n\t" \
        "la %0, 990f\n\t" \
        ".word 0x00500033\n\t"     /* ntl.all */ \
        ".balign 4\n\t" \
        "990:\n\t" \
        ".insn i 0x0F, 0x2, x0, %1, 0x001\n\t"  /* cbo.clean */ \
        : "=&r"(cbo_pc_out) : "r"(&ntl_mem) : "memory"); \
} while (0)

#define PLAIN_CBO_INVAL_SEQ(cbo_pc_out) do { \
    asm volatile( \
        ".balign 4\n\t" \
        "la %0, 990f\n\t" \
        ".balign 4\n\t" \
        "990:\n\t" \
        ".insn i 0x0F, 0x2, x0, %1, 0x000\n\t"  /* cbo.inval */ \
        : "=&r"(cbo_pc_out) : "r"(&ntl_mem) : "memory"); \
} while (0)

#define PLAIN_CBO_CLEAN_SEQ(cbo_pc_out) do { \
    asm volatile( \
        ".balign 4\n\t" \
        "la %0, 990f\n\t" \
        ".balign 4\n\t" \
        "990:\n\t" \
        ".insn i 0x0F, 0x2, x0, %1, 0x001\n\t"  /* cbo.clean */ \
        : "=&r"(cbo_pc_out) : "r"(&ntl_mem) : "memory"); \
} while (0)

/* ===================================================================
 * VS-mode CBO callbacks (run_in_vs_mode): the trap must be raised in
 * V=1, so the sequences cannot run in HS-mode. The CBO instruction
 * PC is recorded into a global for the trap-report comparison.
 * =================================================================== */

static uintptr_t ntl_g_cbo_pc;

static uintptr_t vs_ntl_cbo_inval(uintptr_t arg)
{
    (void)arg;
    NTL_CBO_INVAL_SEQ(ntl_g_cbo_pc);
    return 0;
}

static uintptr_t vs_ntl_cbo_clean(uintptr_t arg)
{
    (void)arg;
    NTL_CBO_CLEAN_SEQ(ntl_g_cbo_pc);
    return 0;
}

static uintptr_t vs_plain_cbo_inval(uintptr_t arg)
{
    (void)arg;
    PLAIN_CBO_INVAL_SEQ(ntl_g_cbo_pc);
    return 0;
}

static uintptr_t vs_plain_cbo_clean(uintptr_t arg)
{
    (void)arg;
    PLAIN_CBO_CLEAN_SEQ(ntl_g_cbo_pc);
    return 0;
}

/* ===================================================================
 * Trap-report snapshot used by the comparison cases
 * =================================================================== */

typedef struct {
    bool      triggered;
    uintptr_t cause;
    uintptr_t epc;
    uintptr_t tval;
} ntl_trap_rep_t;

static inline void ntl_capture_rep(ntl_trap_rep_t *r)
{
    r->triggered = trap_was_triggered();
    r->cause     = trap_get_cause();
    r->epc       = trap_get_epc();
    r->tval      = trap_get_tval();
}

/* ===================================================================
 * Linker-provided two-stage test region (NTL-HYP-06)
 * =================================================================== */

extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

#ifndef TEST_REGION_BASE
#define TEST_REGION_BASE  ((uintptr_t)__vm_test_region_start)
#endif

#endif /* HYPERVISOR_ZIHINTNTL_TEST_HELPERS_H */
