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

#define HVEC_CTX_MASK   0x3UL

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
 * Platform support gates
 *
 * Whether the platform implements V / F is declared by the
 * V_SUPPORTED / F_SUPPORTED definitions in
 * config/<platform>/rvtest_config.h (auto-generated from the
 * platform ISA description). No runtime probing is used.
 * =================================================================== */

/* ===================================================================
 * vsstatus / mstatus context-field accessors
 *
 * Use vsstatus_get_field/vsstatus_set_field/mstatus_get_field/
 * mstatus_set_field directly from common/hyp/hyp_csr.h.
 * =================================================================== */

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
