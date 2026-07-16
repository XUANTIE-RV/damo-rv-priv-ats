/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#ifndef __ASSEMBLER__

/* ===== Basic integer types ===== */
typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef int                int32_t;
typedef unsigned int       uint32_t;
typedef long long          int64_t;
typedef unsigned long long uint64_t;

/* ===== Pointer-width types (adapts to RV32/RV64) ===== */
#if __riscv_xlen == 64
typedef unsigned long long size_t;
typedef long long          ssize_t;
typedef unsigned long long uintptr_t;
typedef long long          intptr_t;
#define XLEN 64
#define PRIxPTR "llx"
#else
typedef unsigned int       size_t;
typedef int                ssize_t;
typedef unsigned int       uintptr_t;
typedef int                intptr_t;
#define XLEN 32
#define PRIxPTR "x"
#endif

/* ===== Boolean ===== */
#define bool  _Bool
#define true  ((_Bool)+1u)
#define false ((_Bool)+0u)

/* ===== NULL ===== */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ===== Bit manipulation helpers =====
 *
 * All macros cast to uintptr_t so results are XLEN-width:
 *   - On RV64, uintptr_t is 64-bit: full value preserved.
 *   - On RV32, uintptr_t is 32-bit: high bits truncate to 0,
 *     which is correct for single-CSR access (high-half bits
 *     are accessed via separate mstatush/envcfgh etc.).
 */
#define BIT(n)              ((uintptr_t)(1ULL << (n)))
#define BIT_MASK(off, len)  ((uintptr_t)(((1ULL << (len)) - 1) << (off)))
#define EXTRACT_FIELD(val, off, len) \
    (((uintptr_t)(val) >> (off)) & ((uintptr_t)(1ULL << (len)) - 1))
#define INSERT_FIELD(val, off, len, field) \
    (((uintptr_t)(val) & ~BIT_MASK(off, len)) | (((uintptr_t)(field) << (off)) & BIT_MASK(off, len)))

#endif /* __ASSEMBLER__ */

#endif /* COMMON_TYPES_H */
