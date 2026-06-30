/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMP_MEM_OPS_H
#define PMP_MEM_OPS_H

#include "types.h"

/* LOAD/STORE macros for use in inline asm (adapt to XLEN) */
#if __riscv_xlen == 64
#define STORE "sd"
#define LOAD  "ld"
#else
#define STORE "sw"
#define LOAD  "lw"
#endif

/* ===================================================================
 * Memory Operation Primitives
 *
 * All load/store operations use .option norvc to ensure non-compressed
 * instructions (4 bytes each), so the trap handler can reliably skip
 * faulting instructions with mepc += 4.
 *
 * The 'volatile' and memory clobber prevent compiler reordering.
 * =================================================================== */

/* ===== Load operations ===== */

static inline uint8_t mem_load8(uintptr_t addr) {
    uint8_t val;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lb %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(val) : "r"(addr) : "memory"
    );
    return val;
}

static inline uint16_t mem_load16(uintptr_t addr) {
    uint16_t val;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lh %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(val) : "r"(addr) : "memory"
    );
    return val;
}

static inline uint32_t mem_load32(uintptr_t addr) {
    uint32_t val;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lw %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(val) : "r"(addr) : "memory"
    );
    return val;
}

#if __riscv_xlen == 64
static inline uint64_t mem_load64(uintptr_t addr) {
    uint64_t val;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(val) : "r"(addr) : "memory"
    );
    return val;
}
#endif

/* ===== Store operations ===== */

static inline void mem_store8(uintptr_t addr, uint8_t val) {
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sb %0, 0(%1)\n\t"
        ".option pop\n\t"
        :: "r"(val), "r"(addr) : "memory"
    );
}

static inline void mem_store16(uintptr_t addr, uint16_t val) {
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sh %0, 0(%1)\n\t"
        ".option pop\n\t"
        :: "r"(val), "r"(addr) : "memory"
    );
}

static inline void mem_store32(uintptr_t addr, uint32_t val) {
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sw %0, 0(%1)\n\t"
        ".option pop\n\t"
        :: "r"(val), "r"(addr) : "memory"
    );
}

#if __riscv_xlen == 64
static inline void mem_store64(uintptr_t addr, uint64_t val) {
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %0, 0(%1)\n\t"
        ".option pop\n\t"
        :: "r"(val), "r"(addr) : "memory"
    );
}
#endif

/* ===== Execute operation ===== */

/*
 * exec_at - Jump to addr and execute instructions there.
 *
 * The code at addr should be nop;ret (filled by entry.S).
 * Before jumping, we save the return address in trap_record.return_addr
 * so the trap handler can recover if an instruction access fault occurs.
 *
 * Implementation:
 *   1. Save recovery label address to trap_record.return_addr
 *   2. jalr x0, addr (jump without saving ra, since we use recovery label)
 *   3. Recovery label: clear return_addr
 */
static inline void exec_at(uintptr_t addr) {
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "la t0, 1f\n\t"                /* t0 = recovery label */
        "la t1, _exec_return_addr\n\t"  /* t1 = &_exec_return_addr */
        STORE " t0, 0(t1)\n\t"         /* save recovery address */
        "mv ra, t0\n\t"                /* ra = recovery (for normal ret) */
        "jr %0\n\t"                     /* jump to test address */
        "1:\n\t"                        /* recovery label */
        STORE " zero, 0(t1)\n\t"        /* clear recovery address */
        ".option pop\n\t"
        :: "r"(addr)
        : "t0", "t1", "ra", "memory"
    );
}

/* ===== AMO operations ===== */

static inline uint32_t mem_amo_swap_w(uintptr_t addr, uint32_t val) {
    uint32_t result;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoswap.w %0, %1, (%2)\n\t"
        ".option pop\n\t"
        : "=r"(result) : "r"(val), "r"(addr) : "memory"
    );
    return result;
}

static inline uint32_t mem_lr_w(uintptr_t addr) {
    uint32_t val;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.w %0, (%1)\n\t"
        ".option pop\n\t"
        : "=r"(val) : "r"(addr) : "memory"
    );
    return val;
}

static inline uint32_t mem_sc_w(uintptr_t addr, uint32_t val) {
    uint32_t result;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sc.w %0, %1, (%2)\n\t"
        ".option pop\n\t"
        : "=r"(result) : "r"(val), "r"(addr) : "memory"
    );
    return result;
}

#endif /* PMP_MEM_OPS_H */
