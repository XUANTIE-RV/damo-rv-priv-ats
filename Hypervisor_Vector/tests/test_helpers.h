/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x V-vector suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Spec: vector-common.adoc (see the Makefile header for the norm
 * list). Vector / vector-FP instructions are raw-encoded so the build
 * march needs no v-extension support.
 */

#ifndef HYPERVISOR_VECTOR_TEST_HELPERS_H
#define HYPERVISOR_VECTOR_TEST_HELPERS_H

#include "test_framework.h"
#include "cause_defs.h"
#include "sm_defs.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_trap.h"

/* ===================================================================
 * Context-status field encodings (shared layout in mstatus/sstatus/
 * vsstatus): Off=0, Initial=1, Clean=2, Dirty=3; SD is bit 63.
 * =================================================================== */

#define CTX_OFF         0x0UL
#define CTX_INITIAL     0x1UL
#define CTX_CLEAN       0x2UL
#define CTX_DIRTY       0x3UL

#define HVEC_VS_SHIFT   9               /* mstatus/vsstatus VS field */
#define HVEC_FS_SHIFT   13              /* mstatus/vsstatus FS field */
#define HVEC_CTX_MASK   0x3UL
#define HVEC_SD_BIT     (1UL << 63)

/* ===================================================================
 * Raw vector instruction encodings
 *
 *   vsetivli x0, 4, e64, m1, ta, ma : 0xCD827057
 *     (zimm[9:0] = {00, vma=1, vta=1, vsew=011(e64), vlmul=000(m1)})
 *   vadd.vv  v1, v2, v3             : 0x023100D7
 *   vfdiv.vv v1, v2, v3 (SEW=64)    : 0x82311057
 *     (vfdiv: 0/0 deterministically raises the NV flag, so fcsr is
 *      guaranteed to change - i.e. FP state is modified, which per
 *      norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp must mark both
 *      fs fields Dirty)
 * =================================================================== */

#define HVEC_EXEC_VSEQ() \
    ({ asm volatile(".word 0xCD827057\n\t"     /* vsetivli */ \
                    ".word 0x023100D7"         /* vadd.vv  */ \
                    ::: "memory"); })

#define HVEC_EXEC_VFSEQ() \
    ({ asm volatile(".word 0xCD827057\n\t"     /* vsetivli */ \
                    ".word 0x82311057"         /* vfdiv.vv */ \
                    ::: "memory"); })

/* Raw vector CSR read: csrr x0, vstart (0x008). */
#define HVEC_EXEC_VCSR_READ() \
    ({ asm volatile(".insn i 0x73, 0x2, x0, x0, 0x008" ::: "memory"); })

/* Single vector instruction (vsetivli only). Used by the Off-gating
 * cases: with a context field Off, every vector instruction raises
 * illegal-instruction, and a single-instruction callback guarantees
 * exactly one armed trap (a two-instruction sequence would fault a
 * second time after the handler skips the first instruction). */
#define HVEC_EXEC_VSET_ONLY() \
    ({ asm volatile(".word 0xCD827057" ::: "memory"); })

/* ===================================================================
 * H extension detection
 * =================================================================== */

static bool hvec_check_h(void)
{
    uintptr_t misa = CSRR(misa);
    return (misa & (1UL << ('H' - 'A'))) != 0;
}

#define H_REQUIRED_OR_SKIP() do { \
    if (!hvec_check_h()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * Platform support gates
 *
 * Whether the platform implements V / F is declared by the
 * V_SUPPORTED / F_SUPPORTED definitions in
 * config/<platform>/rvtest_config.h (auto-generated from the
 * platform ISA description). No runtime probing is used.
 * =================================================================== */

#ifdef V_SUPPORTED
#define V_REQUIRED_OR_SKIP() do { } while (0)
#else
#define V_REQUIRED_OR_SKIP() \
    TEST_SKIP("V not supported: V_SUPPORTED not defined in rvtest_config.h")
#endif

/* Vector FP cases need F plus V. */
#if defined(V_SUPPORTED) && defined(F_SUPPORTED)
#define VF_REQUIRED_OR_SKIP() do { } while (0)
#else
#define VF_REQUIRED_OR_SKIP() \
    TEST_SKIP("vector FP not supported: V_SUPPORTED/F_SUPPORTED not " \
              "defined in rvtest_config.h")
#endif

/* ===================================================================
 * vsstatus / mstatus context-field accessors
 * =================================================================== */

#define CSR_VSSTATUS_ADDR  0x200

static inline uintptr_t hvec_vsstatus_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x200" : "=r"(v) :: "memory");
    return v;
}

static inline void hvec_vsstatus_write(uintptr_t v)
{
    asm volatile("csrw 0x200, %0" :: "r"(v) : "memory");
}

static inline unsigned hvec_vsstatus_field(unsigned shift)
{
    return (unsigned)((hvec_vsstatus_read() >> shift) & HVEC_CTX_MASK);
}

static inline void hvec_vsstatus_set_field(unsigned shift, unsigned val)
{
    uintptr_t v = hvec_vsstatus_read();
    v = (v & ~(HVEC_CTX_MASK << shift)) |
        ((uintptr_t)(val & HVEC_CTX_MASK) << shift);
    hvec_vsstatus_write(v);
}

static inline unsigned hvec_mstatus_field(unsigned shift)
{
    return (unsigned)((CSRR(mstatus) >> shift) & HVEC_CTX_MASK);
}

static inline void hvec_mstatus_set_field(unsigned shift, unsigned val)
{
    uintptr_t ms = CSRR(mstatus);
    ms = (ms & ~(HVEC_CTX_MASK << shift)) |
         ((uintptr_t)(val & HVEC_CTX_MASK) << shift);
    CSRW(mstatus, ms);
}

/* ===================================================================
 * VS/VU-mode callbacks (run_in_vs_mode / run_in_vu_mode)
 *
 * When a gating field is Off the instruction raises
 * illegal-instruction; the M-mode handler records the trap and skips
 * the faulting instruction, so the callback returns normally.
 * =================================================================== */

static uintptr_t hvec_vs_vseq(uintptr_t arg)
{
    (void)arg;
    HVEC_EXEC_VSEQ();
    return 0;
}

static uintptr_t hvec_vu_vseq(uintptr_t arg)
{
    (void)arg;
    HVEC_EXEC_VSEQ();
    return 0;
}

static uintptr_t hvec_vs_vset(uintptr_t arg)
{
    (void)arg;
    HVEC_EXEC_VSET_ONLY();
    return 0;
}

static uintptr_t hvec_vu_vset(uintptr_t arg)
{
    (void)arg;
    HVEC_EXEC_VSET_ONLY();
    return 0;
}

static uintptr_t hvec_vs_vcsr_read(uintptr_t arg)
{
    (void)arg;
    HVEC_EXEC_VCSR_READ();
    return 0;
}

static uintptr_t hvec_vs_vfseq(uintptr_t arg)
{
    (void)arg;
    HVEC_EXEC_VFSEQ();
    return 0;
}

static uintptr_t hvec_vu_vfseq(uintptr_t arg)
{
    (void)arg;
    HVEC_EXEC_VFSEQ();
    return 0;
}

#endif /* HYPERVISOR_VECTOR_TEST_HELPERS_H */
