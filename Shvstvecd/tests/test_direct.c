/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_direct.c - Group 3: MODE=Direct VS-mode trap dispatch
 *
 * Per SPEC/supervisor.adoc:355-364 and Shvstvecd Direct-mode invariant:
 *   When vstvec.MODE=Direct, every trap (sync exception OR async
 *   interrupt) sets pc to BASE -- NOT to BASE + 4*cause.
 *
 * These tests enter VS-mode via run_in_vs_mode(), set up vstvec (via
 * stvec instruction in V=1) to point at shvstvecd_trap_entry, trigger
 * a trap delegated to VS-mode, then verify the landing address.
 *
 * Test coverage:
 *   VSTVEC-DIR-01: ecall from VS-mode (cause=2 as env-call-from-VS is 10,
 *                  but we use illegal insn cause=2 for simplicity)
 *   VSTVEC-DIR-02: illegal instruction (cause=2)
 *   VSTVEC-INT-01: VSSIP interrupt lands at BASE (not BASE+4*cause)
 *   VSTVEC-DIR-03: load guest-page-fault (cause=21) — but since we use
 *                  identity-mapped G-stage, we trigger ecall which is
 *                  delegated to VS-mode (cause=10, env-call-from-VS)
 */

/* ===================================================================
 * VS-mode payload functions
 * =================================================================== */

/* VS-mode: set vstvec via stvec (V=1), then trigger illegal insn. */
static uintptr_t vsmode_trigger_illegal(uintptr_t entry) {
    /* V=1: csrw stvec actually writes vstvec; MODE=0 (Direct) */
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    /* Arm vsscratch via sscratch (V=1) for the trap handler. */
    asm volatile ("csrw sscratch, %0"
                  :: "r"(shvstvecd_trap_scratch) : "memory");
    /* Trigger illegal instruction (cause=2). .4byte 0 is guaranteed
     * illegal on RV64. Trap entry advances sepc by 4. */
    asm volatile (".4byte 0x00000000");
    return 0;
}

/* VS-mode: set vstvec via stvec (V=1), then trigger ecall (cause=2
 * in VU context, but from VS-mode it's cause=9 env-call-from-S which
 * in V=1 becomes cause=10 env-call-from-VS — however since we don't
 * delegate ecall-from-VS to VS-mode (it would conflict with run_in_vs_mode
 * exit path), we use a different approach: trigger illegal insn for DIR-01
 * and use ecall only as exit mechanism.
 * Actually for this test we use the illegal instruction approach. */

/* VSTVEC-DIR-01: Direct mode, illegal instruction lands at BASE.
 * (The original plan used ecall from VU, but framework's run_in_vs_mode
 * uses ecall as its exit path, so we substitute with illegal instruction
 * which is cleanly delegatable.) */

TEST_REGISTER(test_shvstvecd_dir_01_illegal_insn);
bool test_shvstvecd_dir_01_illegal_insn(void) {
    TEST_BEGIN("VSTVEC-DIR-01: Direct mode, illegal insn lands at vstvec.BASE");

    uintptr_t entry = (uintptr_t)&shvstvecd_trap_entry;
    TEST_ASSERT("entry is 4-byte aligned", (entry & 0x3UL) == 0);

    /* Delegate illegal-instruction (cause=2) to VS-mode */
    hyp_delegate_to_vs(1UL << 2, 0);

    shvstvecd_reset_trap_record();

    /* Enter VS-mode: set vstvec and trigger illegal insn */
    run_in_vs_mode(vsmode_trigger_illegal, entry);

    /* Direct mode: trap landing address must == BASE */
    TEST_ASSERT("trap PC equals vstvec.BASE",
                g_shvstvecd_trap_pc == entry);
    TEST_ASSERT("vscause == 2 (illegal instruction)",
                shvstvecd_cause_code(g_shvstvecd_trap_cause) == 2);
    TEST_ASSERT("vscause is sync exception",
                !shvstvecd_is_interrupt(g_shvstvecd_trap_cause));

    hyp_undelegate();
    HYP_TEST_END();
}

/* VS-mode: trigger ecall from VS-mode itself. This generates
 * environment-call-from-VS (cause=10 in non-virtualized, but in V=1
 * context the ecall from S-mode has cause=9; after delegation through
 * hedeleg bit 9, it becomes a VS-mode-handled trap with vscause=9).
 * Actually: when V=1 and the VS-mode executes ecall, it generates
 * "environment call from VS-mode" = cause 10 if trapped by HS-mode,
 * but if hedeleg bit 8 is set... Let me clarify:
 *
 * Per spec: VS-mode ecall generates "environment call from VS-mode"
 * (cause=10) at HS/M level. hedeleg cannot delegate cause=10 (it's
 * not delegatable). So we cannot use VS-mode ecall for this test.
 *
 * Instead, VSTVEC-DIR-02 uses a store access fault scenario. But the
 * simplest approach is: in VS-mode, write an illegal instruction as
 * a second test variant (store to unmapped address via hlv). Let's
 * just use another illegal instruction variant with a different bit
 * pattern to verify the same Direct-mode behavior. */

/* VS-mode: set vstvec, then trigger a different illegal insn. */
static uintptr_t vsmode_trigger_illegal_v2(uintptr_t entry) {
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    asm volatile ("csrw sscratch, %0"
                  :: "r"(shvstvecd_trap_scratch) : "memory");
    /* Different illegal insn encoding (all-1s also illegal on RV64). */
    asm volatile (".4byte 0xFFFFFFFF");
    return 0;
}

