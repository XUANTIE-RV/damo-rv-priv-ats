/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * main.c - Ssdbltrp Extension Compliance Test
 */

#include "test_framework.h"

extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

int main(void)
{
    uart_init();
    reset_state();

    /* Re-install Smdbltrp-specific M-mode trap handler.
     * reset_state() resets mtvec to the common handler (m_trap_entry)
     * which does not clear MDT on trap entry. We need the Smdbltrp
     * handler that clears MDT via MRET to prevent double traps.
     */
    extern void smdbltrp_m_trap_entry(void);
    CSRW(mtvec, (uintptr_t)&smdbltrp_m_trap_entry);

    /* Install the Ssdbltrp S-mode trap handler: wraps the common
     * s_trap_handler and captures mtval2 at S-mode trap entry so
     * tests can verify S-mode trap delivery leaves mtval2 unchanged. */
    extern void ssdbltrp_s_trap_entry(void);
    CSRW(stvec, (uintptr_t)&ssdbltrp_s_trap_entry);

    /* Configure PMP: allow S/U-mode full access */
    asm volatile(
        "li t0, -1\n\t"
        "csrw pmpaddr0, t0\n\t"
        "li t0, 0x1F\n\t"
        "csrw pmpcfg0, t0\n\t"
        ::: "t0"
    );

    test_print_banner("RISC-V Ssdbltrp Extension Compliance Test");

    /* Environment probe: report mstatus.MDT state.  On platforms that
     * apply Smdbltrp semantics (MDT set at reset / on M-mode trap
     * entry), any M-mode exception while MDT=1 is an unrecoverable
     * critical error without Smrnmi, so tests must keep MDT cleared.
     */
    {
        uintptr_t ms0 = CSRR(mstatus);
        clear_mdt();
        uintptr_t ms1 = CSRR(mstatus);
        printf("[ENV] mstatus=0x%lx MDT@reset=%d MDT@after_clear=%d\n",
               (unsigned long)ms0,
               (int)(((unsigned long)ms0 >> 42) & 1UL),
               (int)(((unsigned long)ms1 >> 42) & 1UL));
    }

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);


    for (unsigned int i = 0; i < test_count; i++) {
        /* Re-install Smdbltrp trap handler before each test.
         * reset_state() (called by TEST_END) resets mtvec to the default
         * handler which does not clear MDT, causing double traps.
         */
        extern void smdbltrp_m_trap_entry(void);
        CSRW(mtvec, (uintptr_t)&smdbltrp_m_trap_entry);

        /* Re-install the mtval2-capturing S-mode handler too, since
         * reset_state() resets stvec to the default s_trap_entry. */
        extern void ssdbltrp_s_trap_entry(void);
        CSRW(stvec, (uintptr_t)&ssdbltrp_s_trap_entry);

        _test_table[i]();
    }

    return test_print_summary();
}
