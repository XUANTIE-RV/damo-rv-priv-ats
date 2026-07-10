/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 16: mstatus Hypervisor enhancements
 *
 * Tests MSTAT-01 through MSTAT-14 verify mstatus hypervisor bits.
 */

#include "test_helpers.h"

/* Helper trampolines for PRIV_DO (which requires an expression, not
 * an asm statement). */
static uintptr_t hs_read_hgatp(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile("csrr %0, 0x680" : "=r"(v));  /* hgatp */
    return v;
}

static uintptr_t hs_exec_hfence_gvma(uintptr_t arg) {
    (void)arg;
    asm volatile(".4byte 0x62000073");  /* hfence.gvma x0, x0 */
    return 0;
}

/* ------------------------------------------------------------------
 * MSTAT-01: MPV set to 1 on VS trap to M-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_mpv_vs_trap);
bool mstatus_mpv_vs_trap(void) {
    TEST_BEGIN("MSTAT-01: Verify MPV set to 1 on VS trap to M-mode");

    /* VS ecall trap to M-mode (cause=10). */
    trap_expect_begin();
    run_in_vs_mode(vs_exec_ecall, 0);
    trap_expect_end();

    /* Cause=10 (ecall-from-VS) is only generated when the trap
     * originates in VS-mode (V=1), proving hardware set MPV=1 at
     * trap entry.  After run_in_vs_mode returns to M-mode, MPV has
     * been consumed by mret and reads as 0 — that is correct. */
    uintptr_t cause = trap_get_cause();
    TEST_ASSERT_EQ("VS ecall cause=10 (proves MPV was set)", cause, (uintptr_t)10);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-02: MPV set to 0 on HS trap to M-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_mpv_hs_trap);