TEST_REGISTER(test_shvstvecd_dir_02_illegal_insn_v2);
bool test_shvstvecd_dir_02_illegal_insn_v2(void) {
    TEST_BEGIN("VSTVEC-DIR-02: Direct mode, second illegal insn lands at BASE");

    uintptr_t entry = (uintptr_t)&shvstvecd_trap_entry;

    /* Delegate illegal-instruction (cause=2) to VS-mode */
    hyp_delegate_to_vs(1UL << 2, 0);

    shvstvecd_reset_trap_record();

    run_in_vs_mode(vsmode_trigger_illegal_v2, entry);

    TEST_ASSERT("trap PC equals vstvec.BASE",
                g_shvstvecd_trap_pc == entry);
    TEST_ASSERT("vscause == 2 (illegal instruction)",
                shvstvecd_cause_code(g_shvstvecd_trap_cause) == 2);

    hyp_undelegate();
    HYP_TEST_END();
}

/* VSTVEC-INT-01: VSSIP interrupt lands at BASE (not BASE+4*cause).
 * Uses hvip to inject VSSIP, then enters VS-mode where we enable
 * the interrupt. Since MODE=Direct, trap should land at BASE. */

static uintptr_t vsmode_enable_vssi_and_wait(uintptr_t entry) {
    /* V=1: csrw stvec actually writes vstvec; MODE=0 (Direct) */
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    /* Arm vsscratch for the trap handler. */
    asm volatile ("csrw sscratch, %0"
                  :: "r"(shvstvecd_trap_scratch) : "memory");
    /* Enable SSIE (V=1: actually vsie.SSIE) */
    asm volatile ("csrs sie, %0" :: "r"(BIT_SSI));
    /* Enable SIE (V=1: actually vsstatus.SIE) — interrupt fires immediately */
    asm volatile ("csrs sstatus, %0" :: "r"(SSTATUS_SIE_BIT));
    /* VSSIP should have been taken by now; the nop gives a safe boundary. */
    asm volatile ("nop");
    /* Disable interrupts for tidiness before returning. */
    asm volatile ("csrc sstatus, %0" :: "r"(SSTATUS_SIE_BIT));
    asm volatile ("csrc sie, %0" :: "r"(BIT_SSI));
    return 0;
}

TEST_REGISTER(test_shvstvecd_int_01_vssip);
bool test_shvstvecd_int_01_vssip(void) {
    TEST_BEGIN("VSTVEC-INT-01: Direct mode, VSSIP lands at BASE (not BASE+offset)");

    uintptr_t entry = (uintptr_t)&shvstvecd_trap_entry;

    /* Delegate VSSIP (bit 2 in hideleg) to VS-mode */
    hyp_delegate_to_vs(0, 1UL << 2);

    shvstvecd_reset_trap_record();

    /* Inject VSSIP pending via hvip */
    hvip_set_vssi(true);

    /* Enter VS-mode: set vstvec and enable interrupt */
    run_in_vs_mode(vsmode_enable_vssi_and_wait, entry);

    /* Direct mode: interrupt also lands at BASE, NOT BASE + 4*cause. */
    TEST_ASSERT("trap PC equals BASE (Direct, not Vectored)",
                g_shvstvecd_trap_pc == entry);
    /* vscause: interrupt bit set, code = 1 (supervisor software interrupt) */
    TEST_ASSERT("vscause is interrupt",
                shvstvecd_is_interrupt(g_shvstvecd_trap_cause));
    TEST_ASSERT("vscause code == 1 (VSSI)",
                shvstvecd_cause_code(g_shvstvecd_trap_cause) == 1);

    /* Cleanup */
    hvip_set_vssi(false);
    hyp_undelegate();
    HYP_TEST_END();
}

/* VSTVEC-DIR-03: load page-fault delegated to VS-mode.
 * We trigger a load page-fault by accessing an unmapped VS-stage
 * address. Since we use run_in_vs_mode (which sets up G-stage identity
 * mapping but no VS-stage translation), with vsatp=Bare there's no
 * VS page fault possible. Instead, we use breakpoint (cause=3) as an
 * alternative sync exception that can be delegated.
 *
 * Actually, let's use a different approach: we access address 0
 * which should be unmapped. But with vsatp=Bare and G-stage identity
 * mapping, address 0 might or might not be mapped.
 *
 * Safest approach: use ebreak (cause=3) which is cleanly delegatable. */

static uintptr_t vsmode_trigger_ebreak(uintptr_t entry) {
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    asm volatile ("csrw sscratch, %0"
                  :: "r"(shvstvecd_trap_scratch) : "memory");
    /* ebreak generates breakpoint exception (cause=3) */
    asm volatile ("ebreak");
    return 0;
}

TEST_REGISTER(test_shvstvecd_dir_03_breakpoint);
bool test_shvstvecd_dir_03_breakpoint(void) {
    TEST_BEGIN("VSTVEC-DIR-03: Direct mode, breakpoint lands at BASE");

#ifdef SKIP_BREAKPOINT_TESTS
    TEST_SKIP("platform does not support breakpoint (SKIP_BREAKPOINT_TESTS)");
#endif

    uintptr_t entry = (uintptr_t)&shvstvecd_trap_entry;

    /* Delegate breakpoint (cause=3) to VS-mode */
    hyp_delegate_to_vs(1UL << 3, 0);

    shvstvecd_reset_trap_record();

    run_in_vs_mode(vsmode_trigger_ebreak, entry);

    TEST_ASSERT("trap PC equals vstvec.BASE",
                g_shvstvecd_trap_pc == entry);
    TEST_ASSERT("vscause == 3 (breakpoint)",
                shvstvecd_cause_code(g_shvstvecd_trap_cause) == 3);
    TEST_ASSERT("vscause is sync exception",
                !shvstvecd_is_interrupt(g_shvstvecd_trap_cause));

    hyp_undelegate();
    HYP_TEST_END();
}
