/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 2: Zicfilp Landing Pad Functional Tests
 *
 * Tests the behavior of LPAD instruction and ELP state management
 * under various LPE configurations.
 */

/* CFI-LP-FN-01: LPAD as no-op when LPE=0 */
TEST_REGISTER(test_zicfilp_lpad_noop_lpe0);
bool test_zicfilp_lpad_noop_lpe0(void)
{
    TEST_BEGIN("CFI-LP-FN-01: LPAD operates as no-op when LPE=0");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    /* Ensure MLPE=0 so Zicfilp is not active in M-mode */
    mseccfg_clear(MSECCFG_MLPE);

    /*
     * Execute LPAD (AUIPC x0, 0 = 0x00000017) in M-mode with MLPE=0.
     * Per norm:mseccfg_mlpe_clr_op_list: "The LPAD instruction operates
     * as a no-op."
     *
     * We emit LPAD into the test code region and jump to it.
     */
    uint32_t *code = (uint32_t *)__cfi_test_code_start;
    emit_lpad(code);         /* LPAD (no-op when LPE=0) */
    emit_ret(code + 1);     /* RET */

    /* Fence I-cache to ensure new instructions are visible */
    asm volatile("fence.i" ::: "memory");

    /* Call the function - should execute without exception */
    trap_expect_begin();
    typedef void (*func_t)(void);
    ((func_t)code)();
    trap_expect_end();

    TEST_ASSERT("LPAD as no-op did not trigger exception",
                !trap_was_triggered());

    TEST_END();
}

/* CFI-LP-FN-02: ELP not updated when LPE=0 */
TEST_REGISTER(test_zicfilp_elp_no_update_lpe0);
bool test_zicfilp_elp_no_update_lpe0(void)
{
    TEST_BEGIN("CFI-LP-FN-02: ELP stays NO_LP_EXPECTED when LPE=0");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    /* Ensure MLPE=0 */
    mseccfg_clear(MSECCFG_MLPE);

    /*
     * Per norm:mseccfg_mlpe_clr_op_list:
     * "The hart does not update the ELP state; it remains as
     *  NO_LP_EXPECTED."
     *
     * After an indirect jump with LPE=0, MPELP should remain 0.
     */
    uint32_t *code = (uint32_t *)__cfi_test_code_start;
    emit_nop(code);          /* target: NOP (not LPAD) */
    emit_ret(code + 1);     /* RET */
    asm volatile("fence.i" ::: "memory");

    /* Indirect call to non-LPAD target */
    trap_expect_begin();
    typedef void (*func_t)(void);
    ((func_t)code)();
    trap_expect_end();

    TEST_ASSERT("indirect jump to non-LPAD target with LPE=0 succeeds",
                !trap_was_triggered());

    /* Verify MPELP is still 0 (NO_LP_EXPECTED) */
    uintptr_t ms = mstatus_read();
    TEST_ASSERT("mstatus.MPELP remains 0 (NO_LP_EXPECTED)",
                (ms & MSTATUS_MPELP_BIT) == 0);

    TEST_END();
}

/* CFI-LP-FN-03: Legal indirect jump with LPE=1 (target has LPAD) */
TEST_REGISTER(test_zicfilp_legal_indirect_jump);
bool test_zicfilp_legal_indirect_jump(void)
{
    TEST_BEGIN("CFI-LP-FN-03: Legal indirect jump (target has LPAD, MLPE=1)");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    /*
     * Enable Zicfilp in M-mode: set mseccfg.MLPE=1.
     * Then perform an indirect jump to a target that starts with LPAD.
     * This should succeed without exception.
     *
     * IMPORTANT: Arm trap BEFORE setting MLPE to avoid faulting on
     * the trap_expect_begin() call itself (it's a normal function
     * without LPAD at its entry point).
     */
    uint32_t *code = (uint32_t *)__cfi_test_code_start;
    emit_lpad(code);         /* LPAD - valid landing pad */
    emit_ret(code + 1);     /* RET */
    asm volatile("fence.i" ::: "memory");

    /* Arm trap BEFORE enabling MLPE */
    trap_expect_begin();

    /* Set MLPE, indirect jump to LPAD target, clear MLPE - all inline */
    asm volatile(
        "li    t0, (1 << 10)\n"    /* MSECCFG_MLPE = bit 10 */
        "csrs  0x747, t0\n"        /* mseccfg |= MLPE */
        "jalr  ra, %0, 0\n"        /* indirect jump to LPAD target */
        "csrc  0x747, t0\n"        /* mseccfg &= ~MLPE */
        :
        : "r"(code)
        : "ra", "t0", "memory"
    );

    trap_expect_end();
    mseccfg_clear(MSECCFG_MLPE);

    TEST_ASSERT("indirect jump to LPAD target with MLPE=1 succeeds",
                !trap_was_triggered());

    TEST_END();
}