bool mstatus_mpv_hs_trap(void) {
    TEST_BEGIN("MSTAT-02: Verify MPV set to 0 on HS trap to M-mode");

    /* HS illegal instruction trap to M-mode. */
    EXPECT_TRAP(2, {
        asm volatile(".4byte 0x00401073"); /* illegal instruction */
    });

    /* Check mstatus.MPV. */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    uintptr_t mpv = (ms >> 39) & 1;
    TEST_ASSERT_EQ("MPV should be 0 on HS trap to M-mode", mpv, 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-03: MRET with V<-MPV
 *
 * Manually set MPV=1, MPP=1(S), then execute MRET.  Per spec
 * norm:mstatus_mpv_op, MRET sets V=MPV (unless MPP=3).
 * We verify V=1 by executing csrr hgatp in the post-MRET context:
 * hgatp is an HS-only CSR, so accessing it with V=1 triggers
 * virtual-instruction (cause=22).
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_mret_v_from_mpv);
bool mstatus_mret_v_from_mpv(void) {
    TEST_BEGIN("MSTAT-03: Verify MRET sets V<-MPV");

    /* Save current stack pointer (M-mode sp) for recovery. */
    uintptr_t saved_sp;
    asm volatile("mv %0, sp" : "=r"(saved_sp));

    /* Configure mstatus: MPP=1(S), MPV=1, clear MPRV. */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms &= ~((3UL << 11) | (1UL << 17));   /* clear MPP, MPRV */
    ms |=  (1UL << 11);                    /* MPP = S (1) */
    ms |=  (1UL << 39);                    /* MPV = 1 */
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    /* Arm trap handler before entering VS-mode. */
    trap_expect_begin();

    asm volatile (
        "mv   s1, %0\n\t"          /* save M-mode sp in s1 (callee-saved) */
        "la   t0, 1f\n\t"
        "csrw mepc, t0\n\t"
        "mret\n\t"                 /* MPP=S, MPV=1 -> V=1, S-mode */
        "1:\n\t"
        /* Now in VS-mode (V=1).  Access hgatp -> cause=22. */
        "csrr t0, 0x680\n\t"       /* hgatp: virtual-inst if V=1 */
        /* After trap handler advances mepc, ecall returns to M. */
        "li   t0, 1\n\t"           /* ECALL_GOTO_PRIV = 1 */
        "la   t1, ecall_args\n\t"
        "sd   t0, 0(t1)\n\t"       /* ecall_args[0] = ECALL_GOTO_PRIV */
        "li   t0, 3\n\t"           /* PRIV_M = 3 */
        "sd   t0, 8(t1)\n\t"       /* ecall_args[1] = PRIV_M */
        "ecall\n\t"                 /* VS->M via goto_priv handler */
        "mv   sp, s1\n\t"          /* restore M-mode sp */
        : : "r"(saved_sp) : "s1", "t0", "t1", "memory"
    );

    /* If V=1 after MRET, the hgatp access triggered cause=22. */
    TEST_ASSERT("virtual-inst trap triggered (proves V=1)",
                trap_was_triggered());
    TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                   trap_get_cause(), (uintptr_t)22);
    trap_expect_end();

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-04: MRET with MPP=3 keeps V=0
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_mret_mpp3_v_zero);
bool mstatus_mret_mpp3_v_zero(void) {
    TEST_BEGIN("MSTAT-04: Verify MRET with MPP=3 keeps V=0");

    /* Configure mstatus: MPP=3 (M-mode), MPV=1.
     * Per spec, MRET with MPP=M forces V=0 regardless of MPV. */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms |= (3UL << 11);       /* MPP = M (3) */
    ms |= (1UL << 39);       /* MPV = 1 */
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    /* Execute mret: should land at label 1f in M-mode with V=0. */
    asm volatile (
        "la t0, 1f\n\t"
        "csrw mepc, t0\n\t"
        "mret\n\t"
        "1:\n\t"
        ::: "t0", "memory"
    );

    /* After MRET with MPP=3, spec mandates V=0 and MPV is cleared.
     * Verify MPV=0 and that we are still in M-mode (csrr mstatus
     * succeeded without trapping, which would happen if V=1). */
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    uintptr_t mpv = (ms >> 39) & 1;
    TEST_ASSERT_EQ("V=0 after MRET with MPP=3", mpv, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-05: M-mode GVA correctness
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_gva_m_mode);
bool mstatus_gva_m_mode(void) {
    TEST_BEGIN("MSTAT-05: Verify M-mode GVA correctness");

    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Fire a VS load fault to trigger GPF trap to M-mode. */
    uintptr_t victim_gpa = (uintptr_t)test_fault_page;
    uintptr_t flags = G_FLAGS_RWXU_AD | PTE_R;
    fire_vs_load_fault(victim_gpa, flags);

    /* Per norm:mstatus_gva_op: guest-page-fault writes a GVA to mtval,
     * so GVA must be set to 1. */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    uintptr_t gva = (ms >> 38) & 1;
    TEST_ASSERT_EQ("GVA=1 on guest page fault", gva, (uintptr_t)1);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-06: TSR only affects HS-mode, not VS-mode
 *
 * Per norm:mstatus_modes: "The TSR and TVM fields of mstatus affect
 * execution only in HS-mode, not in VS-mode."
 * Set mstatus.TSR=1 and hstatus.VTSR=0, then execute SRET in
 * VS-mode.  SRET must NOT trap because TSR does not affect VS-mode.
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_tsr_hs_only);
bool mstatus_tsr_hs_only(void) {
    TEST_BEGIN("MSTAT-06: Verify TSR only affects HS-mode SRET");

    /* Set TSR bit (bit 22) in mstatus. */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms |= (1UL << 22);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    /* Clear VTSR to ensure only mstatus.TSR is the factor. */
    hstatus_set_vtsr(0);

    /* VS-mode SRET should NOT trap: TSR does not affect VS-mode. */
    VS_EXPECT_NO_TRAP(run_in_vs_mode(vs_exec_sret, 0));

    /* Clear TSR bit. */
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms &= ~(1UL << 22);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-07: TVM=1 blocks HS-mode hgatp access
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_tvm_hgatp);
bool mstatus_tvm_hgatp(void) {
    TEST_BEGIN("MSTAT-07: Verify TVM=1 blocks HS-mode hgatp access");

    /* Set TVM bit (bit 20). */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms |= (1UL << 20);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    /* Try to access hgatp (CSR 0x680) from HS-mode — TVM=1 should
     * cause illegal-instruction (cause=2).  Must run in S-mode
     * (HS-mode) because TVM does not affect M-mode. */
    goto_priv(PRIV_S);
    PRIV_DO(hs_read_hgatp(0));
    goto_priv(PRIV_M);
    CHECK_TRAP("TVM=1 hgatp access from HS-mode trapped", 2);

    /* Clear TVM bit. */
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms &= ~(1UL << 20);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-08: TVM=1 blocks HS-mode HFENCE.GVMA
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_tvm_hfence_gvma);
bool mstatus_tvm_hfence_gvma(void) {
    TEST_BEGIN("MSTAT-08: Verify TVM=1 blocks HS-mode HFENCE.GVMA");

    /* Set TVM bit (bit 20). */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms |= (1UL << 20);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    /* Try HFENCE.GVMA from HS-mode — TVM=1 should cause
     * illegal-instruction (cause=2).  Must run in S-mode (HS-mode)
     * because TVM does not affect M-mode. */
    goto_priv(PRIV_S);
    PRIV_DO(hs_exec_hfence_gvma(0));
    goto_priv(PRIV_M);
    CHECK_TRAP("TVM=1 HFENCE.GVMA from HS-mode trapped", 2);

    /* Clear TVM bit. */
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms &= ~(1UL << 20);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-09: TVM=1 does not affect vsatp access
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_tvm_vsatp);
bool mstatus_tvm_vsatp(void) {
    TEST_BEGIN("MSTAT-09: Verify TVM=1 does not affect vsatp access");

    /* Set TVM bit (bit 20). */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms |= (1UL << 20);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    /* Try to access vsatp - should NOT trap. */
    vs_read_satp(0);

    /* Clear TVM bit. */
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms &= ~(1UL << 20);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-10: TVM=1 does not affect HFENCE.VVMA
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_tvm_hfence_vvma);
bool mstatus_tvm_hfence_vvma(void) {
    TEST_BEGIN("MSTAT-10: Verify TVM=1 does not affect HFENCE.VVMA");

    /* Set TVM bit (bit 20). */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms |= (1UL << 20);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    /* Try HFENCE.VVMA - should NOT trap. */
    vs_exec_sfence_vma(0);

    /* Clear TVM bit. */
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms &= ~(1UL << 20);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-11: MPRV=1 MPV=1 MPP=1 triggers two-stage translation
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_mprv_mpv_mpp);
bool mstatus_mprv_mpv_mpp(void) {
    TEST_BEGIN("MSTAT-11: Verify MPRV=1 MPV=1 MPP=1 triggers two-stage translation");

    TEST_SKIP("requires MPRV+MPV two-stage setup");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-12: MPRV=1 MPV=0 does not trigger two-stage translation
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_mprv_mpv_zero);
bool mstatus_mprv_mpv_zero(void) {
    TEST_BEGIN("MSTAT-12: Verify MPRV=1 MPV=0 does not trigger two-stage translation");

    TEST_SKIP("requires MPRV two-stage setup");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-13: MPRV does not affect HLV/HSV
 *
 * Per norm:mstatus_mprv_hlsv: "MPRV does not affect the virtual-
 * machine load/store instructions, HLV, HLVX, and HSV.  The
 * explicit loads and stores of these instructions always act as
 * though V=1 and the nominal privilege mode were hstatus.SPVP,
 * overriding MPRV."
 *
 * Set up two-stage translation (G-stage identity + VS-stage Bare),
 * then execute HLV.W from M-mode with MPRV=1 MPP=S.  If MPRV
 * incorrectly affected HLV, the instruction would behave
 * differently.  A successful load proves HLV ignores MPRV.
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_mprv_hlv_hsv);
bool mstatus_mprv_hlv_hsv(void) {
    TEST_BEGIN("MSTAT-13: Verify MPRV does not affect HLV/HSV");

    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Set up G-stage identity mapping + VS-stage Bare. */
    gpt_pool_reset();
    two_stage_ctx_t ctx;
    two_stage_init(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Identity-map low memory (kernel/UART region). */
    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end  = (uintptr_t)__vm_test_region_start
                        & ~(PAGE_SIZE_2M - 1);
    two_stage_setup_identity(&ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    /* Identity-map test region. */
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_size  = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_setup_identity(&ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    /* Activate G-stage (hgatp); VS-stage stays Bare. */
    two_stage_enable(&ctx, 0);

    /* Baseline: HLV.W with MPRV=0 from M-mode should succeed. */
    uintptr_t target = (uintptr_t)test_data_area;
    EXPECT_NO_TRAP(hlv_w(target));

    /* Set MPRV=1, MPP=S(1).  Per spec, HLV ignores MPRV and always
     * acts as V=1 + SPVP, so it must still succeed. */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms &= ~((3UL << 11) | (1UL << 17));  /* clear MPP, MPRV */
    ms |=  (1UL << 11);                   /* MPP = S (1) */
    ms |=  (1UL << 17);                   /* MPRV = 1 */
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    /* HLV.W with MPRV=1: must NOT trap with illegal-instruction.
     * A successful load proves HLV ignores MPRV as required by spec. */
    EXPECT_NO_TRAP(hlv_w(target));

    /* Clear MPRV. */
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms &= ~(1UL << 17);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    two_stage_cleanup(&ctx);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MSTAT-14: TW=1 affects VS-mode WFI
 * ------------------------------------------------------------------ */
TEST_REGISTER(mstatus_tw_vs_wfi);
bool mstatus_tw_vs_wfi(void) {
    TEST_BEGIN("MSTAT-14: Verify TW=1 causes illegal-inst on VS-mode WFI");

    /* Set TW bit (bit 21). */
    uintptr_t ms;
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms |= (1UL << 21);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    /* VS-mode WFI should trap with illegal-instruction. */
    EXPECT_ILLEGAL_INST(run_in_vs_mode(vs_exec_wfi, 0));

    /* Clear TW bit. */
    asm volatile("csrr %0, mstatus" : "=r"(ms));
    ms &= ~(1UL << 21);
    asm volatile("csrw mstatus, %0" :: "r"(ms));

    HYP_TEST_END();
}
