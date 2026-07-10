/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_TEST_FRAMEWORK_H
#define COMMON_TEST_FRAMEWORK_H

#include "types.h"
#include "encoding.h"
#include "uart.h"
#include "mem_ops.h"

/* ===================================================================
 * Test Function Type and Registration
 * =================================================================== */

typedef bool (*test_func_t)(void);

/**
 * TEST_REGISTER - Register a test function for automatic execution
 *
 * Usage:
 *   TEST_REGISTER(test_pmp_napot_rwx);
 *   bool test_pmp_napot_rwx(void) {
 *       TEST_BEGIN("PMP NAPOT RWX");
 *       // ... test logic ...
 *       TEST_END();
 *   }
 *
 * The test function pointer is placed in the .test_table section,
 * which is collected by the linker script.
 */
#define TEST_REGISTER(test_fn) \
    bool test_fn(void); \
    static test_func_t test_fn##_ptr \
        __attribute__((section(".test_table"), used)) = test_fn

/* ===================================================================
 * Test Result Tracking
 * =================================================================== */

#define MAX_FAILED_TESTS  16
#define MAX_SKIPPED_TESTS 16

typedef struct {
    unsigned int total;
    unsigned int passed;
    unsigned int failed;
    unsigned int skipped;
    unsigned int tests_passed;
    unsigned int tests_failed;
    const char  *current_test_name;
    bool         current_test_failed;
    const char  *failed_names[MAX_FAILED_TESTS];
    unsigned int failed_count;
    const char  *skipped_names[MAX_SKIPPED_TESTS];
    const char  *skipped_reasons[MAX_SKIPPED_TESTS];
    unsigned int skipped_count;
} test_result_t;

extern test_result_t test_results;

/* ===================================================================
 * Test Lifecycle Macros
 * =================================================================== */

/* Simple substring search for bare-metal filter (no libc) */
static inline const char *_tf_strstr(const char *h, const char *n) {
    if (!*n) return h;
    for (; *h; h++) {
        const char *hp = h, *np = n;
        while (*hp && *np && *hp == *np) { hp++; np++; }
        if (!*np) return h;
    }
    return (const char *)0;
}

/**
 * TEST_BEGIN - Start a test case
 *
 * Initializes per-test state and prints the test name.
 * When TEST_FILTER is defined, tests whose name does not contain
 * the filter string are silently skipped (return true).
 */
#define TEST_BEGIN(name) do { \
    test_results.current_test_name = (name); \
    test_results.current_test_failed = false; \
    TEST_FILTER_CHECK(name); \
    printf("[TEST] %s\n", (name)); \
} while (0)

#ifdef TEST_FILTER
#define TEST_FILTER_CHECK(name) do { \
    if (!_tf_strstr((name), TEST_FILTER)) { \
        return true; \
    } \
} while (0)
#else
#define TEST_FILTER_CHECK(name) ((void)0)
#endif

/**
 * _test_end_record - Common test-end bookkeeping (statistics + print)
 *
 * Shared by TEST_END() and HYP_TEST_END() to avoid duplicating the
 * pass/fail tracking logic. Each caller is responsible for invoking
 * its own reset function before calling this.
 *
 * Returns the pass/fail result (true = passed).
 */
static inline bool _test_end_record(void) {
    if (test_results.current_test_failed) {
        test_results.tests_failed++;
        if (test_results.failed_count < MAX_FAILED_TESTS) {
            test_results.failed_names[test_results.failed_count++] =
                test_results.current_test_name;
        }
        printf("[FAIL] %s\n\n", test_results.current_test_name);
    } else {
        test_results.tests_passed++;
        printf("[PASS] %s\n\n", test_results.current_test_name);
    }
    return !test_results.current_test_failed;
}

/**
 * TEST_END - End a test case
 *
 * Restores M-mode, clears PMP state, prints PASS/FAIL.
 * Returns true if all assertions passed.
 */
#define TEST_END() do { \
    goto_priv(PRIV_M); \
    reset_state(); \
    return _test_end_record(); \
} while (0)

/**
 * TEST_SKIP - Skip a test case with a reason
 *
 * Increments the skipped counter, prints [SKIP], restores state,
 * and returns true (skip is not a failure).
 */
#define TEST_SKIP(reason) do { \
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

/**
 * TEST_FATAL - Unconditionally fail the current test with a reason
 *
 * Use when a code path that should be unreachable is reached,
 * or to explicitly mark a test as failed with a descriptive message.
 */
