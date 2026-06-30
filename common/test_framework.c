/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"

/* ===================================================================
 * Test result tracking (global)
 * =================================================================== */
test_result_t test_results = {
    .total = 0,
    .passed = 0,
    .failed = 0,
    .skipped = 0,
    .tests_passed = 0,
    .tests_failed = 0,
    .current_test_name = NULL,
    .current_test_failed = false,
    .failed_names = {0},
    .failed_count = 0,
    .skipped_names = {0},
    .skipped_reasons = {0},
    .skipped_count = 0,
};

/* ===================================================================
 * reset_state - Reset common test state to clean baseline
 *
 * Only performs common resets. Extension-specific resets
 * (e.g., pmp_reset, smepmp_reset) should be called separately.
 * =================================================================== */
void reset_state(void) {
    /* Ensure we're in M-mode */
    if (get_current_priv() != PRIV_M)
        goto_priv(PRIV_M);

    /* Reset trap state */
    trap_expect_end();

    /* Set up trap vectors */
    extern void m_trap_entry(void);
    extern void s_trap_entry(void);
    CSRW(mtvec, (uintptr_t)m_trap_entry);
    CSRW(stvec, (uintptr_t)s_trap_entry);

    /* Do NOT delegate exceptions (M-mode handler catches everything) */
    CSRW(medeleg, 0);
    CSRW(mideleg, 0);

    /* Ensure interrupts are disabled */
    CSRC(mstatus, MSTATUS_MIE_BIT | MSTATUS_SIE_BIT);

    /* Clear MPRV to avoid M-mode using translated addresses */
    CSRC(mstatus, MSTATUS_MPRV_BIT);

    /* Clear MXR to avoid PM interaction side effects */
    CSRC(mstatus, MSTATUS_MXR_BIT);

    /* Clear Pointer Masking (PM) state.
     * senvcfg.PMM [33:32], menvcfg.PMM [33:32], mseccfg.PMM [33:32].
     * Each access is trap-protected in case the CSR does not exist. */
    {
        /* Clear senvcfg.PMM (Ssnpm: U-mode PM) */
        trap_expect_begin();
        uintptr_t senvcfg_val = CSRR(CSR_SENVCFG);
        if (!trap_was_triggered()) {
            CSRW(CSR_SENVCFG, senvcfg_val & ~SENVCFG_PMM_MASK);
        }
        trap_expect_end();

        /* Clear menvcfg.PMM (Smnpm: S-mode PM) */
        trap_expect_begin();
        uintptr_t menvcfg_val = CSRR(CSR_MENVCFG);
        if (!trap_was_triggered()) {
            CSRW(CSR_MENVCFG, menvcfg_val & ~MENVCFG_PMM_MASK);
        }
        trap_expect_end();

        /* Clear mseccfg.PMM (Smmpm: M-mode PM).
         * Note: mseccfg.PMM may be sticky on some implementations;
         * the write may silently fail, which is acceptable. */
        trap_expect_begin();
        uintptr_t mseccfg_val = CSRR(CSR_MSECCFG);
        if (!trap_was_triggered()) {
            CSRW(CSR_MSECCFG, mseccfg_val & ~MSECCFG_PMM_MASK);
        }
        trap_expect_end();
    }
}

/* ===================================================================
 * test_print_summary - Print final test results
 * =================================================================== */
int test_print_summary(void) {
    unsigned int tests_total = test_results.tests_passed +
                               test_results.tests_failed +
                               test_results.skipped;

    printf("\n========================================\n");
    printf("  Test Summary\n");
    printf("========================================\n");
    printf("  Total:   %u\n", tests_total);
    printf("  Passed:  %u\n", test_results.tests_passed);
    printf("  Failed:  %u\n", test_results.tests_failed);
    printf("  Skipped: %u\n", test_results.skipped);
    printf("----------------------------------------\n");
    printf("  Assertions: %u total, %u passed, %u failed\n",
           test_results.total, test_results.passed, test_results.failed);

    if (test_results.skipped > 0) {
        printf("----------------------------------------\n");
        printf("  Skipped tests:\n");
        for (unsigned int i = 0; i < test_results.skipped_count; i++) {
            printf("    %u) %s: %s\n", i + 1,
                   test_results.skipped_names[i],
                   test_results.skipped_reasons[i]);
        }
        if (test_results.skipped > test_results.skipped_count) {
            printf("    ... and %u more\n",
                   test_results.skipped - test_results.skipped_count);
        }
    }

    printf("========================================\n");
    if (test_results.tests_failed == 0 && test_results.skipped == 0) {
        printf("  RESULT: ALL PASSED\n");
    } else if (test_results.tests_failed == 0) {
        printf("  RESULT: ALL PASSED (%u tests skipped)\n",
               test_results.skipped);
    } else {
        printf("  RESULT: %u FAILURES\n", test_results.tests_failed);
        printf("----------------------------------------\n");
        printf("  Failed tests:\n");
        for (unsigned int i = 0; i < test_results.failed_count; i++) {
            printf("    %u) %s\n", i + 1, test_results.failed_names[i]);
        }
        if (test_results.tests_failed > test_results.failed_count) {
            printf("    ... and %u more\n",
                   test_results.tests_failed - test_results.failed_count);
        }
    }

    printf("========================================\n\n");

    return (int)test_results.tests_failed;
}
