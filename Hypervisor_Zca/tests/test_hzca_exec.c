/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.1 - HS/VS/VU-mode compressed instruction executability and
 *             cause=22 exclusion (HZCA-01 ~ HZCA-06)
 *
 * Spec basis:
 *   norm:c-lw_op / norm:c-sw_op / norm:c-ld_op / norm:c-sd_op /
 *   norm:c-lwsp_op / norm:c-swsp_op / norm:c-ldsp_op / norm:c-sdsp_op
 *     - compressed load/store expansion, semantics unchanged in VS/VU.
 *   norm:Zca_align16 - IALIGN=16 lets 16/32-bit instructions mix freely.
 *   No virtual-instruction clause in hypervisor.adoc covers a Zca
 *   instruction, so a guest compressed instruction must NEVER report
 *   cause=22 (mandatory negative assertion).
 *
 * All probes are naked and use only registers inside the compressed
 * window (a0..a3 = x10..x13) so the c.* mnemonics assemble to their
 * 16-bit forms; the return value is left in a0 for the trampoline.
 * =================================================================== */

/* HS-mode (V=0) data slot: physical, satp=Bare in HS-mode. */
static volatile uint32_t hzca_hs_slot[4] __attribute__((aligned(16)));

#define HZCA_SEM_MARKER   0x123UL
#define HZCA_DW_MARKER    0x12345678UL
#define HZCA_COMP_RESULT  18UL
#define HZCA_MIX_RESULT   16UL

/* ------------------------------------------------------------------
 * HZCA-01 probe: compressed computational + control-transfer sequence.
 * c.li/c.addi/c.mv/c.add/c.beqz/c.j all retire; returns 18.
 * ------------------------------------------------------------------ */
static uintptr_t hz_comp_jump_probe(uintptr_t arg) __attribute__((naked, aligned(16)));
static uintptr_t hz_comp_jump_probe(uintptr_t arg)
{
    asm volatile (
        ".option push\n\t"
        ".option rvc\n\t"
        "c.li   a0, 4\n\t"        /* a0 = 4                     */
        "c.addi a0, 5\n\t"        /* a0 = 9                     */
        "c.mv   a1, a0\n\t"       /* a1 = 9                     */
        "c.add  a0, a1\n\t"       /* a0 = 18                    */
        "c.beqz a1, 2f\n\t"       /* a1 != 0 -> not taken       */
        "c.j    1f\n\t"           /* unconditional jump to 1f   */
        "2:\n\t"
        "c.li   a0, 0\n\t"        /* (dead path, would zero a0) */
        "1:\n\t"
        ".option pop\n\t"
        "ret\n\t"
        ::: "memory");
}

/* ------------------------------------------------------------------
 * HZCA-02/03 probe: store HZCA_SEM_MARKER via c.sw then load it back
 * via c.lw; returns the loaded word (also observable in memory).
 * ------------------------------------------------------------------ */
static uintptr_t hz_lwsw_semantic(uintptr_t addr) __attribute__((naked, aligned(16)));
static uintptr_t hz_lwsw_semantic(uintptr_t addr)
{
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        "li   a1, 0x123\n\t"      /* marker (a1 = x11)          */
        ".option rvc\n\t"
        "c.sw a1, 0(a0)\n\t"      /* [a0] = a1                  */
        "c.lw a0, 0(a0)\n\t"      /* a0 = sign-ext [a0]         */
        ".option pop\n\t"
        "ret\n\t"
        ::: "memory");
}

/* ------------------------------------------------------------------
 * HZCA-04 probe: stack-pointer-based compressed instructions on the
 * valid guest stack. c.addi16sp adjusts sp, c.swsp/c.lwsp store/load
 * sp-relative, c.addi4spn computes an sp-relative address. Returns the
 * loaded marker; sp is restored before returning.
 * ------------------------------------------------------------------ */
static uintptr_t hz_sp_based_normal(uintptr_t arg) __attribute__((naked, aligned(16)));
static uintptr_t hz_sp_based_normal(uintptr_t arg)
{
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        "li   a1, 0x77\n\t"       /* marker                     */
        ".option rvc\n\t"
        "c.addi16sp sp, -16\n\t"  /* sp -= 16                   */
        "c.swsp a1, 8(sp)\n\t"    /* [sp+8] = a1                */
        "c.lwsp a0, 8(sp)\n\t"    /* a0 = [sp+8]                */
        "c.addi4spn a1, sp, 8\n\t" /* a1 = sp + 8 (address)     */
        "c.addi16sp sp, 16\n\t"   /* restore sp                 */
        ".option pop\n\t"
        "ret\n\t"
        ::: "memory");
}

#if __riscv_xlen == 64
/* ------------------------------------------------------------------
 * HZCA-05 probe: RV64 doubleword compressed load/store. c.sd/c.ld use
 * a register base; c.sdsp/c.ldsp use the (valid) guest stack. Returns
 * the c.ld value; sp is restored.
 * ------------------------------------------------------------------ */
