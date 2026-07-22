/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CFI_ZICFISS_TEST_HELPERS_H
#define CFI_ZICFISS_TEST_HELPERS_H

#include "test_framework.h"
#include "encoding.h"
#include "types.h"
#include "vm/vm.h"

/* ===================================================================
 * CSR Access Helpers for Zicfiss
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
 * Zicfiss Detection
 *
 * Attempts to set menvcfg.SSE. If it sticks, Zicfiss is considered
 * implemented.
 * =================================================================== */

static bool zicfiss_detected = false;
static bool zicfiss_detection_done = false;

static bool detect_zicfiss(void)
{
    if (zicfiss_detection_done)
        return zicfiss_detected;

    /* Try setting menvcfg.SSE (bit 3) */
    menvcfg_set(MENVCFG_SSE);
    uintptr_t val = menvcfg_read();
    if (val & MENVCFG_SSE)
    {
        menvcfg_clear(MENVCFG_SSE);
        zicfiss_detected = true;
        zicfiss_detection_done = true;
        return true;
    }

    zicfiss_detected = false;
    zicfiss_detection_done = true;
    return false;
}

/* ===================================================================
 * Linker symbols for test regions
 * =================================================================== */
extern char __shadow_stack_start[];
extern char __shadow_stack_end[];

/* VM test region: page 0 = shadow stack test page, page 1 = normal RW page */
extern char __vm_test_region_start[];
extern char __vm_test_region_end[];

#define ss_vm_test_page  ((uintptr_t)__vm_test_region_start)
#define rw_vm_test_page  ((uintptr_t)(__vm_test_region_start) + PAGE_SIZE_4K)

/* ===================================================================
 * SSPUSH / SSPOPCHK instruction encodings (Zicfiss)
 *
 * SSPUSH x1:  funct7=1100111, rs2=00001, rs1=00000, funct3=100,
 *             rd=00000, opcode=1110011  => 0xCE104073
 * SSPOPCHK x1: imm=110011011100, rs1=00001, funct3=100,
 *             rd=00000, opcode=1110011  => 0xCDC0C073
 * =================================================================== */
#define INSN_SSPUSH_X1     0xCE104073
#define INSN_SSPOPCHK_X1   0xCDC0C073

/* ===================================================================
 * VM setup helper: identity mapping for code/data + UART
 *
 * Maps memory at PLATFORM_MEM_BASE with full RWX permissions using
 * 2MB megapages, but SKIPS the 2MB region containing the VM test
 * pages so individual tests can create 4KB mappings with custom
 * permissions (e.g. shadow stack pages with xwr=010).
 * =================================================================== */
static int setup_code_mapping(pt_context_t *ctx)
{
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);

    uintptr_t test_region = (uintptr_t)__vm_test_region_start;
    uintptr_t test_region_2m = test_region & ~(PAGE_SIZE_2M - 1);

    for (uintptr_t addr = base; addr < test_region_2m; addr += PAGE_SIZE_2M)
    {
        int ret = pt_map_page(ctx, addr, addr, flags, PT_LEVEL_2M);
        if (ret != 0)
            return ret;
    }

    /* Map one 2MB page after the test region */
    uintptr_t after_test = test_region_2m + PAGE_SIZE_2M;
    pt_map_page(ctx, after_test, after_test, flags, PT_LEVEL_2M);

    /* Map UART I/O region */
    uintptr_t uart_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;
    pt_map_page(ctx, PLATFORM_UART0_BASE, PLATFORM_UART0_BASE,
                uart_flags, PT_LEVEL_4K);

    return 0;
}

#endif /* CFI_ZICFISS_TEST_HELPERS_H */
