/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/tests_2stage/test_group13_hlv_hsv.c
 *
 * Group 13 - HLV / HLVX / HSV instructions (TS-HLV-01..12)
 *
 * Spec basis (priv-1.12):
 *   - HLV/HLVX/HSV are valid in M-mode and HS-mode; valid in U-mode
 *     only when hstatus.HU=1.
 *   - They always use two-stage translation (vsatp + hgatp).
 *   - Effective privilege is controlled by hstatus.SPVP:
 *       0 -> VU-mode (U-level VS-stage check)
 *       1 -> VS-mode (S-level VS-stage check)
 *   - HS-level sstatus.SUM is *ignored*; vsstatus.SUM controls VS-stage
 *     U-bit override when SPVP=1.
 *   - HS-level sstatus.MXR overrides X-only on BOTH stages.
 *   - vsstatus.MXR only overrides X-only on the VS stage.
 *   - HLVX uses execute permission instead of read permission for the
 *     VS-stage check.
 *   - V=1 -> virtual-instruction (cause=22).
 *   - U-mode + HU=0 -> illegal-instruction (cause=2).
 *
 * Notes on environment:
 *   The HLV/HSV instructions execute in HS-mode; the framework's
 *   ts2_setup_full programs vsatp/hgatp without entering V=1, so HLV
 *   accesses guest VAs while the executing code stays in HS-mode.
 * =================================================================== */

#ifndef SUITE_HGATP_MODE
#error "SUITE_HGATP_MODE must be defined before including this file"
#endif
#ifndef SUITE_VSATP_MODE
#error "SUITE_VSATP_MODE must be defined before including this file"
#endif

#include "hyp_ldst.h"

#define G13_GMODE   SUITE_HGATP_MODE
#define G13_VSMODE  SUITE_VSATP_MODE

#define G13_VS_RWX  (PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D)
#define G13_VS_RWXU (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)
#define G13_VS_X    (PTE_V|PTE_X|PTE_A|PTE_D)
#define G13_VS_XU   (PTE_V|PTE_X|PTE_U|PTE_A|PTE_D)

#define G13_G_RWXU  (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)
#define G13_G_XU    (PTE_V|PTE_X|PTE_U|PTE_A|PTE_D)

extern uintptr_t run_in_priv(unsigned priv,
                             uintptr_t (*fn)(uintptr_t), uintptr_t arg);

/* file-scope state passed to U-mode runners */
static volatile uintptr_t g13_target_va;
static volatile uint64_t  g13_load_value;
static volatile int       g13_use_hu_path;

/* HS-mode wrappers used via run_in_priv when needed. */
static uintptr_t g13_u_hlv_d_for_hu(uintptr_t arg) {
    long result;
    asm volatile (
        ".insn r 0x73, 0x4, 0x36, %0, %1, x0"
        : "=r"(result) : "r"(arg) : "memory");
    return (uintptr_t)result;
}


