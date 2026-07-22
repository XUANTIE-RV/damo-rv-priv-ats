/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 3: Zicfilp Exception Behavior Tests
 *
 * Tests that Landing Pad Fault (software-check exception with mtval=2)
 * is correctly triggered when an indirect jump lands on a non-LPAD
 * instruction with Zicfilp enabled in M-mode, S-mode, and U-mode.
 * Also tests exception delegation via medeleg.
 */

/* Recovery address used by trap handler to resume after LP fault */
extern uintptr_t _exec_return_addr;

/* ecall_args is used for privilege switching via ecall */
extern uintptr_t ecall_args[2];

/* trap_record - used to arm/disarm trap capture */
extern volatile struct {
    bool      armed;
    bool      triggered;
    unsigned  priv_level;
    uintptr_t cause;
    uintptr_t epc;
    uintptr_t tval;
    uintptr_t status_snap;
    uintptr_t return_addr;
} trap_record;

/* ECALL_GOTO_PRIV and privilege constants (from encoding.h) */
#ifndef ECALL_GOTO_PRIV
#define ECALL_GOTO_PRIV  1
#endif

/* ===================================================================
 * Helper: set up ecall_args and execute ecall to return to M-mode.
 *
 * When LPE=1 in S/U-mode, function calls (indirect jumps) would
 * trigger LP faults. We use ecall (not an indirect jump) to return
 * to M-mode safely.
 * =================================================================== */
#define ECALL_RETURN_TO_M()                                     \
    do {                                                        \
        asm volatile(                                           \
            "la    t0, ecall_args\n"                            \
            "li    t1, 1\n"      /* ECALL_GOTO_PRIV */          \
            "sd    t1, 0(t0)\n"                                \
            "li    t1, 3\n"      /* PRIV_M */                   \
            "sd    t1, 8(t0)\n"                                \
            "ecall\n"                                           \
            ::: "t0", "t1", "memory"                            \
        );                                                      \
    } while (0)

