/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the single-stage Sv39/Sv48/Sv57
 *                  virtual-memory test suites.
 *
 * Provides shared test data areas, S-mode test functions, utility
 * functions, and all mode-derived test identity macros used by every
 * test file.
 *
 * Shared-source layout: Sv39/ is the master directory holding the real
 * sources; Sv48/ and Sv57/ reuse every file under tests/ (plus main.c
 * and kernel.ld) via symlinks and select the paging mode at compile
 * time through -DSUITE_SATP_MODE in their own Makefile -- mirroring the
 * Sv39x4/Sv48x4/Sv57x4 G-stage suites.
 *
 * Design: All test files are #included into test_register.c,
 * so static functions and variables are visible across all tests
 * within the same compilation unit.
 */

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "test_utils.h"
#include "mem_ops.h"

/* Suite S-stage paging mode. Each of the Sv39/Sv48/Sv57 directories
 * fixes its mode with a bare -DSUITE_SATP_MODE=SATP_MODE_SV39/48/57 in
 * its own Makefile (mirroring the Sv*x4 G-stage suites, whose per-dir
 * Makefile -D is appended after the common hook and therefore wins).
 * The fallback below only applies when the shared sources are compiled
 * without that -D (e.g. ad-hoc preprocessing), defaulting to the master
 * mode Sv39. Every working-mode pt_init()/satp write passes
 * SUITE_SATP_MODE; the reserved-MODE enumeration keeps its explicit
 * constants. */
#ifndef SUITE_SATP_MODE
#define SUITE_SATP_MODE   SATP_MODE_SV39
#endif

/* ===================================================================
 * Mode-derived test identity (SUITE_SATP_MODE)
 *
 * All per-mode differences are derived here from SUITE_SATP_MODE so the
 * test bodies under tests/ stay byte-identical across Sv39/Sv48/Sv57:
 *   - report mode name and test-function name prefix
 *   - test IDs from the unified vm_test_plan.md ID space
 *     (MAP-01/05/08, SIGN-03/05/07, WALK-01/02/03, SATP-02/03/04, ...)
 *   - non-canonical VA boundary, root page-table level and its VPN
 *     accessor, page-table walk depth, and the mode-switch target.
 * =================================================================== */
