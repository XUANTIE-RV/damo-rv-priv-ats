#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

/*
 * Ssdbltrp test helpers - integrates with common test framework
 * and provides Ssdbltrp-specific helpers.
 */

#include "../../common/test_framework.h"

/* ===================================================================
 * SDT / SIE / DTE bit definitions
 * =================================================================== */

/* sstatus.SDT bit position (bit 24) */
#define SSTATUS_SDT_BIT (1ULL << 24)

/* sstatus.SIE bit position (bit 1) */
#define SSTATUS_SIE_BIT (1ULL << 1)

/* sstatus.SPIE bit position (bit 5) */
#define SSTATUS_SPIE_BIT (1ULL << 5)

/* menvcfg.DTE bit position (bit 59) */
#define MENVCFG_DTE_BIT (1ULL << 59)

/* medeleg bit 16 (double trap) */
#define MEDELEG_DOUBLE_TRAP (1ULL << 16)

/* ===================================================================
 * SDT / SIE helper functions
 * =================================================================== */

static inline bool get_sdt(void) {
    uint64_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    return (sstatus & SSTATUS_SDT_BIT) != 0;
}

static inline void set_sdt(void) {
    asm volatile("csrs sstatus, %0" :: "r"(SSTATUS_SDT_BIT));
}

static inline void clear_sdt(void) {
    asm volatile("csrc sstatus, %0" :: "r"(SSTATUS_SDT_BIT));
}

static inline bool get_sie(void) {
    uint64_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    return (sstatus & SSTATUS_SIE_BIT) != 0;
}

static inline void set_sie(void) {
    asm volatile("csrs sstatus, %0" :: "r"(SSTATUS_SIE_BIT));
}

static inline void clear_sie(void) {
    asm volatile("csrc sstatus, %0" :: "r"(SSTATUS_SIE_BIT));
}

static inline void execute_sret(void) {
    asm volatile("sret");
}

/* ===================================================================
 * Ssdbltrp-specific TEST_END / TEST_SKIP
 *
 * Clear MDT before framework's TEST_END/TEST_SKIP to prevent
 * double-trap when reset_state() writes CSRs (mtvec, stvec, etc).
 * =================================================================== */

#define TEST_END() do { \
    clear_mdt(); \
    goto_priv(PRIV_M); \
    reset_state(); \
    _test_end_record(); \
    return !test_results.current_test_failed; \
} while (0)

#define TEST_SKIP(reason) do { \
    clear_mdt(); \
    test_results.skipped++; \
    if (test_results.skipped_count < MAX_SKIPPED_TESTS) { \
        test_results.skipped_names[test_results.skipped_count] = \
            test_results.current_test_name; \
        test_results.skipped_reasons[test_results.skipped_count] = (reason); \
        test_results.skipped_count++; \
    } \
    printf("[SKIP] %s: %s\n\n", test_results.current_test_name, (reason)); \
    goto_priv(PRIV_M); \
    reset_state(); \
    return true; \
} while (0)

/* ===================================================================
 * DTE (Double Trap Enable) helper functions
 * =================================================================== */

static inline void set_dte(void) {
    asm volatile("csrs menvcfg, %0" :: "r"(MENVCFG_DTE_BIT));
}

static inline void clear_dte(void) {
    asm volatile("csrc menvcfg, %0" :: "r"(MENVCFG_DTE_BIT));
}

static inline bool get_dte(void) {
    uint64_t menvcfg;
    asm volatile("csrr %0, menvcfg" : "=r"(menvcfg));
    return (menvcfg & MENVCFG_DTE_BIT) != 0;
}

/* ===================================================================
 * Extension detection helpers
 * =================================================================== */

/*
 * Check if Ssdbltrp is implemented.
 *
 * Per spec, sstatus.SDT is writable only when menvcfg.DTE=1.
 * When DTE=0, SDT reads as read-only zero.
 *
 * Detection strategy:
 * 1. Save current menvcfg
 * 2. Clear MDT (safety: mstatus access requires MDT=0 on Smdbltrp)
 * 3. Set menvcfg.DTE=1 (enables SDT writability)
 * 4. Try to set sstatus.SDT and check if it sticks
 * 5. Restore original state
 */
static inline bool check_ssdbltrp(void) {
    /* Clear MDT first - required for safe mstatus access */
    clear_mdt();

    /* Save menvcfg and set DTE=1 */
    uint64_t menvcfg_orig = CSRR(menvcfg);
    CSRS(menvcfg, MENVCFG_DTE_BIT);

    /* Save sstatus and try to set SDT */
    uint64_t sstatus_orig = CSRR(sstatus);
    set_sdt();
    bool sdt_stuck = get_sdt();
    clear_sdt();

    /* Restore menvcfg (DTE bit) */
    CSRW(menvcfg, menvcfg_orig);

    return sdt_stuck;
}

#endif /* TEST_HELPERS_H */
