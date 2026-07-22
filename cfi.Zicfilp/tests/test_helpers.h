/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CFI_ZICFILP_TEST_HELPERS_H
#define CFI_ZICFILP_TEST_HELPERS_H

#include "test_framework.h"
#include "encoding.h"
#include "types.h"

/* ===================================================================
 * CSR Access Helpers for Zicfilp
 * =================================================================== */

static inline uintptr_t menvcfg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x30A" : "=r"(v));
    return v;
}

static inline void menvcfg_write(uintptr_t val)
{
    asm volatile("csrw 0x30A, %0" :: "r"(val) : "memory");
}

static inline void menvcfg_set(uintptr_t mask)
{
    asm volatile("csrs 0x30A, %0" :: "r"(mask) : "memory");
}

static inline void menvcfg_clear(uintptr_t mask)
{
    asm volatile("csrc 0x30A, %0" :: "r"(mask) : "memory");
}

static inline uintptr_t mseccfg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x747" : "=r"(v));
    return v;
}

static inline void mseccfg_write(uintptr_t val)
{
    asm volatile("csrw 0x747, %0" :: "r"(val) : "memory");
}

static inline void mseccfg_set(uintptr_t mask)
{
    asm volatile("csrs 0x747, %0" :: "r"(mask) : "memory");
}

static inline void mseccfg_clear(uintptr_t mask)
{
    asm volatile("csrc 0x747, %0" :: "r"(mask) : "memory");
}

static inline uintptr_t senvcfg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x10A" : "=r"(v));
    return v;
}

static inline void senvcfg_write(uintptr_t val)
{
    asm volatile("csrw 0x10A, %0" :: "r"(val) : "memory");
}

static inline void senvcfg_set(uintptr_t mask)
{
    asm volatile("csrs 0x10A, %0" :: "r"(mask) : "memory");
}

static inline void senvcfg_clear(uintptr_t mask)
{
    asm volatile("csrc 0x10A, %0" :: "r"(mask) : "memory");
}

static inline uintptr_t mstatus_read(void)
{
    return CSRR(mstatus);
}

/* ===================================================================
 * Zicfilp Detection
 *
 * Attempts to set mseccfg.MLPE and menvcfg.LPE. If either sticks,
 * Zicfilp is considered implemented.
 * =================================================================== */

static bool zicfilp_detected = false;
static bool zicfilp_detection_done = false;

static bool detect_zicfilp(void)
{
    if (zicfilp_detection_done)
        return zicfilp_detected;

    /* Try setting mseccfg.MLPE (bit 10) */
    mseccfg_set(MSECCFG_MLPE);
    uintptr_t val = mseccfg_read();
    if (val & MSECCFG_MLPE)
    {
        /* Restore: clear MLPE to avoid affecting other tests */
        mseccfg_clear(MSECCFG_MLPE);
        zicfilp_detected = true;
        zicfilp_detection_done = true;
        return true;
    }

    /* Also try menvcfg.LPE (bit 2) */
    menvcfg_set(MENVCFG_LPE);
    val = menvcfg_read();
    if (val & MENVCFG_LPE)
    {
        menvcfg_clear(MENVCFG_LPE);
        zicfilp_detected = true;
        zicfilp_detection_done = true;
        return true;
    }

    zicfilp_detected = false;
    zicfilp_detection_done = true;
    return false;
}

/* ===================================================================
 * LPAD instruction encoding
 *
 * LPAD is encoded as: AUIPC x0, imm
 * Opcode: 0x17 (AUIPC), rd=x0
 * Full 32-bit: imm[31:12] | 00000 | 0010111
 * With label=0: 0x00000017
 * =================================================================== */
#define LPAD_INSN_WORD  0x00000017  /* LPAD with label=0 */

/* ===================================================================
 * Helper: emit LPAD instruction at a given address
 * =================================================================== */
static inline void emit_lpad(void *addr)
{
    *(volatile uint32_t *)addr = LPAD_INSN_WORD;
}

/* ===================================================================
 * Helper: emit a RET (jalr x0, x1, 0) instruction
 * =================================================================== */
#define RET_INSN_WORD   0x00008067  /* jalr x0, ra, 0 */

static inline void emit_ret(void *addr)
{
    *(volatile uint32_t *)addr = RET_INSN_WORD;
}

/* ===================================================================
 * Helper: emit a NOP instruction
 * =================================================================== */
#define NOP_INSN_WORD   0x00000013  /* addi x0, x0, 0 */

static inline void emit_nop(void *addr)
{
    *(volatile uint32_t *)addr = NOP_INSN_WORD;
}

/* ===================================================================
 * Linker symbols for test regions
 * =================================================================== */
extern char __cfi_test_code_start[];
extern char __cfi_test_code_end[];

#endif /* CFI_ZICFILP_TEST_HELPERS_H */
