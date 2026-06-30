/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_breakpoint.c - Group 4: Breakpoint exceptions (non-EBREAK)
 *
 * Per Sstvala (SPEC/sstvala.adoc:6-8):
 *   For breakpoint exception not caused by EBREAK/C.EBREAK, stval
 *   must contain the faulting address.
 *
 * Tests:
 *   TVAL-BKP-01: trigger breakpoint via load address match
 *   TVAL-BKP-02: trigger breakpoint via execute address match
 *   TVAL-BKP-03: EBREAK exclusion (reverse check, no stval assertion)
 *
 * NOTE: Requires Sdtrig extension (Debug Trigger module). If the
 * platform does not implement triggers, all tests are skipped.
 */

/* Trigger CSR addresses */
#define CSR_TSELECT  0x7A0
#define CSR_TDATA1   0x7A1
#define CSR_TDATA2   0x7A2

/*
 * mcontrol6 (type=6) field layout for RV64:
 *   [63:60] type    = 6 (mcontrol6)
 *   [10:7]  match   = 0 (exact address match)
 *   [6]     M       (enable in M-mode)
 *   [4]     S       (enable in S-mode)
 *   [3]     U       (enable in U-mode)
 *   [2]     execute
 *   [1]     store
 *   [0]     load
 */
#define MCONTROL6_TYPE     (6UL << 60)
#define MCONTROL6_LOAD     (1UL << 0)
#define MCONTROL6_STORE    (1UL << 1)
#define MCONTROL6_EXECUTE  (1UL << 2)
#define MCONTROL6_M        (1UL << 6)
#define MCONTROL6_S        (1UL << 4)
#define MCONTROL6_MATCH_EQ (0UL << 7)

/* ===================================================================
 * Helper: detect if trigger module is available
 * =================================================================== */
static bool trigger_available = false;
static bool trigger_detection_done = false;

static bool detect_trigger_module(void) {
    if (trigger_detection_done)
        return trigger_available;

    trap_expect_begin();
    asm volatile ("csrr zero, 0x7A0" ::: "memory");  /* read tselect */

    trigger_available = !trap_was_triggered();
    trap_expect_end();
    trigger_detection_done = true;
    return trigger_available;
}

/* ===================================================================
 * Helper: clean up trigger entry 0
 * =================================================================== */
static void trigger_cleanup(void) {
    /* Disable trigger by clearing tdata1 */
    asm volatile ("csrw 0x7A0, zero" ::: "memory");  /* tselect = 0 */
    asm volatile ("csrw 0x7A1, zero" ::: "memory");  /* tdata1  = 0 */
    asm volatile ("csrw 0x7A2, zero" ::: "memory");  /* tdata2  = 0 */
}

/* ===================================================================
 * TVAL-BKP-01: trigger breakpoint on load address match
 * =================================================================== */
TEST_REGISTER(test_sstvala_bkp_01);
bool test_sstvala_bkp_01(void) {
    TEST_BEGIN("TVAL-BKP-01: trigger breakpoint (load) stval == trigger addr");

#ifdef SKIP_BREAKPOINT_TESTS
    TEST_SKIP("platform does not support breakpoint (SKIP_BREAKPOINT_TESTS)");
#endif

    if (!detect_trigger_module())
        TEST_SKIP("platform does not implement trigger module");

    static volatile uint64_t trigger_target = 0xCAFEBABEULL;
    uintptr_t trigger_addr = (uintptr_t)&trigger_target;

    /* Configure trigger in M-mode: breakpoint on load at trigger_addr.
     * Enable for S-mode (not M-mode) so the breakpoint fires when
     * we switch to S-mode to execute the load. */
    asm volatile ("csrw 0x7A0, zero" ::: "memory");       /* tselect = 0 */
    asm volatile ("csrw 0x7A2, %0" :: "r"(trigger_addr) : "memory");
    uintptr_t tdata1_val = MCONTROL6_TYPE | MCONTROL6_LOAD |
                           MCONTROL6_S | MCONTROL6_MATCH_EQ;
    asm volatile ("csrw 0x7A1, %0" :: "r"(tdata1_val) : "memory");

    /* Verify the trigger was actually configured (WARL check) */
    uintptr_t readback;
    asm volatile ("csrr %0, 0x7A1" : "=r"(readback) :: "memory");
    if (readback == 0) {
        trigger_cleanup();
        TEST_SKIP("trigger type=6 (mcontrol6) not supported");
    }

    /* Execute load in S-mode to trigger the breakpoint */
    ensure_smode_pmp();
    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_load64(trigger_addr));
    goto_priv(PRIV_M);

    CHECK_TRAP("breakpoint triggered", CAUSE_BREAKPOINT);
    TEST_ASSERT_EQ("stval == trigger address",
                   trap_get_tval(), trigger_addr);

    trigger_cleanup();
    TEST_END();
}

