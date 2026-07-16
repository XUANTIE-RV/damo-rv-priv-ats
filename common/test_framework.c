/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"

/* Runtime CSR access (defined in csr_accessors.c) */
extern uintptr_t csr_read(uint16_t csr);
extern void csr_write(uint16_t csr, uintptr_t val);

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

#ifdef ENABLE_PM
/* ===================================================================
 * _safe_csr_clear_field - trap-protected CSR field clearing
 *
 * Reads a CSR by runtime address, clears the bits specified by
 * @mask, and writes back.  The read is trap-protected so that
 * accessing a non-existent CSR is safe (the write is skipped).
 * Reusable for any extension that needs conditional CSR cleanup
 * in reset paths.
 * =================================================================== */
static void _safe_csr_clear_field(uint16_t csr, uintptr_t mask) {
    trap_expect_begin();
    uintptr_t val = csr_read(csr);
    if (!trap_was_triggered()) {
        csr_write(csr, val & ~mask);
    }
    trap_expect_end();
}
#endif /* ENABLE_PM */

void reset_state(void) {
    /* Ensure we're in M-mode */
    if (get_current_priv() != PRIV_M)
        goto_priv(PRIV_M);

    /* Reset trap state — clear ALL fields, not just armed.
     * This prevents stale trap_was_triggered()/trap_get_cause() results
     * from a previous test leaking into the next one. */
    trap_clear_record();

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

    /* Clear interrupt enable registers to prevent residual enable bits
     * from triggering unexpected traps when a later test re-enables MIE.
     * mstatus.MIE/SIE only gates the global interrupt; individual enables
     * in mie/sie persist across tests and can fire immediately if mip
     * has pending bits (e.g., after mret restores MIE=MPIE). */
    CSRW(mie, 0);
    CSRW(sie, 0);

    /* Clear software-writable pending interrupt bits in mip.
     * Read-only bits (MTIP, MEIP, STIP) are unaffected by writes.
     * Writable bits: SSIP (1), MSIP (3), SEIP (9), LCOFIP (13). */
    CSRW(mip, 0);

    /* Clear mstatus bits that could affect subsequent tests:
     * - MPRV (bit 17): M-mode using translated addresses
     * - SUM  (bit 18): S-mode accessing U=1 pages without explicit intent
     * - MXR  (bit 19): may alter load access permissions
     * - TVM  (bit 20): S-mode satp/hgatp access restriction
     * - TSR  (bit 22): S-mode sret restriction */
    CSRC(mstatus, MSTATUS_MPRV_BIT | MSTATUS_SUM_BIT |
                  MSTATUS_MXR_BIT  | MSTATUS_TVM_BIT |
                  MSTATUS_TSR_BIT);

    /* Clear Smdbltrp MDT if supported (no-op otherwise).
     * MDT is set by hardware on M-mode trap entry; if left set,
     * the next EXPECT_TRAP in M-mode triggers a fatal double trap. */
    clear_mdt();

#ifdef ENABLE_HYP
    /* Clear mstatus.MPV to prevent stale virtualization state.
     * If a previous test entered VS/VU mode and left MPV=1,
     * subsequent mret could unintentionally enter V=1 mode. */
    CSRC(mstatus, MSTATUS_MPV);
#endif

    /* Disable address translation (satp = bare mode).
     * Prevents a previous test's page table from remaining active
     * when a subsequent test enters S-mode. */
    CSRW(satp, 0);

#ifdef ENABLE_PM
    /* Clear Pointer Masking (PM) state via the generic helper.
     * senvcfg.PMM [33:32] (Ssnpm), menvcfg.PMM [33:32] (Smnpm),
     * mseccfg.PMM [33:32] (Smmpm).
     * Only needed when PM extension is enabled. */
    _safe_csr_clear_field(CSR_SENVCFG, SENVCFG_PMM_MASK);
    _safe_csr_clear_field(CSR_MENVCFG, MENVCFG_PMM_MASK);
    /* mseccfg.PMM may be sticky on some implementations;
     * the write may silently fail, which is acceptable. */
    _safe_csr_clear_field(CSR_MSECCFG, MSECCFG_PMM_MASK);
#endif /* ENABLE_PM */
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
