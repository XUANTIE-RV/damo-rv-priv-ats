/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_trap.c - Group 8: virtualized trap x pointer masking
 *
 * HZPM-TRAP-01 ~ HZPM-TRAP-06
 *
 * See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 8.
 *
 * Spec references:
 *   - norm:pm_csr_hw_apply: hardware writes to CSRs (vstval/stval on
 *     exception) carry the PM-transformed address.
 *   - norm:pm_no_trap_vector_mask: trap delivery does NOT apply PM
 *     to the trap handler address (vstvec/stvec).
 *   - norm:pm_no_csr_sw: software CSR writes are not transformed.
 *   - norm:pm_not_apply_implicit: instruction fetch is not masked.
 *   - norm:pm_mxr_exception: MXR at the effective privilege mode
 *     suppresses PM.
 */

/* ===================================================================
 * VS-mode trap handler (Direct mode) for delegated exceptions
 *
 * Records vscause/vstval into globals, advances vsepc past the
 * faulting instruction, then sret. vsstatus.SPP is left untouched:
 * the hardware sets SPP to the origin mode on trap entry, so sret
 * returns to the correct mode.
 * =================================================================== */

static volatile uintptr_t g_vs_cause;
static volatile uintptr_t g_vs_tval;
static volatile bool      g_vs_triggered;

static void hzpm_vs_trap_handler(void) __attribute__((naked, aligned(4)));
static void hzpm_vs_trap_handler(void) {
    asm volatile (
        "addi   sp, sp, -32\n\t"
        "sd     t0, 0(sp)\n\t"
        "sd     t1, 8(sp)\n\t"
        "sd     t2, 16(sp)\n\t"

        "csrr   t0, scause\n\t"     /* vscause when V=1 */
        "csrr   t1, stval\n\t"      /* vstval when V=1 */
        "la     t2, g_vs_cause\n\t"
        "sd     t0, 0(t2)\n\t"
        "la     t2, g_vs_tval\n\t"
        "sd     t1, 0(t2)\n\t"
        "la     t2, g_vs_triggered\n\t"
        "li     t0, 1\n\t"
        "sb     t0, 0(t2)\n\t"

        /* Advance vsepc past the faulting instruction (4-byte) */
        "csrr   t0, sepc\n\t"
        "addi   t0, t0, 4\n\t"
        "csrw   sepc, t0\n\t"

        "ld     t0, 0(sp)\n\t"
        "ld     t1, 8(sp)\n\t"
        "ld     t2, 16(sp)\n\t"
        "addi   sp, sp, 32\n\t"
        "sret\n\t"
    );
}

static void hzpm_vs_handler_install(void) {
    g_vs_cause = 0;
    g_vs_tval = 0;
    g_vs_triggered = false;
    vs_trap_setup_direct((uintptr_t)hzpm_vs_trap_handler);
}

/* VS-mode payload: plain 4-byte ebreak (breakpoint, cause=3).
 * Forced non-compressed so the handler's vsepc+4 skip lands on the
 * next instruction. Breakpoint is used instead of ecall because the
 * run_in_vs/vu_mode return path relies on ecall reaching M-mode
 * (the framework force-clears hedeleg[10] for that purpose). */
static uintptr_t hzpm_vs_ebreak(uintptr_t unused) {
    (void)unused;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ebreak\n\t"
        ".option pop\n\t"
        ::: "memory");
    return 0;
}

/* VS-mode payload: ebreak with an _exec_return_addr recovery
 * anchor. Used when the trap delivery itself is expected to fail
 * (tagged vstvec): the M-mode armed-trap branch redirects mepc to
 * the anchor, escaping the faulting fetch in one shot. */
static uintptr_t hzpm_vs_ebreak_recovery(uintptr_t unused) {
    (void)unused;
    extern uintptr_t _exec_return_addr;
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        "la   t0, 1f\n\t"
        "la   t1, _exec_return_addr\n\t"
        "sd   t0, 0(t1)\n\t"
        "ebreak\n\t"
        "1:\n\t"
        "sd   zero, 0(t1)\n\t"
        ".option pop\n\t"
        :: : "t0", "t1", "memory"
    );
    return 0;
}