/* ===================================================================
 * TVAL-BKP-02: trigger breakpoint on execute address match
 * =================================================================== */
TEST_REGISTER(test_sstvala_bkp_02);
bool test_sstvala_bkp_02(void) {
    TEST_BEGIN("TVAL-BKP-02: trigger breakpoint (execute) stval == trigger PC");

#ifdef SKIP_BREAKPOINT_TESTS
    TEST_SKIP("platform does not support breakpoint (SKIP_BREAKPOINT_TESTS)");
#endif

    if (!detect_trigger_module())
        TEST_SKIP("platform does not implement trigger module");

    /* Use test_exec_page as the trigger target (fill with nop;ret) */
    init_exec_page();
    uintptr_t trigger_pc = (uintptr_t)test_exec_page;

    /* Configure trigger in M-mode: breakpoint on execute at trigger_pc.
     * Enable for S-mode so it fires when we switch to S-mode. */
    asm volatile ("csrw 0x7A0, zero" ::: "memory");
    asm volatile ("csrw 0x7A2, %0" :: "r"(trigger_pc) : "memory");
    uintptr_t tdata1_val = MCONTROL6_TYPE | MCONTROL6_EXECUTE |
                           MCONTROL6_S | MCONTROL6_MATCH_EQ;
    asm volatile ("csrw 0x7A1, %0" :: "r"(tdata1_val) : "memory");

    uintptr_t readback;
    asm volatile ("csrr %0, 0x7A1" : "=r"(readback) :: "memory");
    if (readback == 0) {
        trigger_cleanup();
        TEST_SKIP("trigger type=6 (mcontrol6) not supported");
    }

    /* Execute in S-mode to trigger the breakpoint */
    ensure_smode_pmp();
    goto_priv(PRIV_S);
    PRIV_DO_EXEC_TRAP(trigger_pc);
    goto_priv(PRIV_M);

    CHECK_TRAP("breakpoint triggered", CAUSE_BREAKPOINT);
    TEST_ASSERT_EQ("stval == trigger PC",
                   trap_get_tval(), trigger_pc);

    trigger_cleanup();
    TEST_END();
}

/* ===================================================================
 * TVAL-BKP-03: EBREAK exclusion (reverse check)
 *
 * Sstvala explicitly excludes EBREAK/C.EBREAK from the stval
 * requirement. This test verifies that EBREAK does trigger a
 * breakpoint (cause=3) but does NOT assert a specific stval value.
 * =================================================================== */
TEST_REGISTER(test_sstvala_bkp_03);
bool test_sstvala_bkp_03(void) {
    TEST_BEGIN("TVAL-BKP-03: EBREAK breakpoint (stval not asserted by Sstvala)");

#ifdef SKIP_BREAKPOINT_TESTS
    TEST_SKIP("platform does not support breakpoint (SKIP_BREAKPOINT_TESTS)");
#endif

    /* Execute EBREAK in S-mode */
    ensure_smode_pmp();
    goto_priv(PRIV_S);
    trap_expect_begin();
    asm volatile ("ebreak" ::: "memory");
    trap_expect_end();
    goto_priv(PRIV_M);

    CHECK_TRAP("EBREAK trap", CAUSE_BREAKPOINT);
    /* Per Sstvala spec, EBREAK's stval behavior is defined by
     * other specifications. We only log the value, no assertion. */
    printf("  [INFO] EBREAK stval = 0x%lx (not asserted)\n",
           (unsigned long)trap_get_tval());

    TEST_END();
}
