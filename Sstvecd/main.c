/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * main.c - Sstvecd Extension Compliance Test entry point
 *
 * See docs/sstvecd_test_plan.md for the full test plan.
 *
 * The Sstvecd extension narrows the WARL behavior of stvec:
 *   - stvec.MODE must be capable of holding 0 (Direct).
 *   - When stvec.MODE=Direct, stvec.BASE must be capable of holding any
 *     valid four-byte-aligned address.
 *
 * Group 1/2 of this suite operate entirely in M-mode by reading/writing
 * stvec via csr instructions; Group 3 enters S-mode through
 * vm_run_in_smode() and uses a local 4-byte-aligned trap entry
 * (sstvecd_trap_entry, defined in tests/sstvecd_strap.S) to record the
 * trap-landing PC and scause. Before any S-mode entry, sscratch must
 * point at the 16-byte scratch region used by the trap entry to spill
 * t0/t1; we initialize sscratch here for safety.
 */

#include "test_framework.h"
#include "vm/vm.h"

/* Linker-provided test table */
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

/* Provided by tests/sstvecd_strap.S */
extern unsigned long sstvecd_trap_scratch[];

int main(void) {
    uart_init();
    reset_state();

    /* Arm sscratch with the address of the asm trap entry's scratch
     * area. The S-mode trap handler in sstvecd_strap.S relies on this
     * scratch region to spill t0/t1. Group 3 test cases also re-arm
     * sscratch inside the S-mode trampoline for safety. */
    asm volatile ("csrw sscratch, %0"
                  :: "r"(sstvecd_trap_scratch) : "memory");

    /* Print banner */
    printf("\n");
    printf("==============================================\n");
    printf("  RISC-V Sstvecd Extension Compliance Test\n");
    printf("==============================================\n");
    printf("  Platform:     %s\n", CONFIG_NAME);
    printf("  XLEN:         %d\n", __riscv_xlen);

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);

    printf("  Test count:   %u\n", test_count);
    printf("==============================================\n\n");

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    return test_print_summary();
}
