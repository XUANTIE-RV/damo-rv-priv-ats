/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 8: htimedelta time offset
 *
 * Tests HTDL-01 through HTDL-04 verify htimedelta register behavior.
 */

#include "test_helpers.h"

/* ------------------------------------------------------------------
 * HTDL-01: htimedelta basic read/write
 * ------------------------------------------------------------------ */
TEST_REGISTER(htimedelta_basic_rw);
bool htimedelta_basic_rw(void) {
    TEST_BEGIN("HTDL-01: Verify htimedelta basic read/write");

    /* Write 1000 to htimedelta. */
    htimedelta_set(1000);
    uintptr_t val; asm volatile("csrr %0, 0x605" : "=r"(val));

    /* Verify value is written. */
    TEST_ASSERT_EQ("htimedelta should be writable", val, 1000);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HTDL-02: htimedelta affects VS time observation
 * ------------------------------------------------------------------ */
TEST_REGISTER(htimedelta_affects_vs_time);
bool htimedelta_affects_vs_time(void) {
    TEST_BEGIN("HTDL-02: Verify htimedelta affects VS time observation");

    /* Allow VS-mode to access time CSR.
     * Both mcounteren.TM and hcounteren.TM must be set for VS-mode
     * to read time without trapping (spec: mcounteren gates S/HS,
     * hcounteren gates VS). */
    uintptr_t saved_mcen = mcounteren_read();
    mcounteren_set(0x2); /* TM bit */
    uintptr_t saved_hcen = hcounteren_read();
    hcounteren_set(0x2); /* TM bit */
    /* Set htimedelta to a known offset. */
    uintptr_t delta = 100000;
    htimedelta_set(delta);
    /* Read real time from HS-mode */
    uintptr_t hs_time; asm volatile("csrr %0, time" : "=r"(hs_time));
    /* Read time from VS-mode (should be hs_time + delta) */
    uintptr_t vs_time = run_in_vs_mode(vs_read_time, 0);
    /* VS time should be approximately hs_time + delta
     * Allow some slack for instruction execution time */
    int64_t diff = (int64_t)(vs_time - hs_time);
    TEST_ASSERT("VS time >= HS time + delta (approx)",
                diff >= (int64_t)(delta - 1000) && diff <= (int64_t)(delta + 100000));
    htimedelta_set(0); /* Restore */
    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HTDL-03: htimedelta=0 means VS time equals real time
 * ------------------------------------------------------------------ */
TEST_REGISTER(htimedelta_zero_equals_real_time);
bool htimedelta_zero_equals_real_time(void) {
    TEST_BEGIN("HTDL-03: Verify htimedelta=0 means VS time equals real time");

    /* Set htimedelta to 0. */
    htimedelta_set(0);
    uintptr_t val; asm volatile("csrr %0, 0x605" : "=r"(val));

    /* Verify htimedelta is zero. */
    TEST_ASSERT_EQ("htimedelta should be 0", val, 0);

    /* Allow VS-mode to access time CSR.
     * Both mcounteren.TM and hcounteren.TM must be set for VS-mode
     * to read time without trapping. */
    uintptr_t saved_mcen = mcounteren_read();
    mcounteren_set(0x2); /* TM bit */
    uintptr_t saved_hcen = hcounteren_read();
    hcounteren_set(0x2); /* TM bit */
    /* VS time should equal real time when delta is 0. */
    htimedelta_set(0);
    uintptr_t hs_time; asm volatile("csrr %0, time" : "=r"(hs_time));
    uintptr_t vs_time = run_in_vs_mode(vs_read_time, 0);
    int64_t diff = (int64_t)(vs_time - hs_time);
    /* With delta=0, VS and HS time should be very close */
    TEST_ASSERT("VS time close to HS time when delta=0",
                diff >= 0 && diff < 100000);
    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HTDL-04: htimedelta large value test
 * ------------------------------------------------------------------ */
TEST_REGISTER(htimedelta_large_value);
bool htimedelta_large_value(void) {
    TEST_BEGIN("HTDL-04: Verify htimedelta handles large values");

    /* Set htimedelta to a large value. */
    int64_t large_delta = 1000000000LL;
    htimedelta_set(large_delta);
    uintptr_t val; asm volatile("csrr %0, 0x605" : "=r"(val));

    /* Verify large value is handled. */
    TEST_ASSERT_EQ("htimedelta should handle large values", val, large_delta);

    HYP_TEST_END();
}
