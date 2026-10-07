/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Svadu extension test cases
 *
 * Provides shared test data areas, S-mode test functions, Svadu detection,
 * menvcfg.ADUE access helpers, and PTE inspection/modification utilities
 * used by Svadu test files.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 *
 * Svadu semantics (per RISC-V Privileged Spec):
 *   When Svadu is implemented AND menvcfg.ADUE=1, the hardware atomically
 *   sets PTE.A on access and PTE.D on store/AMO. When menvcfg.ADUE=0, the
 *   processor falls back to Svade behavior (page-fault instead of HW update).
 */

#ifndef SVADU_TEST_HELPERS_H
#define SVADU_TEST_HELPERS_H

#include "test_framework.h"
#include "csr_ops.h"
#include "vm/vm.h"
#include "test_utils.h"
#include "mem_ops.h"

/* SUITE_SATP_MODE (this suite's S-stage paging mode) is set by the suite
 * Makefile: `SUITE_SATP_MODE ?= PLATFORM_SATP_MODE`, overridable with
 * `make SATP_MODE=sv39|sv48|sv57` (see common/Makefile.common). Every
 * pt_init()/satp write below passes SUITE_SATP_MODE. */

/* ===================================================================
 * Test data and executable regions
 *
 * Provided by svadu/kernel.ld (same layout as svade/kernel.ld):
 *   .vm_test_region    : 3 x 4KB pages (data / fault / exec) - 4K tests
 *   .vm_test_region_2m : 2 MiB region                         - 2M megapage
 * =================================================================== */
extern uintptr_t __vm_test_region_start;
extern uintptr_t __vm_test_region_2m_start;

/* 4K test pages (within .vm_test_region) */
#define test_data_area  ((volatile uintptr_t *)&__vm_test_region_start)
#define test_fault_page ((volatile uint8_t *)((uintptr_t)&__vm_test_region_start + PAGE_SIZE_4K))
#define test_exec_page  ((uint8_t *)((uintptr_t)&__vm_test_region_start + 2 * PAGE_SIZE_4K))

/* 2 MiB megapage test region */
#define test_region_2m_va ((uintptr_t)&__vm_test_region_2m_start)
#define test_region_2m_pa ((uintptr_t)&__vm_test_region_2m_start)

/* MAGIC_WRITE / MAGIC_READ are provided by common/vm/vm_defs.h. */

/* ===================================================================
 * Snapshot of menvcfg at boot (populated by main.c before any test).
 * Used by SVADU-CSR-04 to read the reset value of ADUE.
 * =================================================================== */
extern uintptr_t g_menvcfg_reset_value;

/* ===================================================================
 * menvcfg CSR (0x30A) access helpers
 *
 * Inline asm uses the literal 0x30A because csrr/csrs/csrc require
 * the CSR number to be an immediate (not a macro that expands to one
 * at a different stage). common/encoding.h also defines:
 *   #define CSR_MENVCFG  0x30A
 *   #define MENVCFG_ADUE (1ULL << 61)
 * =================================================================== */
static inline void menvcfg_set(uintptr_t mask) {
    asm volatile ("csrs " CSR_STR(CSR_MENVCFG) ", %0" :: "r"(mask) : "memory");
}

static inline void menvcfg_clear(uintptr_t mask) {
    asm volatile ("csrc " CSR_STR(CSR_MENVCFG) ", %0" :: "r"(mask) : "memory");
}

/* Set or clear menvcfg.ADUE and issue the required SFENCE.VMA
 * (SPEC/hypervisor.adoc:2109-2111 requires synchronization after
 * modifying ADUE on non-Hyp platforms). */
static inline void set_menvcfg_adue(int enable) {
    if (enable) menvcfg_set(MENVCFG_ADUE);
    else        menvcfg_clear(MENVCFG_ADUE);
    asm volatile ("sfence.vma zero, zero" ::: "memory");
}

static inline int get_menvcfg_adue(void) {
    return (menvcfg_read() & MENVCFG_ADUE) ? 1 : 0;
}

/* ===================================================================
 * Initialization helper: fill exec page with nop;ret
 * =================================================================== */
static void init_exec_page(void) {
    vm_fill_exec_page((uintptr_t)test_exec_page);
}

/* ===================================================================
 * Common helper: set up identity mapping for code execution
 *
 * Same behavior as svade helpers: maps memory at PLATFORM_MEM_BASE with
 * full RWX permissions using 2 MiB megapages, but SKIPS the 2 MiB
 * region(s) containing test pages. Also maps UART.
 * =================================================================== */
static int setup_code_mapping(pt_context_t *ctx) {
    uintptr_t regions[] = {
        (uintptr_t)&__vm_test_region_start,
        (uintptr_t)&__vm_test_region_2m_start,
    };
    return vm_setup_code_mapping(ctx, PAGE_SIZE_2M, regions, 2);
}

/* ===================================================================
 * S-mode test functions (identical to svade helpers)
 *
 * Return 0 on success, scause on trap.
 * =================================================================== */
/* probe_* access payloads are provided by common/test_utils.h. */




static uintptr_t test_smode_amoadd(uintptr_t arg) {
    uint32_t result;
    trap_expect_begin();
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.w %0, %1, (%2)\n\t"
        ".option pop\n\t"
        : "=r"(result)
        : "r"(1), "r"(arg)
        : "memory"
    );
    (void)result;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * PTE inspection / modification helpers are provided by common/vm/vm.h
 * (pte_read / pte_set_bits).
 * =================================================================== */

/* ===================================================================
 * Svadu capability gating
 *
 * Svadu support is a platform capability declared by the build config
 * (config/<platform>/rvtest_config.h: SVADU_SUPPORTED), surfaced as the
 * compile-time SVADU_AVAILABLE macro by common/capabilities.h. Do NOT
 * probe it at runtime by checking menvcfg.ADUE writability or by
 * mapping an A=0 page and observing the hardware A-bit update.
 * =================================================================== */

#endif /* SVADU_TEST_HELPERS_H */
