/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SS_DEFS_H
#define SS_DEFS_H

/* ===================================================================
 * RISC-V Supervisor-mode CSR Definitions
 *
 * All S-mode CSR addresses and associated bit-field definitions.
 * Safe for both C and assembly inclusion (plain integer constants).
 * =================================================================== */

/* ----- Supervisor-level CSRs ----- */
#define CSR_SSTATUS     0x100
#define CSR_SIE         0x104
#define CSR_STVEC       0x105
#define CSR_SCOUNTEREN  0x106
#define CSR_SSCRATCH    0x140
#define CSR_SEPC        0x141
#define CSR_SCAUSE      0x142
#define CSR_STVAL       0x143
#define CSR_SIP         0x144
#define CSR_SATP        0x180

/* ----- Supervisor Environment Configuration ----- */
#define CSR_SENVCFG     0x10A
#define CSR_SENVCFGH    0x11A   /* RV32 only */

#define SENVCFG_LPE     (1ULL << 2)    /* Landing Pad Enable for U-mode (Zicfilp) */
#define SENVCFG_SSE     (1ULL << 3)    /* Shadow Stack Enable for U-mode (Zicfiss) */
#define SENVCFG_CBIE_SHIFT  4            /* CBIE field offset (Zicbom) */
#define SENVCFG_CBIE_MASK   (3ULL << 4)  /* CBIE field mask [5:4] (Zicbom) */
#define SENVCFG_CBCFE   (1ULL << 6)     /* Cache-Block Clean/Flush Enable (Zicbom) */
#define SENVCFG_CBZE    (1ULL << 7)     /* Cache-Block Zero Enable (Zicboz) */
#define SENVCFG_PMM_OFF  32             /* PMM field offset in senvcfg */
#define SENVCFG_PMM_MASK (3ULL << 32)   /* PMM field mask [33:32] (Ssnpm) */

/* ===================================================================
 * satp layout constants
 *
 * RV32 (Sv32): [31] MODE | [30:22] ASID | [21:0] PPN
 * RV64 (Sv39+): [63:60] MODE | [59:44] ASID | [43:0] PPN
 * =================================================================== */

/* RV64 layout */
#ifndef SATP64_MODE_SHIFT
#define SATP64_MODE_SHIFT  60
#endif
#ifndef SATP64_ASID_SHIFT
#define SATP64_ASID_SHIFT  44
#endif
#ifndef SATP64_PPN_MASK
#define SATP64_PPN_MASK    ((1ULL << 44) - 1)
#endif
#define SATP64_ASID_MASK   ((1ULL << 16) - 1)

/* RV32 layout (Sv32) */
#define SATP32_MODE_SHIFT  31
#define SATP32_ASID_SHIFT  22
#define SATP32_PPN_MASK    ((1UL << 22) - 1)
#define SATP32_ASID_MASK   ((1UL << 9) - 1)

/* XLEN-aware generic names.
 * vm_defs.h provides fallback RV64 values with #ifndef guards;
 * since encoding.h (-> ss_defs.h) is included before vm.h (-> vm_defs.h),
 * these definitions take precedence and vm_defs.h skips its own. */
#if __riscv_xlen == 32
#define SATP_MODE_SHIFT    SATP32_MODE_SHIFT
#define SATP_ASID_SHIFT    SATP32_ASID_SHIFT
#define SATP_PPN_MASK      SATP32_PPN_MASK
#define SATP_ASID_MASK     SATP32_ASID_MASK
#define SATP_MODE_BITS     1
#else
#define SATP_MODE_SHIFT    SATP64_MODE_SHIFT
#define SATP_ASID_SHIFT    SATP64_ASID_SHIFT
#define SATP_PPN_MASK      SATP64_PPN_MASK
#define SATP_ASID_MASK     SATP64_ASID_MASK
#define SATP_MODE_BITS     4
#endif

/* SATP MODE field values */
#ifndef SATP_MODE_BARE
#define SATP_MODE_BARE     0
#define SATP_MODE_SV32     1
#define SATP_MODE_SV39     8
#define SATP_MODE_SV48     9
#define SATP_MODE_SV57     10
#endif

#ifndef MAKE_SATP
#define MAKE_SATP(mode, asid, ppn) \
    ((uintptr_t)( \
    (((uintptr_t)(mode) << SATP_MODE_SHIFT) | \
     (((uintptr_t)(asid) & SATP_ASID_MASK) << SATP_ASID_SHIFT) | \
     ((uintptr_t)(ppn) & SATP_PPN_MASK))))
#endif

#define SATP_GET_MODE(v)   (((uintptr_t)(v) >> SATP_MODE_SHIFT) & ((1UL << SATP_MODE_BITS) - 1))
#define SATP_GET_ASID(v)   (((uintptr_t)(v) >> SATP_ASID_SHIFT) & SATP_ASID_MASK)
#define SATP_GET_PPN(v)    ((uintptr_t)(v) & SATP_PPN_MASK)

/* ===================================================================
 * Sstc Extension: S-mode CSRs and field definitions
 * =================================================================== */
#define CSR_STIMECMP    0x14D
#define CSR_STIMECMPH   0x15D   /* RV32 only */

#define SIP_STIP        (1ULL << 5)
#define SIE_STIE        (1ULL << 5)

/* ===================================================================
 * Supervisor Count Overflow (Sscofpmf)
 * =================================================================== */
#define CSR_SCOUNTOVF     0xDA0

/* ===================================================================
 * Smstateen S-mode CSRs
 * =================================================================== */
#define CSR_SSTATEEN0      0x10C
#define CSR_SSTATEEN1      0x10D
#define CSR_SSTATEEN2      0x10E
#define CSR_SSTATEEN3      0x10F

#endif /* SS_DEFS_H */