/* ===================================================================
 * TS-HLV-01: HS-mode HLV.D reads guest memory.
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_01_hs_load);
bool test_ts_hlv_01_hs_load(void) {
    TEST_BEGIN("TS-HLV-01: HS-mode HLV.D -> reads guest VA");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;

    /* Plant a known value at the SPA (identity-mapped). */
    *(volatile uint64_t *)va = 0xDEADBEEF12345678ULL;

    ts2_setup_full(&ctx, G13_VSMODE, G13_GMODE);
    /* Mark VS leaf as VS-level (SPVP will pick S/U). */
    two_stage_enable(&ctx, /*vmid*/0);
    /* SPVP=1 (VS-mode). */
    hstatus_set_spvp(PRIV_S);

    trap_expect_begin();
    uint64_t v = hlv_d(va);
    bool fired = trap_was_triggered();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("no trap on HLV.D", !fired);
    TEST_ASSERT_EQ("HLV.D returned planted value",
                   v, 0xDEADBEEF12345678ULL);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-02: HS-mode HSV.D writes guest memory.
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_02_hs_store);
bool test_ts_hlv_02_hs_store(void) {
    TEST_BEGIN("TS-HLV-02: HS-mode HSV.D -> writes guest VA");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    *(volatile uint64_t *)va = 0;

    ts2_setup_full(&ctx, G13_VSMODE, G13_GMODE);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);

    trap_expect_begin();
    hsv_d(va, 0xCAFEBABE0BADF00DULL);
    bool fired = trap_was_triggered();
    trap_expect_end();
    uint64_t v = *(volatile uint64_t *)va;
    ts2_finish(&ctx);

    TEST_ASSERT("no trap on HSV.D", !fired);
    TEST_ASSERT_EQ("guest memory contains stored value",
                   v, 0xCAFEBABE0BADF00DULL);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-03: SPVP=0 (VU effective), VS PTE U=0 -> page fault.
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_03_spvp0_u0);
bool test_ts_hlv_03_spvp0_u0(void) {
    TEST_BEGIN("TS-HLV-03: SPVP=0 + VS U=0 -> page fault");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    /* dual: VS U=0 RWX, G U=1 RWX. */
    ts2_setup_with_dual_victim(&ctx, G13_VSMODE, G13_GMODE,
                               va, G13_VS_RWX, G13_G_RWXU);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_U);   /* effective VU */

    trap_expect_begin();
    (void)hlv_d(va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    /* restore SPVP=S to keep state clean for next test */
    hstatus_set_spvp(PRIV_S);
    ts2_finish(&ctx);

    TEST_ASSERT("HLV.D trapped", fired);
    TEST_ASSERT_EQ("cause = load-page-fault (13)",
                   cause, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-04: SPVP=1 (VS effective), VS PTE U=0 -> success.
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_04_spvp1_u0);
bool test_ts_hlv_04_spvp1_u0(void) {
    TEST_BEGIN("TS-HLV-04: SPVP=1 + VS U=0 -> success");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    *(volatile uint64_t *)va = 0x4040404040404040ULL;

    ts2_setup_with_dual_victim(&ctx, G13_VSMODE, G13_GMODE,
                               va, G13_VS_RWX, G13_G_RWXU);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);   /* effective VS */

    trap_expect_begin();
    uint64_t v = hlv_d(va);
    bool fired = trap_was_triggered();
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("no trap", !fired);
    TEST_ASSERT_EQ("loaded planted value",
                   v, 0x4040404040404040ULL);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-05: HS-level sstatus.SUM is ignored; vsstatus.SUM controls.
 *   - SPVP=1 (VS-level, S in VS), VS PTE U=1
 *   - sstatus.SUM=0, vsstatus.SUM=1 -> success
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_05_sum_via_vsstatus);
bool test_ts_hlv_05_sum_via_vsstatus(void) {
    TEST_BEGIN("TS-HLV-05: vsstatus.SUM controls SPVP=1 access to U=1");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    *(volatile uint64_t *)va = 0x5050505050505050ULL;

    ts2_setup_with_dual_victim(&ctx, G13_VSMODE, G13_GMODE,
                               va, G13_VS_RWXU, G13_G_RWXU);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);

    /* Clear sstatus.SUM, set vsstatus.SUM. */
    asm volatile ("csrc sstatus, %0" :: "r"((uintptr_t)SSTATUS_SUM));
    asm volatile ("csrs 0x200, %0" :: "r"((uintptr_t)SSTATUS_SUM));

    trap_expect_begin();
    uint64_t v = hlv_d(va);
    bool fired = trap_was_triggered();
    trap_expect_end();

    /* Restore vsstatus.SUM=0. */
    asm volatile ("csrc 0x200, %0" :: "r"((uintptr_t)SSTATUS_SUM));
    ts2_finish(&ctx);

    TEST_ASSERT("HLV.D succeeded with vsstatus.SUM=1", !fired);
    TEST_ASSERT_EQ("loaded planted value",
                   v, 0x5050505050505050ULL);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-06: sstatus.MXR=1 makes VS-stage X-only readable via HLV.
 *   - VS X-only (R=0,X=1), G RWXU, SPVP=1
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_06_sstatus_mxr_vs);
bool test_ts_hlv_06_sstatus_mxr_vs(void) {
    TEST_BEGIN("TS-HLV-06: sstatus.MXR overrides VS-stage X-only");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    *(volatile uint64_t *)va = 0x6060606060606060ULL;

    ts2_setup_with_dual_victim(&ctx, G13_VSMODE, G13_GMODE,
                               va, G13_VS_X, G13_G_RWXU);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);

    asm volatile ("csrs sstatus, %0" :: "r"((uintptr_t)SSTATUS_MXR));

    trap_expect_begin();
    uint64_t v = hlv_d(va);
    bool fired = trap_was_triggered();
    trap_expect_end();

    asm volatile ("csrc sstatus, %0" :: "r"((uintptr_t)SSTATUS_MXR));
    ts2_finish(&ctx);

    TEST_ASSERT("HLV.D succeeded with sstatus.MXR=1 on VS X-only",
                !fired);
    TEST_ASSERT_EQ("loaded planted value", v, 0x6060606060606060ULL);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-07: sstatus.MXR=1 also makes G-stage X-only readable.
 *   - VS RWX, G X-only U
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_07_sstatus_mxr_g);
bool test_ts_hlv_07_sstatus_mxr_g(void) {
    TEST_BEGIN("TS-HLV-07: sstatus.MXR overrides G-stage X-only");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    *(volatile uint64_t *)va = 0x7070707070707070ULL;

    ts2_setup_with_dual_victim(&ctx, G13_VSMODE, G13_GMODE,
                               va, G13_VS_RWX, G13_G_XU);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);

    asm volatile ("csrs sstatus, %0" :: "r"((uintptr_t)SSTATUS_MXR));

    trap_expect_begin();
    uint64_t v = hlv_d(va);
    bool fired = trap_was_triggered();
    trap_expect_end();

    asm volatile ("csrc sstatus, %0" :: "r"((uintptr_t)SSTATUS_MXR));
    ts2_finish(&ctx);

    TEST_ASSERT("HLV.D succeeded with sstatus.MXR=1 on G X-only",
                !fired);
    TEST_ASSERT_EQ("loaded planted value", v, 0x7070707070707070ULL);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-08: vsstatus.MXR=1 only affects VS-stage; G X-only -> fault.
 *   - VS RWX, G X-only U, sstatus.MXR=0, vsstatus.MXR=1
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_08_vsstatus_mxr_no_g);
bool test_ts_hlv_08_vsstatus_mxr_no_g(void) {
    TEST_BEGIN("TS-HLV-08: vsstatus.MXR does not override G-stage X-only");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;

    ts2_setup_with_dual_victim(&ctx, G13_VSMODE, G13_GMODE,
                               va, G13_VS_RWX, G13_G_XU);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_S);

    asm volatile ("csrc sstatus, %0" :: "r"((uintptr_t)SSTATUS_MXR));
    asm volatile ("csrs 0x200, %0" :: "r"((uintptr_t)SSTATUS_MXR));

    trap_expect_begin();
    (void)hlv_d(va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    asm volatile ("csrc 0x200, %0" :: "r"((uintptr_t)SSTATUS_MXR));
    ts2_finish(&ctx);

    TEST_ASSERT("HLV.D trapped on G X-only when only vsstatus.MXR=1",
                fired);
    TEST_ASSERT_EQ("cause = guest-page-fault (load) = 21",
                   cause, (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-09: HLVX.WU substitutes X for R; succeeds on X-only pages.
 *   - VS X-only U, G X-only U
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_09_hlvx_wu);
bool test_ts_hlv_09_hlvx_wu(void) {
    TEST_BEGIN("TS-HLV-09: HLVX.WU uses X permission instead of R");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    *(volatile uint32_t *)va = 0x90909090U;

    ts2_setup_with_dual_victim(&ctx, G13_VSMODE, G13_GMODE,
                               va, G13_VS_XU, G13_G_XU);
    two_stage_enable(&ctx, 0);
    hstatus_set_spvp(PRIV_U);   /* HLVX uses U/VU effective ok */

    trap_expect_begin();
    uint32_t v = hlvx_wu(va);
    bool fired = trap_was_triggered();
    trap_expect_end();

    hstatus_set_spvp(PRIV_S);
    ts2_finish(&ctx);

    TEST_ASSERT("no trap on HLVX.WU through X-only pages", !fired);
    TEST_ASSERT_EQ("HLVX.WU returned planted word",
                   (uintptr_t)v, (uintptr_t)0x90909090U);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-10: V=1 executing HLV -> virtual-instruction (cause=22).
 * =================================================================== */
static uintptr_t g13_vs_hlv_d(uintptr_t arg) {
    /* HLV.D rd,(rs1): funct7=0x36, rs2=x0, funct3=4, opcode=SYSTEM. */
    long result;
    asm volatile (
        ".insn r 0x73, 0x4, 0x36, %0, %1, x0"
        : "=r"(result) : "r"(arg) : "memory");
    return (uintptr_t)result;
}

TEST_REGISTER(test_ts_hlv_10_v1_virt_inst);
bool test_ts_hlv_10_v1_virt_inst(void) {
    TEST_BEGIN("TS-HLV-10: HLV.D in V=1 -> virt-inst (22)");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    ts2_setup_full(&ctx, G13_VSMODE, G13_GMODE);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, g13_vs_hlv_d, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("HLV.D in VS-mode trapped", fired);
    TEST_ASSERT_EQ("cause = virtual-instruction (22)",
                   cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-11: U-mode + hstatus.HU=0 -> illegal-instruction (cause=2).
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_11_u_hu0_illegal);
bool test_ts_hlv_11_u_hu0_illegal(void) {
    TEST_BEGIN("TS-HLV-11: U-mode + HU=0 -> illegal-inst (2)");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    ts2_setup_full_u(&ctx, G13_VSMODE, G13_GMODE);
    two_stage_enable(&ctx, 0);
    hstatus_set_hu(false);

    trap_expect_begin();
    (void)run_in_priv(PRIV_U, g13_u_hlv_d_for_hu, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    ts2_finish(&ctx);

    TEST_ASSERT("HLV.D in U-mode (HU=0) trapped", fired);
    TEST_ASSERT_EQ("cause = illegal-instruction (2)",
                   cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-HLV-12: U-mode + hstatus.HU=1 -> success.
 * =================================================================== */
TEST_REGISTER(test_ts_hlv_12_u_hu1_ok);
bool test_ts_hlv_12_u_hu1_ok(void) {
    TEST_BEGIN("TS-HLV-12: U-mode + HU=1 -> HLV.D succeeds");
    REQUIRE_VSATP_MODE(G13_VSMODE);
    REQUIRE_HGATP_MODE(G13_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    *(volatile uint64_t *)va = 0xC0C0C0C0C0C0C0C0ULL;

    ts2_setup_full_u(&ctx, G13_VSMODE, G13_GMODE);
    two_stage_enable(&ctx, 0);
    hstatus_set_hu(true);
    /* SPVP=0 -> VU effective. ts2_setup_full_u maps test region with
     * U=1 in VS-stage, so U-level access succeeds. */
    hstatus_set_spvp(PRIV_U);

    g13_target_va = va;
    g13_load_value = 0;

    trap_expect_begin();
    (void)run_in_priv(PRIV_U, g13_u_hlv_d_for_hu, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    hstatus_set_hu(false);
    hstatus_set_spvp(PRIV_S);
    ts2_finish(&ctx);

    if (fired) {
        printf("  TS-HLV-12 fired cause=0x%lx\n", (unsigned long)cause);
    }
    TEST_ASSERT("HLV.D in U-mode (HU=1) did not trap", !fired);
    HYP_TEST_END();
}
