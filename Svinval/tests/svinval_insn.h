/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * svinval_insn.h - Svinval Extension Instruction Macros
 *
 * Defines inline assembly macros for the three Svinval instructions:
 *   - SINVAL.VMA rs1, rs2
 *   - SFENCE.W.INVAL
 *   - SFENCE.INVAL.IR
 *
 * Encoding format (R-type):
 *   .insn r opcode, funct3, funct7, rd, rs1, rs2
 *
 * sinval.vma rs1, rs2:
 *   opcode=0x73(SYSTEM), funct3=0, funct7=0x0B(0001011), rd=x0
 *
 * sfence.w.inval:
 *   opcode=0x73(SYSTEM), funct3=0, funct7=0x0C(0001100), rd=x0, rs1=x0, rs2=x0
 *
 * sfence.inval.ir:
 *   opcode=0x73(SYSTEM), funct3=0, funct7=0x0C(0001100), rd=x0, rs1=x0, rs2=x1
 */

#ifndef SVINVAL_INSN_H
#define SVINVAL_INSN_H

#include "types.h"

/*
 * Note: Macros use ({ ... }) statement expressions so they can be
 * used inside (void)(stmt) in PRIV_DO_TRAP / PRIV_DO_NO_TRAP macros.
 * Plain asm volatile(...) is a statement, not an expression, and
 * cannot be cast with (void).
 */

#ifdef USE_SVINVAL_ASM
/* Assembler natively supports Svinval mnemonics (GCC 12+ / binutils 2.38+) */

#define SINVAL_VMA(vaddr, asid) \
    ({ asm volatile("sinval.vma %0, %1" \
                    :: "r"((uintptr_t)(vaddr)), "r"((uintptr_t)(asid)) : "memory"); })

#define SFENCE_W_INVAL() \
    ({ asm volatile("sfence.w.inval" ::: "memory"); })

#define SFENCE_INVAL_IR() \
    ({ asm volatile("sfence.inval.ir" ::: "memory"); })

#else
/* Use .insn manual encoding */

#define SINVAL_VMA(vaddr, asid) \
    ({ asm volatile(".insn r 0x73, 0, 0x0B, x0, %0, %1" \
                    :: "r"((uintptr_t)(vaddr)), "r"((uintptr_t)(asid)) : "memory"); })

#define SFENCE_W_INVAL() \
    ({ asm volatile(".insn r 0x73, 0, 0x0C, x0, x0, x0" ::: "memory"); })

#define SFENCE_INVAL_IR() \
    ({ asm volatile(".insn r 0x73, 0, 0x0C, x0, x0, x1" ::: "memory"); })

#endif /* USE_SVINVAL_ASM */

#endif /* SVINVAL_INSN_H */
