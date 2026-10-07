/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Svinval test cases
 *
 * Provides shared test data areas, S-mode test functions, and
 * utility functions used by all Svinval test files.
 *
 * Design: All test files are #included into test_register.c,
 * so static functions and variables are visible across all tests
 * within the same compilation unit.
 */

#ifndef SVINVAL_TEST_HELPERS_H
#define SVINVAL_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "pmp_cfg.h"
#include "mem_ops.h"
#include "test_utils.h"
#include "svinval_insn.h"

/* SUITE_SATP_MODE (this suite's S-stage paging mode) is set by the suite
 * Makefile: `SUITE_SATP_MODE ?= PLATFORM_SATP_MODE`, overridable with
 * `make SATP_MODE=sv39|sv48|sv57` (see common/Makefile.common). Every
 * pt_init()/satp write below passes SUITE_SATP_MODE. */

/* ===================================================================
 * Test data and executable regions
 *
 * These pages are placed in a separate 2MB-aligned region by the
 * linker script (.vm_test_region section). This ensures they are
 * NOT covered by setup_code_mapping()'s 2MB megapages, allowing
 * individual tests to create 4KB mappings with custom permissions.
 * =================================================================== */
extern uintptr_t __vm_test_region_start;

/* Page 0: test data area for read/write verification */
#define test_data_area  ((volatile uintptr_t *)&__vm_test_region_start)

/* Page 1: test page for permission/fault tests */
#define test_fault_page ((volatile uint8_t *)((uintptr_t)&__vm_test_region_start + PAGE_SIZE_4K))

/* Page 2: test page for additional tests */
#define test_extra_page ((volatile uint8_t *)((uintptr_t)&__vm_test_region_start + 2 * PAGE_SIZE_4K))

/* MAGIC_WRITE / MAGIC_READ are provided by common/vm/vm_defs.h. */

/* ===================================================================
 * Common helper: set up identity mapping for code execution
 *
 * Maps memory at MEM_BASE with full RWX permissions using 2MB
 * megapages, but SKIPS the 2MB region containing the test pages
 * (.vm_test_region). This allows individual tests to create 4KB
 * mappings in that region with custom permissions.
 *
 * Also maps the UART I/O region for S-mode printf support.
 * =================================================================== */
static int setup_code_mapping(pt_context_t *ctx) {
    uintptr_t regions[] = { (uintptr_t)&__vm_test_region_start };
    return vm_setup_code_mapping(ctx, PAGE_SIZE_2M, regions, 1);
}

/* ===================================================================
 * S-mode test functions
 *
 * These functions execute in S-mode with VM enabled via
 * vm_run_in_smode(). They use trap_expect_begin/end to detect faults.
 * =================================================================== */

/* probe_load / probe_store are provided by common/test_utils.h; the former
 * smode_load_expect_fault / smode_store_expect_fault were byte-identical to
 * them (the "expect fault" intent lives at the call site) and are collapsed
 * into probe_load / probe_store. */

/**
 * smode_read_write - Write magic value and read back to verify.
 * Returns 0 on success, 1 on mismatch, or scause on fault.
 */
static uintptr_t smode_read_write(uintptr_t addr) {
    trap_expect_begin();
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = MAGIC_WRITE;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();

    trap_expect_begin();
    uintptr_t val = *ptr;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();

    if (val != MAGIC_WRITE)
        return 1;
    return 0;
}

/* ===================================================================
 * Global variables for S-mode PTE manipulation
 *
 * Used by tests that need to modify PTEs from within S-mode.
 * =================================================================== */
static volatile uintptr_t *g_pte_addr __attribute__((unused));
static volatile uintptr_t g_test_va __attribute__((unused));

#endif /* SVINVAL_TEST_HELPERS_H */
