/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * rv32_csr_helpers.h - Common RV32/RV64 CSR 64-bit logical view helpers
 *
 * On RV32, many 64-bit CSRs are split into a low-half (original address)
 * and a high-half (original + 0x80). This header provides generic read/write
 * MACROS that present a unified 64-bit logical view regardless of XLEN.
 *
 * IMPORTANT: These macros use inline asm with literal CSR addresses, NOT
 * csr_read()/csr_write() from csr_accessors.c. This avoids the pmpcfg
 * address conflict (0x3A0-0x3AF) that would corrupt mhpmeventh access.
 *
 * Usage:
 *   uint64_t val = CSR_READ64(0x30A, 0x31A);  // menvcfg 64-bit view
 *   CSR_WRITE64(0x30A, 0x31A, val);
 *   CSR_SET64(0x30A, 0x31A, MENVCFG_STCE);
 *   CSR_CLEAR64(0x30A, 0x31A, MENVCFG_STCE);
 */

#ifndef RV32_CSR_HELPERS_H
#define RV32_CSR_HELPERS_H

#include "types.h"

/* ===================================================================
 * CSR address stringification macro
 *
 * Converts a numeric CSR address to a string literal for use in
 * inline asm instructions (csrr, csrw, csrs, csrc).
 * Also defined in common/encoding.h — use #ifndef to avoid conflict.
 * =================================================================== */
#ifndef _CSR_STR
#define _CSR_STR(addr)  #addr
#endif
#ifndef CSR_STR
#define CSR_STR(addr)   _CSR_STR(addr)
#endif

/* ===================================================================
 * XLEN-width single CSR read/write/set/clear (inline asm macros)
 *
 * These operate on a single CSR at a COMPILE-TIME constant address.
 * =================================================================== */
#define CSR_RD(addr) ({ \
    uintptr_t _val; \
    asm volatile("csrr %0, " CSR_STR(addr) : "=r"(_val) :: "memory"); \
    _val; \
})

#define CSR_WR(addr, val) ({ \
    asm volatile("csrw " CSR_STR(addr) ", %0" :: "r"((uintptr_t)(val)) : "memory"); \
})

#define CSR_SET(addr, bits) ({ \
    asm volatile("csrs " CSR_STR(addr) ", %0" :: "r"((uintptr_t)(bits)) : "memory"); \
})

#define CSR_CLR(addr, bits) ({ \
    asm volatile("csrc " CSR_STR(addr) ", %0" :: "r"((uintptr_t)(bits)) : "memory"); \
})

/* ===================================================================
 * 64-bit CSR logical view macros
 *
 * On RV64: operate on the single CSR (csr_lo only; csr_hi ignored).
 * On RV32: read/write both halves, presenting a uint64_t logical view.
 *
 * Parameters:
 *   csr_lo - low-half CSR address (e.g., 0x30A for menvcfg)
 *   csr_hi - high-half CSR address (e.g., 0x31A for menvcfgh)
 * =================================================================== */

#if __riscv_xlen == 32

/* RV32: read low half + high half, combine into uint64_t */
#define CSR_READ64(csr_lo, csr_hi) ({ \
    uint32_t _lo = CSR_RD(csr_lo); \
    uint32_t _hi = CSR_RD(csr_hi); \
    ((uint64_t)_hi << 32) | (uint64_t)_lo; \
})

/* RV32: split uint64_t value, write low half then high half */
#define CSR_WRITE64(csr_lo, csr_hi, val64) ({ \
    CSR_WR(csr_lo, (uint32_t)(val64)); \
    CSR_WR(csr_hi, (uint32_t)((uint64_t)(val64) >> 32)); \
})

/* RV32: set bits across both halves.
 * bits_lo = bits that land in the low-half CSR,
 * bits_hi = bits that land in the high-half CSR.
 * Note: on RV64, bits_hi is unused. */
#define CSR_SET64(csr_lo, csr_hi, bits_lo, bits_hi) ({ \
    CSR_SET(csr_lo, bits_lo); \
    CSR_SET(csr_hi, bits_hi); \
})

/* RV32: clear bits across both halves */
#define CSR_CLEAR64(csr_lo, csr_hi, bits_lo, bits_hi) ({ \
    CSR_CLR(csr_lo, bits_lo); \
    CSR_CLR(csr_hi, bits_hi); \
})

#else /* __riscv_xlen == 64 */

/* RV64: single 64-bit CSR, csr_hi ignored */
#define CSR_READ64(csr_lo, csr_hi) ({ \
    (uint64_t)CSR_RD(csr_lo); \
})

#define CSR_WRITE64(csr_lo, csr_hi, val64) ({ \
    CSR_WR(csr_lo, (uintptr_t)(val64)); \
})

/* RV64: bits all in one CSR, bits_hi unused */
#define CSR_SET64(csr_lo, csr_hi, bits, _bits_hi_unused) ({ \
    CSR_SET(csr_lo, (uintptr_t)(bits)); \
})

#define CSR_CLEAR64(csr_lo, csr_hi, bits, _bits_hi_unused) ({ \
    CSR_CLR(csr_lo, (uintptr_t)(bits)); \
})

#endif /* __riscv_xlen */

/* ===================================================================
 * XLEN-width convenience constants
 *
 * These provide XLEN-width bit patterns useful for CSR read/write tests.
 * =================================================================== */

#if __riscv_xlen == 64
#define XLEN_ALL_ONES       0xFFFFFFFFFFFFFFFFULL
#define XLEN_PATTERN_ALT    0x5555555555555555ULL
#define XLEN_PATTERN_INV    0xAAAAAAAAAAAAAAAAULL
#else
#define XLEN_ALL_ONES       0xFFFFFFFFUL
#define XLEN_PATTERN_ALT    0x55555555UL
#define XLEN_PATTERN_INV    0xAAAAAAAAUL
#endif

#endif /* RV32_CSR_HELPERS_H */
