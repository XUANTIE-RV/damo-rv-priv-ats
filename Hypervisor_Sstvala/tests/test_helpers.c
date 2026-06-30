/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * test_helpers.c - Hypervisor × Sstvala cross test helpers
 *
 * Provides VS-mode payload functions for triggering guest-page-faults,
 * virtual-instruction exceptions, and VS-mode trap handler support
 * for delegated fault tests.
 * =================================================================== */

#include "test_helpers.h"

/* ===================================================================
 * Global variables for VS-mode trap recording (delegated faults)
 * =================================================================== */

uintptr_t g_vs_trap_vstval = 0;
uintptr_t g_vs_trap_cause = 0;
bool      g_vs_trap_triggered = false;

/* ===================================================================
 * VS-mode payload functions (non-delegated faults)
 *
 * These are passed to two_stage_run_in_vs(). The M-mode caller must
 * arm traps via trap_expect_begin() before invocation.
 * =================================================================== */

uintptr_t test_vs_jump_to(uintptr_t arg) {
    /* Jump to arg expecting a guest-page-fault on fetch.
     * Mirrors test_vs_exec_expect_fault pattern with recovery anchor. */
    extern uintptr_t _exec_return_addr;
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        "la   t0, 1f\n\t"
        "la   t1, _exec_return_addr\n\t"
        "sd   t0, 0(t1)\n\t"
        "mv   ra, t0\n\t"
        "jr   %0\n\t"
        "1:\n\t"
        "sd   zero, 0(t1)\n\t"
        ".option pop\n\t"
        :: "r"(arg)
        : "t0", "t1", "ra", "memory"
    );
    return trap_get_cause();
}

uintptr_t test_vs_store_to(uintptr_t arg) {
    volatile uint64_t *p = (volatile uint64_t *)arg;
    *p = 0xDEADBEEF;
    return trap_get_cause();
}

uintptr_t test_vs_amo_to(uintptr_t arg) {
    /* AMOADD.W: atomically add 0x1234 to *(int32_t*)arg */
    uintptr_t result;
    asm volatile (
        "amoadd.w %0, %1, (%2)"
        : "=r"(result)
        : "r"(0x1234), "r"(arg)
        : "memory"
    );
    return trap_get_cause();
}

/* ===================================================================
 * VS-mode payload functions for virtual-instruction stval tests
 * (HCROSS-SSTVALA-06~08)
 *
 * Each payload is a naked function containing a single pre-encoded
 * HS-level CSR access instruction. In VS-mode, these instructions
 * trigger virtual-instruction exception (cause=22) which traps to
 * M-mode. The M-mode handler captures stval (expected to contain
 * the faulting instruction encoding per Sstvala), advances sepc
 * past the faulting instruction, and returns.
 *
 * Pre-encoded instructions:
 *   csrrs x5, 0x600, x0 (hstatus)  = 0x600022F3
 *   csrrw x0, 0x680, x0 (hgatp)    = 0x68001073
 *   csrrs x5, 0x603, x0 (hideleg)  = 0x603022F3
 * =================================================================== */

uintptr_t test_vs_vi_read_hstatus(uintptr_t arg) __attribute__((naked));
uintptr_t test_vs_vi_read_hstatus(uintptr_t arg) {
    (void)arg;
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        ".word  0x600022F3\n\t"   /* csrrs x5, hstatus, x0 */
        "ret\n\t"
        ".option pop\n\t"
    );
}

uintptr_t test_vs_vi_write_hgatp(uintptr_t arg) __attribute__((naked));
uintptr_t test_vs_vi_write_hgatp(uintptr_t arg) {
    (void)arg;
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        ".word  0x68001073\n\t"   /* csrrw x0, hgatp, x0 */
        "ret\n\t"
        ".option pop\n\t"
    );
}

uintptr_t test_vs_vi_read_hideleg(uintptr_t arg) __attribute__((naked));
uintptr_t test_vs_vi_read_hideleg(uintptr_t arg) {
    (void)arg;
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        ".word  0x603022F3\n\t"   /* csrrs x5, hideleg, x0 */
        "ret\n\t"
        ".option pop\n\t"
    );
}

/* ===================================================================
 * VS-mode trap handler for delegated faults
 *
 * This handler is installed at vstvec and runs in VS-mode when a
 * guest-page-fault is delegated via hedeleg. It records vscause and
 * vstval into global variables, then returns to HS-mode via ecall.
 * =================================================================== */

static void vs_trap_handler_sstvala(void) __attribute__((naked));
static void vs_trap_handler_sstvala(void) {
    asm volatile (
        /* Save ra and t0-t2 */
        "addi   sp, sp, -32\n\t"
        "sd     ra, 0(sp)\n\t"
        "sd     t0, 8(sp)\n\t"
        "sd     t1, 16(sp)\n\t"
        "sd     t2, 24(sp)\n\t"

        /* Read vscause -> t0 */
        "csrr   t0, scause\n\t"

        /* Read vstval -> t1 */
        "csrr   t1, stval\n\t"

        /* Store to global variables */
        "la     t2, g_vs_trap_cause\n\t"
        "sd     t0, 0(t2)\n\t"
        "la     t2, g_vs_trap_vstval\n\t"
        "sd     t1, 0(t2)\n\t"
        "la     t2, g_vs_trap_triggered\n\t"
        "li     t0, 1\n\t"
        "sb     t0, 0(t2)\n\t"

        /* Advance sepc past the faulting instruction (assume 4 bytes) */
        "csrr   t0, sepc\n\t"
        "addi   t0, t0, 4\n\t"
        "csrw   sepc, t0\n\t"

        /* Restore registers */
        "ld     ra, 0(sp)\n\t"
        "ld     t0, 8(sp)\n\t"
        "ld     t1, 16(sp)\n\t"
        "ld     t2, 24(sp)\n\t"
        "addi   sp, sp, 32\n\t"

        /* Return from trap */
        "sret\n\t"
    );
}

void setup_vs_trap_handler_for_sstvala(void) {
    /* Clear global trap record */
    g_vs_trap_vstval = 0;
    g_vs_trap_cause = 0;
    g_vs_trap_triggered = false;

    /* Install VS-mode trap handler (Direct mode) */
    vs_trap_setup_direct((uintptr_t)vs_trap_handler_sstvala);
}

/* ===================================================================
 * VS-mode payload functions (delegated faults)
 *
 * These trigger guest-page-faults that are delegated to VS-mode via
 * hedeleg. The VS-mode handler records trap info, then execution
 * continues and returns to HS-mode normally.
 * =================================================================== */

uintptr_t test_vs_jump_delegated(uintptr_t arg) {
    /* Jump to arg. If G-stage unmapped, triggers inst guest-page-fault
     * which is delegated to VS-mode handler. Handler advances sepc
     * past the faulting instruction, so we continue here. */
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        "la     t0, 1f\n\t"
        "mv     ra, t0\n\t"
        "jr     %0\n\t"
        "1:\n\t"
        ".option pop\n\t"
        :: "r"(arg)
        : "t0", "ra", "memory"
    );
    return 0;
}

uintptr_t test_vs_store_delegated(uintptr_t arg) {
    /* Store to arg. If G-stage unmapped, triggers store guest-page-fault
     * which is delegated to VS-mode handler. Handler advances sepc
     * past the faulting instruction, so we continue here. */
    volatile uint64_t *p = (volatile uint64_t *)arg;
    *p = 0xDEADBEEF;
    return 0;
}