#define TEST_FATAL(reason) do { \
    test_results.total++; \
    test_results.failed++; \
    test_results.current_test_failed = true; \
    printf("[FATAL] %s: %s\n\n", test_results.current_test_name, (reason)); \
    goto_priv(PRIV_M); \
    reset_state(); \
    return false; \
} while (0)

/* ===================================================================
 * Assertion Macros
 * =================================================================== */

/**
 * TEST_ASSERT - Check a boolean condition
 */
#define TEST_ASSERT(msg, cond) do { \
    test_results.total++; \
    if (cond) { \
        test_results.passed++; \
    } else { \
        test_results.failed++; \
        test_results.current_test_failed = true; \
        printf("  ASSERT FAIL: %s (%s:%d)\n", (msg), __FILE__, __LINE__); \
    } \
} while (0)

/**
 * TEST_ASSERT_EQ - Check equality of two values
 */
#define TEST_ASSERT_EQ(msg, actual, expected) do { \
    test_results.total++; \
    if ((uintptr_t)(actual) == (uintptr_t)(expected)) { \
        test_results.passed++; \
    } else { \
        test_results.failed++; \
        test_results.current_test_failed = true; \
        printf("  ASSERT FAIL: %s: got 0x%lx, expected 0x%lx (%s:%d)\n", \
               (msg), (unsigned long)(actual), (unsigned long)(expected), \
               __FILE__, __LINE__); \
    } \
} while (0)

/**
 * TEST_ASSERT_NEQ - Check inequality of two values
 *
 * Asserts that actual != not_expected. On failure, prints the value
 * that should NOT have been equal.
 */
#define TEST_ASSERT_NEQ(msg, actual, not_expected) do { \
    test_results.total++; \
    if ((uintptr_t)(actual) != (uintptr_t)(not_expected)) { \
        test_results.passed++; \
    } else { \
        test_results.failed++; \
        test_results.current_test_failed = true; \
        printf("  ASSERT FAIL: %s: got 0x%lx, should NOT equal 0x%lx (%s:%d)\n", \
               (msg), (unsigned long)(actual), (unsigned long)(not_expected), \
               __FILE__, __LINE__); \
    } \
} while (0)

/**
 * TEST_ASSERT_BITS - Check specific bit field of a value
 *
 * Extracts (value & mask) and compares with (expected & mask).
 * On failure, prints the full value, mask, and extracted field values.
 *
 * Useful for verifying CSR bit fields (e.g., mseccfg.MML, hstatus.SPV).
 */
#define TEST_ASSERT_BITS(msg, value, mask, expected) do { \
    test_results.total++; \
    uintptr_t _val = (uintptr_t)(value); \
    uintptr_t _mask = (uintptr_t)(mask); \
    uintptr_t _exp = (uintptr_t)(expected); \
    if ((_val & _mask) == (_exp & _mask)) { \
        test_results.passed++; \
    } else { \
        test_results.failed++; \
        test_results.current_test_failed = true; \
        printf("  ASSERT FAIL: %s: value=0x%lx, mask=0x%lx, " \
               "got field 0x%lx, expected field 0x%lx (%s:%d)\n", \
               (msg), (unsigned long)_val, (unsigned long)_mask, \
               (unsigned long)(_val & _mask), (unsigned long)(_exp & _mask), \
               __FILE__, __LINE__); \
    } \
} while (0)


/* ===================================================================
 * Exception Test Macros
 *
 * These macros wrap a statement with trap arming/disarming and
 * automatically check whether the expected exception occurred.
 * =================================================================== */

/* Forward declarations from trap.c */
extern void trap_expect_begin(void);
extern void trap_expect_end(void);
extern bool trap_was_triggered(void);
extern uintptr_t trap_get_cause(void);
extern uintptr_t trap_get_epc(void);
extern uintptr_t trap_get_tval(void);
extern uintptr_t trap_get_status_snap(void);

/* ===================================================================
 * Double Trap (Smdbltrp) Support
 *
 * On CPUs with Smdbltrp, mstatus.MDT (bit 42) is set to 1 on reset
 * and on every M-mode trap entry. While MDT=1, any synchronous
 * exception in M-mode triggers a fatal double trap instead of
 * entering the trap handler normally.
 *
 * mret clears MDT automatically, but if M-mode code *expects* to
 * trigger a trap (e.g., probing whether a CSR exists), MDT must be
 * cleared explicitly before the trapping instruction.
 *
 * Use M_TRAP_EXPECT_BEGIN / M_EXPECT_TRAP / M_EXPECT_NO_TRAP
 * instead of their plain counterparts when the protected operation
 * runs in M-mode and may raise a synchronous exception.
 * =================================================================== */