/* ----- HZPM-TRAP-01: vstval holds transformed address ----- */
TEST_REGISTER(test_hzpm_trap_01);
bool test_hzpm_trap_01(void) {
    TEST_BEGIN("HZPM-TRAP-01: vstval contains transformed address");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    /* Delegate load page-fault (cause=13) to VS-mode */
    hyp_delegate_to_vs(1ULL << CAUSE_LOAD_PAGE_FAULT, 0);
    hzpm_vs_handler_install();

    /* Tagged VA whose *transformed* address is valid but unmapped in
     * VS-stage. The fault is taken after the ignore transformation,
     * so vstval must hold the transformed (sign-extended) address
     * (norm:pm_csr_hw_apply). */
    uintptr_t tagged = pm_tag_address(HZPM_UNMAPPED, 0x55, 7);
    uintptr_t expected = pm_transform_va(tagged, 7);

    two_stage_run_in_vs(&ctx, hzpm_load64, tagged);

    TEST_ASSERT("VS trap taken", g_vs_triggered);
    TEST_ASSERT_EQ("vscause=13 (load page-fault)",
                   g_vs_cause, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT_EQ("vstval == PM-transformed address",
                   g_vs_tval, expected);

    hyp_undelegate();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-TRAP-02: stval holds transformed GVA (guest-page-fault) ----- */
TEST_REGISTER(test_hzpm_trap_02);
bool test_hzpm_trap_02(void) {
    TEST_BEGIN("HZPM-TRAP-02: stval contains transformed GVA");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Tagged GPA whose *transformed* (zero-extended) address is
     * unmapped in G-stage -> guest-page-fault to HS/M. stval must
     * hold the transformed GPA (norm:pm_csr_hw_apply +
     * norm:pm_ignore_pa). */
    uintptr_t tagged = pm_tag_address(HZPM_UNMAPPED, 0x55, 7);
    uintptr_t expected = pm_transform_pa(tagged, 7);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT("guest-page-fault triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=21 (load guest-page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
        TEST_ASSERT_EQ("stval == PM-transformed GPA",
                       trap_get_tval(), expected);
        CHECK_GVA("GVA=1 for GPA-writing trap", 1);
    }
    trap_expect_end();

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-TRAP-03: trap delivery not affected by PM ----- */
TEST_REGISTER(test_hzpm_trap_03);
bool test_hzpm_trap_03(void) {
    TEST_BEGIN("HZPM-TRAP-03: trap delivery to vstvec not masked");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    /* No translation needed: Bare/Bare, PMP is wide open. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_BARE);

    /* Delegate breakpoint (cause=3) to VS-mode */
    hyp_delegate_to_vs(1ULL << CAUSE_BREAKPOINT, 0);
    hzpm_vs_handler_install();

    /* norm:pm_no_trap_vector_mask: with PM enabled, trap delivery
     * must still reach the exact vstvec address. If PM were applied
     * to the vector, the handler would not run. */
    two_stage_run_in_vs(&ctx, hzpm_vs_ebreak, 0);

    TEST_ASSERT("VS handler executed (delivery not masked)",
                g_vs_triggered);
    TEST_ASSERT_EQ("vscause=3 (breakpoint)",
                   g_vs_cause, (uintptr_t)CAUSE_BREAKPOINT);

    hyp_undelegate();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-TRAP-04: tagged vstvec is NOT transformed ----- */
TEST_REGISTER(test_hzpm_trap_04);
bool test_hzpm_trap_04(void) {
    TEST_BEGIN("HZPM-TRAP-04: tagged vstvec used verbatim");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Delegate breakpoint (cause=3) to VS-mode */
    hyp_delegate_to_vs(1ULL << CAUSE_BREAKPOINT, 0);
    hzpm_vs_handler_install();

    /* norm:pm_no_csr_sw allows software to store a tagged address
     * into vstvec; norm:pm_no_trap_vector_mask requires delivery to
     * use it verbatim (no ignore transformation). The fetch at the
     * tagged GPA then faults: instruction guest-page-fault (20)
     * with stval == raw tagged address. */
    uintptr_t handler = (uintptr_t)hzpm_vs_trap_handler;
    uintptr_t tagged_handler = pm_tag_address(handler, 0x55, 7);
    vstvec_write(tagged_handler & VSTVEC_BASE_MASK);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, hzpm_vs_ebreak_recovery, 0);
    TEST_ASSERT("fetch fault at tagged vstvec", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=20 (inst guest-page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_INST_GUEST_PAGE_FAULT);
        TEST_ASSERT_EQ("stval == raw tagged vstvec (not transformed)",
                       trap_get_tval(),
                       tagged_handler & VSTVEC_BASE_MASK);
    }
    trap_expect_end();
    TEST_ASSERT("VS handler must NOT run at transformed address",
                !g_vs_triggered);

    hyp_undelegate();
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-TRAP-05: vsstatus.MXR suppresses PM in VS-mode ----- */
TEST_REGISTER(test_hzpm_trap_05);
bool test_hzpm_trap_05(void) {
    TEST_BEGIN("HZPM-TRAP-05: MXR suppresses PM in VS-mode");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    /* norm:pm_mxr_exception: when MXR is in effect at the effective
     * privilege mode, pointer masking does not apply. With
     * vsstatus.MXR=1 the tagged VA is used verbatim -> invalid Sv39
     * VA -> load page-fault (13). */
    ts2_set_vsstatus_mxr(1);

    uintptr_t tagged = pm_tag_address(HZPM_DATA1, pm_max_tag(7), 7);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, hzpm_load64, tagged);
    TEST_ASSERT("trap triggered (PM suppressed by MXR)",
                trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=13 (load page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    }
    trap_expect_end();

    ts2_set_vsstatus_mxr(0);
    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ----- HZPM-TRAP-06: instruction fetch not masked in VS-mode ----- */
TEST_REGISTER(test_hzpm_trap_06);
bool test_hzpm_trap_06(void) {
    TEST_BEGIN("HZPM-TRAP-06: instruction fetch not masked (VS-mode)");
    SSNPM_HYP_REQUIRED_OR_SKIP();
    REQUIRE_VSATP_SV39();
    if (!hzpm_try_set_vs_pmm(PMM_PMLEN7))
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, HGATP_MODE_BARE);

    /* norm:pm_not_apply_implicit: PM does not apply to instruction
     * fetches. Jumping to a tagged function address must fault
     * (invalid Sv39 VA) even though PM is enabled for VS-mode. */
    uintptr_t tagged_fn = pm_tag_address((uintptr_t)hzpm_load64, 0x55, 7);

    trap_expect_begin();
    uintptr_t cause = two_stage_run_in_vs(&ctx, test_vs_exec_expect_fault,
                                          tagged_fn);
    trap_expect_end();

    TEST_ASSERT_EQ("cause=12 (inst page-fault) for tagged fetch",
                   cause, (uintptr_t)CAUSE_INST_PAGE_FAULT);

    ts2_finish(&ctx);
    HYP_TEST_END();
}
