/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_ENCODING_H
#define COMMON_ENCODING_H

/* ===================================================================
 * RISC-V Encoding — Umbrella Header
 *
 * This header provides the full set of RISC-V privileged encoding
 * constants and inline CSR access macros. It is the single include
 * point for all C and assembly source files in the framework.
 *
 * Contents are split into focused sub-headers by privilege level:
 *   sm_defs.h     — M-mode CSR addresses and bit fields
 *   ss_defs.h     — S-mode CSR addresses and bit fields
 *   sh_defs.h     — Hypervisor CSR addresses and bit fields
 *   cause_defs.h  — Exception / interrupt cause codes
 *
 * This file retains: CSR access macros, privilege level constants,
 * unprivileged CSRs, PM common constants, shared stateen bits, and
 * cross-level utility macros.
 * =================================================================== */

#include "types.h"
#include "sm_defs.h"
#include "ss_defs.h"
#include "sh_defs.h"
#include "cause_defs.h"

/* ===================================================================
 * Inline CSR access macros (compile-time CSR name)
 *
 * These macros generate csrr/csrw/csrs/csrc instructions with
 * compile-time constant CSR addresses. For runtime CSR address
 * access, use csr_read()/csr_write() from csr_accessors.c.
 *
 * Guarded by __ASSEMBLER__ — not available in assembly files.
 * =================================================================== */
#ifndef __ASSEMBLER__

#ifndef _CSR_STR
#define _CSR_STR(s) #s
#endif
#ifndef CSR_STR
#define CSR_STR(s)  _CSR_STR(s)
#endif

#if __riscv_xlen == 64
#define CSRR(csr) ({ \
    uint64_t _v; \
    asm volatile("csrr %0, " CSR_STR(csr) : "=r"(_v) :: "memory"); \
    _v; \
})
#else
#define CSRR(csr) ({ \
    uint32_t _v; \
    asm volatile("csrr %0, " CSR_STR(csr) : "=r"(_v) :: "memory"); \
    _v; \
})
#endif

#define CSRW(csr, val) \
    asm volatile("csrw " CSR_STR(csr) ", %0" :: "rK"(val) : "memory")

#define CSRS(csr, val) \
    asm volatile("csrs " CSR_STR(csr) ", %0" :: "rK"(val) : "memory")

#define CSRC(csr, val) \
    asm volatile("csrc " CSR_STR(csr) ", %0" :: "rK"(val) : "memory")

/* SFENCE.VMA */
static inline void sfence_vma(void) {
    asm volatile("sfence.vma" ::: "memory");
}

#endif /* __ASSEMBLER__ */

/* ===================================================================
 * Privilege Level Definitions
 *
 * Encoding:
 *   PRIV_U  = 0   User mode
 *   PRIV_S  = 1   Supervisor mode
 *   PRIV_M  = 3   Machine mode
 *
 * Virtualized privilege levels (V=1) — used by hypervisor (H ext) tests.
 * The high bit (bit 2) marks V=1; low 2 bits hold the nominal privilege.
 *   PRIV_VU = 4   V=1, nominal U
 *   PRIV_VS = 5   V=1, nominal S
 * =================================================================== */

#define PRIV_U  0
#define PRIV_S  1
#define PRIV_M  3

#define PRIV_VU 4
#define PRIV_VS 5

/* ===================================================================
 * Ecall protocol for privilege switching (goto_priv)
 *
 * When goto_priv() needs to switch to a higher privilege level,
 * it issues an ecall with a0 = ECALL_GOTO_PRIV and a1 = target priv.
 * The trap handler checks ecall_args[0] to distinguish this from
 * test-triggered ecalls.
 * =================================================================== */
#define ECALL_GOTO_PRIV  1

/* ===================================================================
 * Unprivileged CSRs (U-mode read-only)
 * =================================================================== */

/* Shadow Stack Pointer (Zicfiss) */
#define CSR_SSP         0x011

/* Base Counter CSRs (S/U-mode read-only) */
#define CSR_CYCLE       0xC00
#define CSR_TIME        0xC01
#define CSR_INSTRET     0xC02

/* HPM Counter CSRs (S/U-mode read-only shadow) */
#define CSR_HPMCOUNTER3   0xC03
#define CSR_HPMCOUNTER4   0xC04
#define CSR_HPMCOUNTER5   0xC05
#define CSR_HPMCOUNTER6   0xC06
#define CSR_HPMCOUNTER7   0xC07
#define CSR_HPMCOUNTER8   0xC08
#define CSR_HPMCOUNTER9   0xC09
#define CSR_HPMCOUNTER10  0xC0A
#define CSR_HPMCOUNTER11  0xC0B
#define CSR_HPMCOUNTER12  0xC0C
#define CSR_HPMCOUNTER13  0xC0D
#define CSR_HPMCOUNTER14  0xC0E
#define CSR_HPMCOUNTER15  0xC0F
#define CSR_HPMCOUNTER16  0xC10
#define CSR_HPMCOUNTER17  0xC11
#define CSR_HPMCOUNTER18  0xC12
#define CSR_HPMCOUNTER19  0xC13
#define CSR_HPMCOUNTER20  0xC14
#define CSR_HPMCOUNTER21  0xC15
#define CSR_HPMCOUNTER22  0xC16
#define CSR_HPMCOUNTER23  0xC17
#define CSR_HPMCOUNTER24  0xC18
#define CSR_HPMCOUNTER25  0xC19
#define CSR_HPMCOUNTER26  0xC1A
#define CSR_HPMCOUNTER27  0xC1B
#define CSR_HPMCOUNTER28  0xC1C
#define CSR_HPMCOUNTER29  0xC1D
#define CSR_HPMCOUNTER30  0xC1E
#define CSR_HPMCOUNTER31  0xC1F

/* ===================================================================
 * Pointer Masking (PM) common constants
 * =================================================================== */
#define PMM_DISABLED    0   /* Pointer masking disabled (PMLEN=0) */
#define PMM_RESERVED    1   /* Reserved */
#define PMM_PMLEN7      2   /* PMLEN = XLEN-57 = 7 on RV64 */
#define PMM_PMLEN16     3   /* PMLEN = XLEN-48 = 16 on RV64 */

/* ===================================================================
 * Stateen shared bit definitions (common to M/H/S levels)
 * =================================================================== */

/* Cast to uintptr_t: on RV64 preserves full value; on RV32 the
 * high bits are 0 (high-half register access needed for bits >= 32). */
#define STATEEN_BIT63      ((uintptr_t)(1ULL << 63))
#define STATEEN_BIT58      ((uintptr_t)(1ULL << 58))

/* stateen0 shared bit definitions */
#define STATEEN0_C         ((uintptr_t)(1ULL << 0))
#define STATEEN0_FCSR      ((uintptr_t)(1ULL << 1))
#define STATEEN0_JVT       ((uintptr_t)(1ULL << 2))
#define STATEEN0_CONTEXT   ((uintptr_t)(1ULL << 57))
#define STATEEN0_IMSIC     ((uintptr_t)(1ULL << 58))
#define STATEEN0_AIA       ((uintptr_t)(1ULL << 59))
#define STATEEN0_CSRIND    ((uintptr_t)(1ULL << 60))
#define STATEEN0_ENVCFG    ((uintptr_t)(1ULL << 62))
#define STATEEN0_SE0       STATEEN_BIT63

/* ===================================================================
 * Cross-level utility macros
 * =================================================================== */

/* HGATP mode matches SATP mode numerically */
#define HGATP_MODE_FOR_SATP(satp_mode) (satp_mode)

#endif /* COMMON_ENCODING_H */