#if SUITE_SATP_MODE == SATP_MODE_SV39
#define SUITE_MODE_NAME        "Sv39"
#define SV_FN_BASE             test_sv39_
#define ID_MAP_1G              "MAP-01"
#define ID_MAP_2M              "MAP-02"
#define ID_MAP_4K              "MAP-03"
#define ID_SIGN_NONCANON       "SIGN-03"
#define ID_WALK_FULL           "WALK-01"
#define ID_SATP_ENABLE         "SATP-02"
#define ID_SATP_SWITCH         "SATP-05"
#define SFX_MAP_1G             map01_1g
#define SFX_MAP_2M             map02_2m
#define SFX_MAP_4K             map03_4k
#define SFX_SIGN_NONCANON      sign03
#define SFX_WALK_FULL          walk01
#define SFX_SATP_ENABLE        satp02
#define SFX_SATP_SWITCH        satp05
#define SUITE_NONCANON_VA      0x0000004000000000UL
#define SUITE_WALK_DEPTH       "three-level"
#define SUITE_ROOT_LEVEL_STR   "L2"
#define SUITE_ROOT_VPN(va)     VA_VPN2(va)
#define SUITE_NEXT_MODE        SATP_MODE_SV48
#define SUITE_NEXT_MODE_NAME   "Sv48"
#elif SUITE_SATP_MODE == SATP_MODE_SV48
#define SUITE_MODE_NAME        "Sv48"
#define SV_FN_BASE             test_sv48_
#define ID_MAP_1G              "MAP-05"
#define ID_MAP_2M              "MAP-06"
#define ID_MAP_4K              "MAP-07"
#define ID_SIGN_NONCANON       "SIGN-05"
#define ID_WALK_FULL           "WALK-02"
#define ID_SATP_ENABLE         "SATP-03"
#define ID_SATP_SWITCH         "SATP-06"
#define SFX_MAP_1G             map05_1g
#define SFX_MAP_2M             map06_2m
#define SFX_MAP_4K             map07_4k
#define SFX_SIGN_NONCANON      sign05
#define SFX_WALK_FULL          walk02
#define SFX_SATP_ENABLE        satp03
#define SFX_SATP_SWITCH        satp06
#define SUITE_NONCANON_VA      0x0000800000000000UL
#define SUITE_WALK_DEPTH       "four-level"
#define SUITE_ROOT_LEVEL_STR   "L3"
#define SUITE_ROOT_VPN(va)     VA_VPN3(va)
#define SUITE_NEXT_MODE        SATP_MODE_SV57
#define SUITE_NEXT_MODE_NAME   "Sv57"
#elif SUITE_SATP_MODE == SATP_MODE_SV57
#define SUITE_MODE_NAME        "Sv57"
#define SV_FN_BASE             test_sv57_
#define ID_MAP_1G              "MAP-08"
#define ID_MAP_2M              "MAP-09"
#define ID_MAP_4K              "MAP-10"
#define ID_SIGN_NONCANON       "SIGN-07"
#define ID_WALK_FULL           "WALK-03"
#define ID_SATP_ENABLE         "SATP-04"
#define SFX_MAP_1G             map08_1g
#define SFX_MAP_2M             map09_2m
#define SFX_MAP_4K             map10_4k
#define SFX_SIGN_NONCANON      sign07
#define SFX_WALK_FULL          walk03
#define SFX_SATP_ENABLE        satp04
#define SUITE_NONCANON_VA      0x0100000000000000UL
#define SUITE_WALK_DEPTH       "five-level"
#define SUITE_ROOT_LEVEL_STR   "L4"
#define SUITE_ROOT_VPN(va)     VA_VPN4(va)
/* Sv57 is the highest mode: there is no higher mode to switch to, so the
 * mode-switch test is compiled out (see test_satp.c). ID_SATP_SWITCH,
 * SFX_SATP_SWITCH, SUITE_NEXT_MODE and SUITE_NEXT_MODE_NAME are
 * intentionally left undefined here. */
#else
#error "SUITE_SATP_MODE must be SATP_MODE_SV39, SATP_MODE_SV48 or SATP_MODE_SV57"
#endif

/* Build a per-mode test function name: SVFN(ad01) -> test_sv39_ad01.
 * SV_FN_BASE already ends with '_', so a single token paste suffices;
 * the two-level SV_CAT lets SVFN's argument expand before pasting. */
#define SV_CAT_(a, b)  a##b
#define SV_CAT(a, b)   SV_CAT_(a, b)
#define SVFN(name)     SV_CAT(SV_FN_BASE, name)

/* SV_REGISTER - per-mode equivalent of TEST_REGISTER.
 *
 * TEST_REGISTER(fn) pastes fn##_ptr, and the ## operator suppresses
 * expansion of its argument, so a function-like SVFN(x) cannot be
 * passed to it directly. SV_REGISTER does the paste itself and expands
 * to exactly what TEST_REGISTER(test_sv39_x) would: a forward
 * declaration plus the .test_table function pointer. */