/* CFI-LP-EXC-01: Illegal indirect jump triggers LP Fault in S-mode */
TEST_REGISTER(test_zicfilp_trap_smode_lp_fault);
bool test_zicfilp_trap_smode_lp_fault(void)
{
    TEST_BEGIN("CFI-LP-EXC-01: LP Fault on indirect jump to non-LPAD (S-mode)");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    /*
     * With menvcfg.LPE=1, an indirect jump in S-mode to a target
     * that does NOT start with LPAD must trigger:
     *   - software-check exception (mcause=18)
     *   - mtval=2 (Landing Pad Fault)
     *
     * The exception is NOT delegated (medeleg=0), so it traps to
     * M-mode handler. We use inline asm for the entire S-mode
     * execution because any compiler-generated function call would
     * be an indirect jump that triggers LP fault when LPE=1.
     *
     * Flow:
     * 1. Arm trap
     * 2. Set _exec_return_addr to recovery label
     * 3. Set mstatus.MPP = S (01)
     * 4. Set menvcfg.LPE = 1
     * 5. Set mepc to S-mode jalr code
     * 6. mret -> switch to S-mode
     * 7. S-mode: jalr to non-LPAD target -> LP fault -> M-mode handler
     * 8. Handler sets mepc = _exec_return_addr, mret to recovery label (S-mode)
     * 9. Recovery: ecall back to M-mode
     */
    uint32_t *code = (uint32_t *)__cfi_test_code_start;
    emit_nop(code);          /* NOT an LPAD - should trigger fault */
    emit_ret(code + 1);
    asm volatile("fence.i" ::: "memory");

    /* Ensure no delegation: trap goes to M-mode */
    asm volatile("csrw 0x302, zero" ::: "memory");  /* medeleg = 0 */

    trap_expect_begin();

    asm volatile(
        /* Set recovery address */
        "la    t1, 1f\n"               /* t1 = recovery label */
        "la    t2, _exec_return_addr\n"
        "sd    t1, 0(t2)\n"            /* _exec_return_addr = recovery */

        /* Set MPP = S (01): set bit 11, clear bit 12 */
        "li    t0, (1 << 11)\n"
        "csrs  mstatus, t0\n"          /* set MPP[0] */
        "li    t0, (1 << 12)\n"
        "csrc  mstatus, t0\n"          /* clear MPP[1] */

        /* Enable LPE for S-mode */
        "li    t0, (1 << 2)\n"         /* MENVCFG_LPE = bit 2 */
        "csrs  0x30A, t0\n"            /* menvcfg |= LPE */

        /* Set mepc to S-mode code (the jalr instruction below) */
        "la    t0, 2f\n"
        "csrw  mepc, t0\n"

        /* Switch to S-mode */
        "mret\n"

        /* --- S-mode code starts here --- */
        "2:\n"
        "jalr  ra, %0, 0\n"           /* indirect jump -> triggers LP fault */

        /* --- Recovery label (S-mode, after trap handler returns) --- */
        "1:\n"
        /* Return to M-mode via ecall */
        "la    t0, ecall_args\n"
        "li    t1, 1\n"               /* ECALL_GOTO_PRIV */
        "sd    t1, 0(t0)\n"
        "li    t1, 3\n"               /* PRIV_M */
        "sd    t1, 8(t0)\n"
        "ecall\n"
        :
        : "r"(code)
        : "ra", "t0", "t1", "t2", "memory"
    );

    trap_expect_end();

    /* Clean up: clear LPE */
    menvcfg_clear(MENVCFG_LPE);

    TEST_ASSERT("software-check exception triggered",
                trap_was_triggered());
    TEST_ASSERT("mcause is CAUSE_SOFTWARE_CHECK (18)",
                trap_get_cause() == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("mtval is SWCHECK_LANDING_PAD_FAULT (2)",
                trap_get_tval() == SWCHECK_LANDING_PAD_FAULT);

    TEST_END();
}

/* CFI-LP-EXC-02: Illegal indirect jump triggers LP Fault in M-mode */
TEST_REGISTER(test_zicfilp_trap_mmode_lp_fault);
bool test_zicfilp_trap_mmode_lp_fault(void)
{
    TEST_BEGIN("CFI-LP-EXC-02: LP Fault on indirect jump to non-LPAD (M-mode)");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    /*
     * With mseccfg.MLPE=1, an indirect jump in M-mode to a target
     * that does NOT start with LPAD must trigger:
     *   - software-check exception (mcause=18)
     *   - mtval=2 (Landing Pad Fault)
     */
    uint32_t *code = (uint32_t *)__cfi_test_code_start;
    emit_nop(code);          /* NOT an LPAD - should trigger fault */
    emit_ret(code + 1);
    asm volatile("fence.i" ::: "memory");

    /* Arm trap BEFORE enabling MLPE */
    trap_expect_begin();

    asm volatile(
        "la    t1, 1f\n"           /* t1 = recovery address */
        "la    t2, _exec_return_addr\n"
        "sd    t1, 0(t2)\n"        /* _exec_return_addr = recovery */
        "li    t0, (1 << 10)\n"    /* MSECCFG_MLPE = bit 10 */
        "csrs  0x747, t0\n"        /* mseccfg |= MLPE */
        "jalr  ra, %0, 0\n"        /* indirect jump -> triggers LP fault */
        "1:\n"
        "csrc  0x747, t0\n"        /* mseccfg &= ~MLPE (after trap return) */
        :
        : "r"(code)
        : "ra", "t0", "t1", "t2", "memory"
    );

    trap_expect_end();
    mseccfg_clear(MSECCFG_MLPE);

    TEST_ASSERT("software-check exception triggered",
                trap_was_triggered());
    TEST_ASSERT("mcause is CAUSE_SOFTWARE_CHECK (18)",
                trap_get_cause() == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("mtval is SWCHECK_LANDING_PAD_FAULT (2)",
                trap_get_tval() == SWCHECK_LANDING_PAD_FAULT);

    TEST_END();
}

/* CFI-LP-EXC-03: Illegal indirect jump triggers LP Fault in U-mode */
TEST_REGISTER(test_zicfilp_trap_umode_lp_fault);
bool test_zicfilp_trap_umode_lp_fault(void)
{
    TEST_BEGIN("CFI-LP-EXC-03: LP Fault on indirect jump to non-LPAD (U-mode)");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    /*
     * With senvcfg.LPE=1, an indirect jump in U-mode to a target
     * that does NOT start with LPAD must trigger:
     *   - software-check exception (mcause=18)
     *   - mtval=2 (Landing Pad Fault)
     *
     * The exception is NOT delegated, so it traps to M-mode handler.
     * Same inline asm approach as S-mode test but with MPP=U (00).
     */
    uint32_t *code = (uint32_t *)__cfi_test_code_start;
    emit_nop(code);          /* NOT an LPAD - should trigger fault */
    emit_ret(code + 1);
    asm volatile("fence.i" ::: "memory");

    /* Ensure no delegation */
    asm volatile("csrw 0x302, zero" ::: "memory");  /* medeleg = 0 */

    /* Save and set senvcfg.LPE=1 for U-mode */
    uintptr_t orig_senvcfg = senvcfg_read();
    senvcfg_set(SENVCFG_LPE);

    trap_expect_begin();

    asm volatile(
        /* Set recovery address */
        "la    t1, 1f\n"
        "la    t2, _exec_return_addr\n"
        "sd    t1, 0(t2)\n"

        /* Set MPP = U (00): clear both bits 11 and 12 */
        "li    t0, (1 << 11)\n"
        "csrc  mstatus, t0\n"          /* clear MPP[0] */
        "li    t0, (1 << 12)\n"
        "csrc  mstatus, t0\n"          /* clear MPP[1] */

        /* Set mepc to U-mode code */
        "la    t0, 2f\n"
        "csrw  mepc, t0\n"

        /* Switch to U-mode */
        "mret\n"

        /* --- U-mode code starts here --- */
        "2:\n"
        "jalr  ra, %0, 0\n"           /* indirect jump -> triggers LP fault */

        /* --- Recovery label (U-mode) --- */
        "1:\n"
        "la    t0, ecall_args\n"
        "li    t1, 1\n"               /* ECALL_GOTO_PRIV */
        "sd    t1, 0(t0)\n"
        "li    t1, 3\n"               /* PRIV_M */
        "sd    t1, 8(t0)\n"
        "ecall\n"
        :
        : "r"(code)
        : "ra", "t0", "t1", "t2", "memory"
    );

    trap_expect_end();

    /* Clean up */
    senvcfg_write(orig_senvcfg);

    TEST_ASSERT("software-check exception triggered",
                trap_was_triggered());
    TEST_ASSERT("mcause is CAUSE_SOFTWARE_CHECK (18)",
                trap_get_cause() == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("mtval is SWCHECK_LANDING_PAD_FAULT (2)",
                trap_get_tval() == SWCHECK_LANDING_PAD_FAULT);

    TEST_END();
}

/* CFI-LP-EXC-04: LP Fault exception delegation via medeleg */
TEST_REGISTER(test_zicfilp_trap_delegation);
bool test_zicfilp_trap_delegation(void)
{
    TEST_BEGIN("CFI-LP-EXC-04: software-check exception delegatable via medeleg[18]");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    /*
     * Per SPEC: software-check exception (cause=18) can be delegated
     * to S-mode via medeleg[18].
     *
     * This test verifies:
     * 1. medeleg[18] is writable (delegation capability exists)
     * 2. With medeleg[18]=0, LP fault in S-mode is captured by M-mode
     *    handler (no delegation)
     *
     * NOTE: Full end-to-end delegation testing (verifying the S-mode
     * handler receives the delegated exception) requires a custom
     * S-mode trap handler that can operate with LPE=1 (where all
     * indirect calls require LPAD). The framework's S-mode handler
     * uses function calls which fault under LPE=1. This is a
     * framework limitation to be addressed in a future update.
     */

    /* Part 1: Verify medeleg[18] is writable */
    uintptr_t orig_medeleg;
    asm volatile("csrr %0, 0x302" : "=r"(orig_medeleg));

    uintptr_t bit18 = (1UL << 18);
    asm volatile("csrs 0x302, %0" :: "r"(bit18) : "memory");

    uintptr_t new_medeleg;
    asm volatile("csrr %0, 0x302" : "=r"(new_medeleg));

    bool delegatable = (new_medeleg & bit18) != 0;

    /* Restore medeleg */
    asm volatile("csrw 0x302, %0" :: "r"(orig_medeleg) : "memory");

    TEST_ASSERT("medeleg[18] is writable (software-check delegatable)",
                delegatable);

    /* Part 2: Verify LP fault in S-mode without delegation (medeleg=0)
     * is captured by M-mode handler. This confirms the fault mechanism
     * works correctly in S-mode. */
    uint32_t *code = (uint32_t *)__cfi_test_code_start;
    emit_nop(code);          /* NOT an LPAD */
    emit_ret(code + 1);
    asm volatile("fence.i" ::: "memory");

    /* Ensure no delegation */
    asm volatile("csrw 0x302, zero" ::: "memory");

    uintptr_t orig_menvcfg = menvcfg_read();

    trap_expect_begin();

    asm volatile(
        "la    t1, 1f\n"
        "la    t2, _exec_return_addr\n"
        "sd    t1, 0(t2)\n"

        /* Set MPP = S (01) */
        "li    t0, (1 << 11)\n"
        "csrs  mstatus, t0\n"
        "li    t0, (1 << 12)\n"
        "csrc  mstatus, t0\n"

        /* Enable LPE for S-mode */
        "li    t0, (1 << 2)\n"
        "csrs  0x30A, t0\n"

        /* Set mepc to S-mode code */
        "la    t0, 2f\n"
        "csrw  mepc, t0\n"

        /* Switch to S-mode */
        "mret\n"

        /* --- S-mode code --- */
        "2:\n"
        "jalr  ra, %0, 0\n"      /* triggers LP fault -> M-mode handler */

        /* --- Recovery (S-mode) --- */
        "1:\n"
        "la    t0, ecall_args\n"
        "li    t1, 1\n"
        "sd    t1, 0(t0)\n"
        "li    t1, 3\n"
        "sd    t1, 8(t0)\n"
        "ecall\n"
        :
        : "r"(code)
        : "ra", "t0", "t1", "t2", "memory"
    );

    trap_expect_end();

    /* Clean up */
    menvcfg_write(orig_menvcfg);
    asm volatile("csrw 0x302, %0" :: "r"(orig_medeleg) : "memory");

    TEST_ASSERT("LP fault in S-mode captured by M-mode handler",
                trap_was_triggered());
    TEST_ASSERT("mcause is CAUSE_SOFTWARE_CHECK (18)",
                trap_get_cause() == CAUSE_SOFTWARE_CHECK);
    TEST_ASSERT("mtval is SWCHECK_LANDING_PAD_FAULT (2)",
                trap_get_tval() == SWCHECK_LANDING_PAD_FAULT);

    TEST_END();
}
