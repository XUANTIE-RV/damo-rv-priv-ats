/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.8 - IALIGN=16 and instruction-address-misaligned exclusion
 *             (HZCA-25 ~ HZCA-27)
 *
 * Spec basis:
 *   norm:Zca_align16 - Zca sets IALIGN=16: 16-bit instructions may be
 *     freely intermixed with 32-bit instructions, and a 32-bit
 *     instruction may start on any 16-bit boundary. This relaxation also
 *     holds when a VS/VU-mode instruction fetch undergoes VS-stage +
 *     G-stage two-stage translation.
 *   norm:Zca_no_misaligned - with Zca no instruction raises an
 *     instruction-address-misaligned exception (cause=0). So jumping to a
 *     2-byte-aligned (not 4-byte-aligned) address must NOT report cause=0.
 *
 * cause=0 is a mandatory negative assertion: a guest fetch reporting it
 * violates norm:Zca_no_misaligned and must remain FAIL.
 *
 * The jump uses test_vs_exec_expect_fault (recovery-anchor pattern): it
 * sets ra to a recovery label and jr's to the target, so a target that
 * simply `ret`s returns cleanly on success, while any fault is captured
 * and recovered in one shot.
 * =================================================================== */

/* Place a compressed `ret` (c.jr ra = 0x8082) at a 2-byte-aligned
 * address inside test_exec_page so a jump there returns to the caller. */
static void hzca_place_c_ret_at(uintptr_t addr)
{
    *(volatile uint16_t *)addr = 0x8082UL;   /* c.jr ra */
    asm volatile ("fence.i" ::: "memory");
}

/* Place a 32-bit nop (addi x0,x0,0 = 0x00000013) starting at a
 * 2-byte-aligned address, followed by a compressed ret, written as
 * halfwords so no misaligned store is performed. */
static void hzca_place_nop32_then_ret_at(uintptr_t addr)
{
    volatile uint16_t *h = (volatile uint16_t *)addr;
    h[0] = 0x0013UL;   /* low half of 0x00000013  (bits[1:0]=11 -> 32-bit) */
    h[1] = 0x0000UL;   /* high half                                       */
    h[2] = 0x8082UL;   /* c.jr ra at addr+6                               */
    asm volatile ("fence.i" ::: "memory");
}

/* ------------------------------------------------------------------
 * HZCA-25: VS-mode jumps to a 2-byte-aligned address to execute a
 *          compressed instruction; must not report cause=0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_25_vs_jump_2byte_aligned);
bool test_hzca_25_vs_jump_2byte_aligned(void)
{
    TEST_BEGIN("HZCA-25: VS-mode jump to 2-byte-aligned compressed, no cause=0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t target = (uintptr_t)test_exec_page + 2;  /* 2-aligned, not 4 */
    hzca_place_c_ret_at(target);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_exec_expect_fault, target);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("VS-mode executed at a 2-byte-aligned address (no trap)",
                !fired);
    if (fired) {
        /* A guest fetch reporting cause=0 violates norm:Zca_no_misaligned
         * and must be reported, not relaxed. cause defaults to 0 when no
         * trap fired, so these verdicts apply only when one occurred. */
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
        TEST_ASSERT_NEQ("never instruction-address-misaligned (cause=0)",
                        cause, (uintptr_t)CAUSE_INST_ADDR_MISALIGN);
        TEST_ASSERT_NEQ("never virtual-instruction (cause=22)",
                        cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-26: VU-mode jumps to a 2-byte-aligned address; no cause=0, no
 *          cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_26_vu_jump_2byte_aligned);
bool test_hzca_26_vu_jump_2byte_aligned(void)
{
    TEST_BEGIN("HZCA-26: VU-mode jump to 2-byte-aligned address, no cause=0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t target = (uintptr_t)test_exec_page + 2;
    hzca_place_c_ret_at(target);

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);

    trap_expect_begin();
    (void)two_stage_run_in_vu(&ctx, test_vs_exec_expect_fault, target);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("VU-mode executed at a 2-byte-aligned address (no trap)",
                !fired);
    if (fired) {
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
        TEST_ASSERT_NEQ("never instruction-address-misaligned (cause=0)",
                        cause, (uintptr_t)CAUSE_INST_ADDR_MISALIGN);
        TEST_ASSERT_NEQ("never virtual-instruction (cause=22)",
                        cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-27: IALIGN=16 lets a 32-bit instruction start on a 16-bit
 *          boundary (2-byte-aligned, not 4-byte-aligned).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_27_32bit_on_16bit_boundary);
bool test_hzca_27_32bit_on_16bit_boundary(void)
{
    TEST_BEGIN("HZCA-27: 32-bit instruction on a 16-bit boundary, no cause=0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* A 32-bit nop starting at test_exec_page+2 (odd 16-bit boundary). */
    uintptr_t target = (uintptr_t)test_exec_page + 2;
    hzca_place_nop32_then_ret_at(target);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_exec_expect_fault, target);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("VS-mode fetched a 32-bit inst on a 16-bit boundary",
                !fired);
    if (fired) {
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
        TEST_ASSERT_NEQ("never instruction-address-misaligned (cause=0)",
                        cause, (uintptr_t)CAUSE_INST_ADDR_MISALIGN);
    }

    HYP_TEST_END();
}