/* CFI-LP-FN-04: MPELP/SPELP save and restore across trap */
TEST_REGISTER(test_zicfilp_pelp_trap_save_restore);
bool test_zicfilp_pelp_trap_save_restore(void)
{
    TEST_BEGIN("CFI-LP-FN-04: xPELP trap save/restore behavior");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    /*
     * Per norm:Zicfilp_pelp_trap (SPEC/priv/cfi.adoc):
     * "When a trap is taken into privilege mode x, the xpelp is set
     *  to ELP and ELP is set to NO_LP_EXPECTED."
     *
     * When MLPE=1 and an indirect jump targets a non-LPAD instruction,
     * the hardware sets ELP=LP_EXPECTED before fetching the target.
     * The software-check exception is then taken, and MPELP captures
     * ELP=LP_EXPECTED at trap entry.
     *
     * The trap handler in trap.c captures mstatus into status_snap
     * BEFORE clearing MPELP. We use trap_get_status_snap() to verify
     * that MPELP was set to 1 (LP_EXPECTED) at trap entry.
     */

    uint32_t *code = (uint32_t *)__cfi_test_code_start;
    emit_nop(code);          /* NOT an LPAD - will trigger fault */
    emit_ret(code + 1);
    asm volatile("fence.i" ::: "memory");

    /* Arm trap BEFORE enabling MLPE */
    trap_expect_begin();

    /* Set MLPE=1, indirect jump to non-LPAD target (triggers LP fault),
     * then recover. All in inline asm to avoid compiler-generated calls. */
    asm volatile(
        "la    t1, 1f\n"           /* t1 = recovery address */
        "la    t2, _exec_return_addr\n"
        "sd    t1, 0(t2)\n"        /* _exec_return_addr = recovery */
        "li    t0, (1 << 10)\n"    /* MSECCFG_MLPE = bit 10 */
        "csrs  0x747, t0\n"        /* mseccfg |= MLPE */
        "jalr  ra, %0, 0\n"        /* indirect jump -> triggers LP fault */
        "1:\n"
        "csrc  0x747, t0\n"        /* mseccfg &= ~MLPE */
        :
        : "r"(code)
        : "ra", "t0", "t1", "t2", "memory"
    );

    trap_expect_end();
    mseccfg_clear(MSECCFG_MLPE);

    /* Verify the software-check exception was captured */
    TEST_ASSERT("software-check exception triggered",
                trap_was_triggered());
    TEST_ASSERT("mcause is CAUSE_SOFTWARE_CHECK (18)",
                trap_get_cause() == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("mtval is SWCHECK_LANDING_PAD_FAULT (2)",
                trap_get_tval() == SWCHECK_LANDING_PAD_FAULT);

    /*
     * Verify MPELP was set at trap entry.
     * status_snap captures mstatus BEFORE the handler clears MPELP.
     * When the LP fault occurs, ELP was LP_EXPECTED (set by the
     * indirect jump), so MPELP should be 1 in the snapshot.
     */
    uintptr_t snap = trap_get_status_snap();
    uintptr_t mpelp_at_trap = (snap & MSTATUS_MPELP_BIT) ? 1 : 0;
    printf("    MPELP at trap entry = %lu (expect 1 = LP_EXPECTED)\n",
           (unsigned long)mpelp_at_trap);
    TEST_ASSERT("MPELP captured ELP=LP_EXPECTED at trap entry",
                mpelp_at_trap == 1);

    /* After trap handler clears MPELP, it should be 0 */
    uintptr_t ms_after = mstatus_read();
    TEST_ASSERT("MPELP cleared by handler after trap",
                (ms_after & MSTATUS_MPELP_BIT) == 0);

    TEST_END();
}
