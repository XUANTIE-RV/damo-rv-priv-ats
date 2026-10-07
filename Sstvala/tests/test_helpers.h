/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Sstvala extension test cases
 *
 * Provides shared test data areas, S-mode test functions, and
 * page table setup utilities used by all Sstvala test files.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 *
 * Sstvala semantics (Trap Value Reporting, Version 1.0):
 *   - Address-type exceptions (page-fault, access-fault, misaligned,
 *     non-EBREAK breakpoint): stval = faulting virtual address
 *   - Instruction-type exceptions (illegal-instruction,
 *     virtual-instruction): stval = faulting instruction encoding
 */

#ifndef SSTVALA_TEST_HELPERS_H
#define SSTVALA_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "pmp/pmp_cfg.h"
#include "mem_ops.h"
#include "test_utils.h"

/* SUITE_SATP_MODE (this suite's S-stage paging mode) is set by the suite
 * Makefile: `SUITE_SATP_MODE ?= PLATFORM_SATP_MODE`, overridable with
 * `make SATP_MODE=sv39|sv48|sv57` (see common/Makefile.common). Every
 * pt_init()/satp write below passes SUITE_SATP_MODE. */

/* ===================================================================
 * Test data and executable regions
 *
 * .vm_test_region    : 3 x 4KB pages (data / fault / exec) - 4K tests
 * .vm_test_region_2m : 2 MiB region (for setup_code_mapping skip)
 * =================================================================== */
extern uintptr_t __vm_test_region_start;
extern uintptr_t __vm_test_region_2m_start;

/* 4K test pages (within .vm_test_region) */
#define test_data_area  ((volatile uintptr_t *)&__vm_test_region_start)
#define test_fault_page ((volatile uint8_t *)((uintptr_t)&__vm_test_region_start + PAGE_SIZE_4K))
#define test_exec_page  ((uint8_t *)((uintptr_t)&__vm_test_region_start + 2 * PAGE_SIZE_4K))

/* ===================================================================
 * Delegation helper: delegate Sstvala-relevant exceptions to S-mode
 *
 * Sstvala specifies stval behavior for these exception causes:
 *   cause 0: instruction address misaligned
 *   cause 2: illegal instruction
 *   cause 3: breakpoint
 *   cause 4: load address misaligned
 *   cause 6: store address misaligned
 *
 * Page-fault and access-fault delegation is handled per-test.
 * =================================================================== */
static void sstvala_delegate_exceptions(void) {
    uintptr_t deleg = BIT(CAUSE_INST_ADDR_MISALIGN)   /* cause 0 */
                    | BIT(CAUSE_ILLEGAL_INST)          /* cause 2 */
                    | BIT(CAUSE_BREAKPOINT)            /* cause 3 */
                    | BIT(CAUSE_LOAD_ADDR_MISALIGN)    /* cause 4 */
                    | BIT(CAUSE_STORE_ADDR_MISALIGN);  /* cause 6 */
    CSRW(medeleg, CSRR(medeleg) | deleg);
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
 * Maps memory at PLATFORM_MEM_BASE with full RWX permissions using
 * 2 MiB megapages, but SKIPS the 2 MiB region(s) containing test
 * pages so that individual tests can install their own mappings with
 * custom permissions.
 *
 * Also maps the UART I/O region for S-mode printf support.
 * =================================================================== */
static int setup_code_mapping(pt_context_t *ctx) {
    uintptr_t regions[] = {
        (uintptr_t)&__vm_test_region_start,
        (uintptr_t)&__vm_test_region_2m_start,
    };
    return vm_setup_code_mapping(ctx, PAGE_SIZE_2M, regions, 2);
}

/* ===================================================================
 * S-mode payload functions
 *
 * These functions execute in S-mode with VM enabled. They use
 * trap_expect_begin/end to detect faults. The trap_record (including
 * tval) is captured by the M-mode/S-mode trap handler and accessible
 * after returning to M-mode via trap_get_tval().
 * =================================================================== */

/* probe_load / probe_store / probe_exec are provided by common/test_utils.h. */

/* ensure_smode_pmp is provided by common/pmp/pmp_cfg.h. */

#endif /* SSTVALA_TEST_HELPERS_H */