#define SV_REGISTER(name)                                       \
    bool SVFN(name)(void);                                      \
    static test_func_t SVFN(name##_ptr)                         \
        __attribute__((section(".test_table"), used)) = SVFN(name)

/* Compile-time platform-capability gate, mirroring REQUIRE_SATP_MODE in
 * common/hyp/hyp_test.h. The single-stage Sv39/48/57 suites do not pull
 * in the hypervisor headers, so the gate is defined locally; it depends
 * only on SV39/48/57_AVAILABLE (force-included from capabilities.h) and
 * TEST_SKIP (test_framework.h). A test whose required paging mode is not
 * declared by the platform config is SKIPPED -- never runtime-probed. */
#ifndef REQUIRE_SATP_MODE
#define SV_PAGING_MODE_UNAVAILABLE_(mode) (                     \
    ((mode) == SATP_MODE_BARE)                          ||      \
    ((mode) == SATP_MODE_SV39 && !SV39_AVAILABLE)       ||      \
    ((mode) == SATP_MODE_SV48 && !SV48_AVAILABLE)       ||      \
    ((mode) == SATP_MODE_SV57 && !SV57_AVAILABLE))
#define REQUIRE_SATP_MODE(mode) do {                            \
    if (SV_PAGING_MODE_UNAVAILABLE_(mode)) {                    \
        TEST_SKIP("satp mode not declared by platform config"); \
    }                                                           \
} while (0)
#endif

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

/* Page 2: test page filled with executable instructions (nop; ret) */
#define test_exec_page  ((uint8_t *)((uintptr_t)&__vm_test_region_start + 2 * PAGE_SIZE_4K))

/* MAGIC_WRITE / MAGIC_READ are provided by common/vm/vm_defs.h. */

/* ===================================================================
 * Initialization helper: fill exec page with nop;ret
 * =================================================================== */
static void init_exec_page(void) {
    vm_fill_exec_page((uintptr_t)test_exec_page);
}

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
 * These functions execute in S-mode with VM enabled.
 * They use trap_expect_begin/end to detect faults.
 * =================================================================== */

/**
 * test_smode_read_write - Basic read/write test in S-mode
 * Returns 0 on success, non-zero on failure.
 */
static uintptr_t test_smode_read_write(uintptr_t arg) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)arg;

    ptr[0] = MAGIC_WRITE;
    if (ptr[0] != MAGIC_WRITE)
        return 1;

    ptr[1] = MAGIC_READ;
    if (ptr[1] != MAGIC_READ)
        return 2;

    for (int i = 0; i < 4; i++)
        ptr[i] = (uintptr_t)i * 0x1111111111111111UL;

    for (int i = 0; i < 4; i++) {
        if (ptr[i] != (uintptr_t)i * 0x1111111111111111UL)
            return 3;
    }

    return 0;
}

/* probe_* access payloads are provided by common/test_utils.h. */




/* ===================================================================
 * S-mode test functions with SUM/MXR control
 * =================================================================== */

/**
 * test_smode_load_with_sum - Load with SUM=1
 */
static uintptr_t test_smode_load_with_sum(uintptr_t arg) {
    CSRS(sstatus, MSTATUS_SUM_BIT);
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    CSRC(sstatus, MSTATUS_SUM_BIT);
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_store_with_sum - Store with SUM=1
 */
static uintptr_t test_smode_store_with_sum(uintptr_t arg) {
    CSRS(sstatus, MSTATUS_SUM_BIT);
    trap_expect_begin();
    *(volatile uintptr_t *)arg = MAGIC_WRITE;
    trap_expect_end();
    CSRC(sstatus, MSTATUS_SUM_BIT);
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_exec_with_sum - Execute with SUM=1
 *
 * Uses exec_at() for safe trap recovery (see probe_exec).
 */
static uintptr_t test_smode_exec_with_sum(uintptr_t arg) {
    CSRS(sstatus, MSTATUS_SUM_BIT);
    trap_expect_begin();
    exec_at(arg);
    trap_expect_end();
    CSRC(sstatus, MSTATUS_SUM_BIT);
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/**
 * test_smode_load_with_mxr - Load with MXR=1
 */
static uintptr_t test_smode_load_with_mxr(uintptr_t arg) {
    CSRS(sstatus, MSTATUS_MXR_BIT);
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    CSRC(sstatus, MSTATUS_MXR_BIT);
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

#endif /* TEST_HELPERS_H */