static uintptr_t hz_doubleword_normal(uintptr_t addr) __attribute__((naked, aligned(16)));
static uintptr_t hz_doubleword_normal(uintptr_t addr)
{
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        "li   a1, 0x12345678\n\t" /* 64-bit marker              */
        ".option rvc\n\t"
        "c.sd   a1, 0(a0)\n\t"    /* [a0] = a1 (doubleword)     */
        "c.ld   a2, 0(a0)\n\t"    /* a2 = [a0]                  */
        "c.addi16sp sp, -16\n\t"  /* sp-relative region         */
        "c.sdsp a1, 0(sp)\n\t"    /* [sp] = a1                  */
        "c.ldsp a3, 0(sp)\n\t"    /* a3 = [sp]                  */
        "c.addi16sp sp, 16\n\t"   /* restore sp                 */
        "c.mv   a0, a2\n\t"       /* return the c.ld value      */
        ".option pop\n\t"
        "ret\n\t"
        ::: "memory");
}
#endif /* __riscv_xlen == 64 */

/* ------------------------------------------------------------------
 * HZCA-06 probe: a sequence freely mixing 16-bit compressed and 32-bit
 * instructions (IALIGN=16). Deterministic result 16 in HS and VS.
 * ------------------------------------------------------------------ */
static uintptr_t hz_mixed_seq(uintptr_t arg) __attribute__((naked, aligned(16)));
static uintptr_t hz_mixed_seq(uintptr_t arg)
{
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        "li     a0, 1\n\t"        /* 32-bit      : a0 = 1       */
        ".option rvc\n\t"
        "c.addi a0, 2\n\t"        /* compressed  : a0 = 3       */
        ".option norvc\n\t"
        "addi   a0, a0, 3\n\t"    /* 32-bit      : a0 = 6       */
        ".option rvc\n\t"
        "c.slli a0, 1\n\t"        /* compressed  : a0 = 12      */
        ".option norvc\n\t"
        "addi   a0, a0, 4\n\t"    /* 32-bit      : a0 = 16      */
        ".option pop\n\t"
        "ret\n\t"
        ::: "memory");
}

/* ------------------------------------------------------------------
 * HZCA-01: HS/VS/VU-mode compressed computational + jump instructions
 *          execute normally; VS/VU never report cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_01_exec_all_modes);
bool test_hzca_01_exec_all_modes(void)
{
    TEST_BEGIN("HZCA-01: HS/VS/VU compressed comp+jump execute, no cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* HS-mode (V=0). */
    trap_expect_begin();
    uintptr_t hs_r = run_in_priv(PRIV_S, hz_comp_jump_probe, 0);
    bool hs_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("HS-mode: no exception", !hs_fired);
    TEST_ASSERT_EQ("HS-mode: compressed sequence result == 18",
                   hs_r, HZCA_COMP_RESULT);

    /* VS-mode (V=1). */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    trap_expect_begin();
    uintptr_t vs_r = two_stage_run_in_vs(&ctx, hz_comp_jump_probe, 0);
    bool vs_fired = trap_was_triggered();
    uintptr_t vs_cause = vs_fired ? trap_get_cause() : 0;
    trap_expect_end();
    if (vs_fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)vs_cause);
    TEST_ASSERT("VS-mode: no exception", !vs_fired);
    TEST_ASSERT_NEQ("VS-mode: never virtual-instruction (22)",
                    vs_cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("VS-mode: compressed sequence result == 18",
                   vs_r, HZCA_COMP_RESULT);
    ts2_finish(&ctx);

    /* VU-mode (V=1, U). */
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    trap_expect_begin();
    uintptr_t vu_r = two_stage_run_in_vu(&ctx, hz_comp_jump_probe, 0);
    bool vu_fired = trap_was_triggered();
    uintptr_t vu_cause = vu_fired ? trap_get_cause() : 0;
    trap_expect_end();
    if (vu_fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)vu_cause);
    TEST_ASSERT("VU-mode: no exception", !vu_fired);
    TEST_ASSERT_NEQ("VU-mode: never virtual-instruction (22)",
                    vu_cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("VU-mode: compressed sequence result == 18",
                   vu_r, HZCA_COMP_RESULT);
    ts2_finish(&ctx);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-02: VS-mode c.lw/c.sw semantics identical to non-virtualized
 *          (HS-mode): same loaded value and same final memory.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_02_vs_hs_lwsw_parity);