#ifdef PLATFORM_DOUBLE_TRAP
#if __riscv_xlen == 64
#define MSTATUS_MDT_BIT  (1UL << 42)
static inline void clear_mdt(void) {
    CSRC(mstatus, MSTATUS_MDT_BIT);
}
#else
/* On RV32, mstatus.MDT is bit 42 of the full 64-bit mstatus register.
 * The low 32 bits are accessed via mstatus, the high 32 bits via mstatush.
 * bit 42 = mstatush bit (42-32) = bit 10. */
#define MSTATUSH_MDT_BIT  (1UL << 10)
static inline void clear_mdt(void) {
    CSRC(mstatush, MSTATUSH_MDT_BIT);
}
#endif
#else
static inline void clear_mdt(void) { /* no-op */ }
#endif

/**
 * M_TRAP_EXPECT_BEGIN - Like trap_expect_begin, but first clears
 * mstatus.MDT on platforms with Smdbltrp so the expected trap
 * does not cause a double trap.
 */
#define M_TRAP_EXPECT_BEGIN() do { \
    clear_mdt(); \
    trap_expect_begin(); \
} while (0)

/**
 * M_EXPECT_TRAP - Like EXPECT_TRAP, but clears MDT first.
 */
#define M_EXPECT_TRAP(expected_cause, stmt) do { \
    clear_mdt(); \
    EXPECT_TRAP(expected_cause, stmt); \
} while (0)

/**
 * M_EXPECT_NO_TRAP - Like EXPECT_NO_TRAP, but clears MDT first.
 */
#define M_EXPECT_NO_TRAP(stmt) do { \
    clear_mdt(); \
    EXPECT_NO_TRAP(stmt); \
} while (0)

/**
 * EXPECT_NO_TRAP - Execute stmt and assert no exception occurs
 *
 * Must only be used in M-mode. In S/U-mode, use PRIV_DO + CHECK_NO_TRAP instead.
 */
#define EXPECT_NO_TRAP(stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("no trap", !trap_was_triggered()); \
    trap_expect_end(); \
} while (0)

/**
 * EXPECT_TRAP - Execute stmt and assert a specific exception occurs
 *
 * Must only be used in M-mode. In S/U-mode, use PRIV_DO + CHECK_TRAP instead.
 */
#define EXPECT_TRAP(expected_cause, stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("trap triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ("cause matches", trap_get_cause(), (expected_cause)); \
    } \
    trap_expect_end(); \
} while (0)

/**
 * EXPECT_EXEC_TRAP - Jump to addr and assert instruction access fault
 *
 * Uses exec_at() which saves return_addr for trap recovery.
 */
#define EXPECT_EXEC_TRAP(expected_cause, addr) do { \
    trap_expect_begin(); \
    exec_at(addr); \
    TEST_ASSERT("exec trap triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ("exec cause matches", trap_get_cause(), (expected_cause)); \
    } \
    trap_expect_end(); \
} while (0)

/**
 * EXPECT_EXEC_NO_TRAP - Jump to addr and assert no exception
 */
#define EXPECT_EXEC_NO_TRAP(addr) do { \
    trap_expect_begin(); \
    exec_at(addr); \
    TEST_ASSERT("exec no trap", !trap_was_triggered()); \
    trap_expect_end(); \
} while (0)

/* ===================================================================
 * S/U-mode safe trap macros
 *
 * When running in S-mode or U-mode, printf (UART access) is not
 * available. Use PRIV_DO_* to arm/execute/disarm in privileged mode,
 * then return to M-mode and use CHECK_* to assert results.
 *
 * Pattern:
 *   goto_priv(PRIV_S);
 *   PRIV_DO_NO_TRAP(mem_load8(addr));   // arm + execute + disarm, no printf
 *   goto_priv(PRIV_M);
 *   CHECK_NO_TRAP("read succeeded");    // assert in M-mode (printf OK)
 * =================================================================== */

/**
 * PRIV_DO - Arm trap, execute stmt, disarm. No printf.
 *
 * Use in S/U-mode where UART is inaccessible. After returning to
 * M-mode, use CHECK_NO_TRAP or CHECK_TRAP to assert the result.
 *
 * The trap arming/disarming is identical regardless of whether a
 * trap is expected; the expectation is expressed by the CHECK_*
 * macro used afterwards.
 */
#define PRIV_DO(stmt) do { \
    trap_expect_begin(); \
    (void)(stmt); \
    trap_expect_end(); \
} while (0)

/**
 * PRIV_DO_EXEC - Arm trap, exec_at(addr), disarm. No printf.
 *
 * Use in S/U-mode for testing instruction execution permissions.
 * After returning to M-mode, use CHECK_NO_TRAP or CHECK_TRAP.
 */
#define PRIV_DO_EXEC(addr) do { \
    trap_expect_begin(); \
    exec_at(addr); \
    trap_expect_end(); \
} while (0)

/* Backward-compatible aliases (deprecated — prefer PRIV_DO / PRIV_DO_EXEC) */
#define PRIV_DO_NO_TRAP(stmt)       PRIV_DO(stmt)
#define PRIV_DO_TRAP(stmt)          PRIV_DO(stmt)
#define PRIV_DO_EXEC_NO_TRAP(addr)  PRIV_DO_EXEC(addr)
#define PRIV_DO_EXEC_TRAP(addr)     PRIV_DO_EXEC(addr)

/**
 * CHECK_NO_TRAP - Assert that the last PRIV_DO_NO_TRAP did not trigger a trap.
 * Must be called in M-mode after goto_priv(PRIV_M).
 */
#define CHECK_NO_TRAP(msg) do { \
    TEST_ASSERT(msg, !trap_was_triggered()); \
} while (0)

/**
 * CHECK_TRAP - Assert that the last PRIV_DO_TRAP triggered the expected trap.
 * Must be called in M-mode after goto_priv(PRIV_M).
 */
#define CHECK_TRAP(msg, expected_cause) do { \
    TEST_ASSERT(msg ": trap triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ(msg ": cause", trap_get_cause(), (uintptr_t)(expected_cause)); \
    } \
} while (0)

/* ===================================================================
 * Privilege Level Definitions and API
 * =================================================================== */

#define PRIV_U  0
#define PRIV_S  1
#define PRIV_M  3

/* Virtualized privilege levels (V=1) — used by hypervisor (H ext) tests.
 * The high bit (bit 2) marks V=1; low 2 bits hold the nominal privilege.
 *   PRIV_VU = V=1, nominal U  (4)
 *   PRIV_VS = V=1, nominal S  (5)
 */
#define PRIV_VU 4
#define PRIV_VS 5

extern void goto_priv(unsigned target);
extern unsigned get_current_priv(void);
extern uintptr_t run_in_priv(unsigned priv, uintptr_t (*fn)(uintptr_t), uintptr_t arg);

#ifdef ENABLE_HYP
/* Hypervisor trap-record extensions, defined in trap.c when ENABLE_HYP=1.
 * Available after a guest-page-fault / virtual-instruction trap. */
extern uintptr_t trap_get_htval(void);
extern uintptr_t trap_get_htinst(void);
extern bool      trap_get_gva(void);
#endif

/* ===================================================================
 * State Management
 * =================================================================== */

/**
 * reset_state - Reset all PMP and test state
 *
 * Clears all PMP entries, resets mseccfg, returns to M-mode.
 */
void reset_state(void);

/**
 * test_print_summary - Print final test results
 */
int test_print_summary(void);

/* ===================================================================
 * Log Level Macros
 * =================================================================== */

#ifndef LOG_LEVEL
#define LOG_LEVEL 3
#endif

#define LOG_NONE    0
#define LOG_ERROR   1
#define LOG_WARN    2
#define LOG_INFO    3
#define LOG_DEBUG   4
#define LOG_TRACE   5
#define LOG_VERBOSE 6

#define LOG_E(fmt, ...) do { if (LOG_LEVEL >= LOG_ERROR)   printf("[E] " fmt, ##__VA_ARGS__); } while(0)
#define LOG_W(fmt, ...) do { if (LOG_LEVEL >= LOG_WARN)    printf("[W] " fmt, ##__VA_ARGS__); } while(0)
#define LOG_I(fmt, ...) do { if (LOG_LEVEL >= LOG_INFO)    printf("[I] " fmt, ##__VA_ARGS__); } while(0)
#define LOG_D(fmt, ...) do { if (LOG_LEVEL >= LOG_DEBUG)   printf("[D] " fmt, ##__VA_ARGS__); } while(0)
#define LOG_T(fmt, ...) do { if (LOG_LEVEL >= LOG_TRACE)   printf("[T] " fmt, ##__VA_ARGS__); } while(0)
#define LOG_V(fmt, ...) do { if (LOG_LEVEL >= LOG_VERBOSE) printf("[V] " fmt, ##__VA_ARGS__); } while(0)

#endif /* COMMON_TEST_FRAMEWORK_H */