bool test_hzca_02_vs_hs_lwsw_parity(void)
{
    TEST_BEGIN("HZCA-02: VS-mode c.lw/c.sw semantics == HS-mode");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* HS-mode run on a physical slot. */
    uintptr_t hs_addr = (uintptr_t)&hzca_hs_slot[0];
    *(volatile uint32_t *)hs_addr = 0;
    trap_expect_begin();
    uintptr_t hs_r = run_in_priv(PRIV_S, hz_lwsw_semantic, hs_addr);
    bool hs_fired = trap_was_triggered();
    trap_expect_end();
    uintptr_t hs_mem = *(volatile uint32_t *)hs_addr;
    TEST_ASSERT("HS-mode c.sw/c.lw took no trap", !hs_fired);

    /* VS-mode run on the two-stage-mapped test region. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t vs_addr = (uintptr_t)test_data_area;
    *(volatile uint32_t *)vs_addr = 0;
    trap_expect_begin();
    uintptr_t vs_r = two_stage_run_in_vs(&ctx, hz_lwsw_semantic, vs_addr);
    bool vs_fired = trap_was_triggered();
    trap_expect_end();
    uintptr_t vs_mem = *(volatile uint32_t *)vs_addr;
    ts2_finish(&ctx);

    TEST_ASSERT("VS-mode c.sw/c.lw took no trap", !vs_fired);
    TEST_ASSERT_EQ("HS-mode c.lw returned the stored marker",
                   hs_r, HZCA_SEM_MARKER);
    TEST_ASSERT_EQ("HS-mode memory holds the c.sw marker",
                   hs_mem, HZCA_SEM_MARKER);
    TEST_ASSERT_EQ("VS-mode c.lw result == HS-mode", vs_r, hs_r);
    TEST_ASSERT_EQ("VS-mode memory == HS-mode memory", vs_mem, hs_mem);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-03: VU-mode c.lw/c.sw execute normally, never cause=22.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_03_vu_lwsw_exec);
bool test_hzca_03_vu_lwsw_exec(void)
{
    TEST_BEGIN("HZCA-03: VU-mode c.lw/c.sw execute, never cause=22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint32_t *)va = 0;

    trap_expect_begin();
    uintptr_t r = two_stage_run_in_vu(&ctx, hz_lwsw_semantic, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    uintptr_t mem = *(volatile uint32_t *)va;
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VU-mode c.sw/c.lw took no trap", !fired);
    TEST_ASSERT_NEQ("VU-mode never reports virtual-instruction (22)",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("VU-mode c.lw returned the stored marker",
                   r, HZCA_SEM_MARKER);
    TEST_ASSERT_EQ("VU-mode memory holds the c.sw marker",
                   mem, HZCA_SEM_MARKER);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-04: VS-mode stack-pointer-based compressed memory access
 *          (c.addi4spn/c.lwsp/c.swsp/c.addi16sp) executes normally.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_04_vs_sp_based);
bool test_hzca_04_vs_sp_based(void)
{
    TEST_BEGIN("HZCA-04: VS-mode sp-based compressed access normal");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);

    trap_expect_begin();
    uintptr_t r = two_stage_run_in_vs(&ctx, hz_sp_based_normal, 0);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VS-mode sp-based sequence took no trap", !fired);
    TEST_ASSERT_NEQ("VS-mode sp-based never reports cause=22",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("c.lwsp returned the c.swsp marker (0x77)",
                   r, (uintptr_t)0x77UL);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-05: RV64 doubleword compressed load/store (c.ld/c.sd/c.ldsp/
 *          c.sdsp) execute normally with correct 64-bit semantics.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_05_rv64_doubleword);
bool test_hzca_05_rv64_doubleword(void)
{
    TEST_BEGIN("HZCA-05: RV64 c.ld/c.sd/c.ldsp/c.sdsp normal");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    if (__riscv_xlen != 64) TEST_SKIP("RV64-only compressed doubleword instruction");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint64_t *)va = 0;

    trap_expect_begin();
    uintptr_t r = two_stage_run_in_vs(&ctx, hz_doubleword_normal, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    uintptr_t mem = *(volatile uint64_t *)va;
    ts2_finish(&ctx);

    if (fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)cause);
    TEST_ASSERT("VS-mode doubleword sequence took no trap", !fired);
    TEST_ASSERT_EQ("c.ld returned the c.sd marker",
                   r, HZCA_DW_MARKER);
    TEST_ASSERT_EQ("memory holds the 64-bit c.sd marker",
                   mem, HZCA_DW_MARKER);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-06: VS-mode mixed compressed + 32-bit sequence executes and
 *          matches the non-virtualized result (norm:Zca_align16).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_06_vs_mixed_seq);
bool test_hzca_06_vs_mixed_seq(void)
{
    TEST_BEGIN("HZCA-06: VS-mode mixed 16/32-bit sequence == HS-mode");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* HS-mode baseline. */
    trap_expect_begin();
    uintptr_t hs_r = run_in_priv(PRIV_S, hz_mixed_seq, 0);
    bool hs_fired = trap_was_triggered();
    trap_expect_end();
    TEST_ASSERT("HS-mode mixed sequence took no trap", !hs_fired);

    /* VS-mode run under two-stage translation. */
    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, HZ_VSMODE, HZ_GMODE);
    trap_expect_begin();
    uintptr_t vs_r = two_stage_run_in_vs(&ctx, hz_mixed_seq, 0);
    bool vs_fired = trap_was_triggered();
    uintptr_t vs_cause = vs_fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    if (vs_fired)
        printf("  UNEXPECTED TRAP: cause=%lu\n", (unsigned long)vs_cause);
    TEST_ASSERT("VS-mode mixed sequence took no trap", !vs_fired);
    TEST_ASSERT_EQ("HS-mode mixed result == 16", hs_r, HZCA_MIX_RESULT);
    TEST_ASSERT_EQ("VS-mode mixed result == HS-mode (IALIGN=16 mixing)",
                   vs_r, hs_r);

    HYP_TEST_END();
}
