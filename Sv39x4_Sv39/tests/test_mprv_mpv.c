/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/tests_2stage/test_group14_mprv_mpv.c
 *
 * Group 14 - mstatus.MPRV+MPV triggered two-stage access (TS-MPRV-01..05)
 *
 * Spec basis (priv-1.12):
 *   - When the hart is in M-mode and mstatus.MPRV=1, explicit memory
 *     accesses are translated as if the hart were running in the
 *     privilege mode encoded in mstatus.MPP, with virtualization mode
 *     mstatus.MPV.
 *   - MPP=M makes MPRV a no-op (no translation regardless of MPV).
 *   - MPRV/MPV/MPP do NOT affect HLV/HLVX/HSV instructions; those
 *     always perform two-stage with effective privilege from
 *     hstatus.SPVP.
 *
 * IMPORTANT (stack safety, see test plan note for Group 14):
 *   The MPRV=1 window must contain only the single targeted ld, with
 *   no stack access. We therefore:
 *     - pre-program MPV / MPP outside the MPRV window
 *     - enter the window with `csrs mstatus, MPRV`, do exactly one
 *       inline-asm ld, then `csrc mstatus, MPRV`.
 *   Tests that intentionally fault inside MPRV are NOT included here
 *   (they would risk the M-mode trap handler running with MPRV=1).
 * =================================================================== */

#ifndef SUITE_HGATP_MODE
#error "SUITE_HGATP_MODE must be defined before including this file"
#endif
#ifndef SUITE_VSATP_MODE
#error "SUITE_VSATP_MODE must be defined before including this file"
#endif

#include "hyp_ldst.h"

#define G14_GMODE   SUITE_HGATP_MODE
#define G14_VSMODE  SUITE_VSATP_MODE

#define G14_VS_RWX  (PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D)
#define G14_VS_RWXU (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)
#define G14_G_RWXU  (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)

/* Pre-program mstatus.MPV / MPP to (mpv, mpp). Caller is responsible
 * for clearing them after the test. */
static inline void g14_set_mpv_mpp(int mpv, unsigned mpp)
{
    /* clear MPP first */
    asm volatile ("csrc mstatus, %0" :: "r"((uintptr_t)MSTATUS_MPP_MASK));
    if (mpv) {
        asm volatile ("csrs mstatus, %0" :: "r"((uintptr_t)MSTATUS_MPV));
    } else {
        asm volatile ("csrc mstatus, %0" :: "r"((uintptr_t)MSTATUS_MPV));
    }
    asm volatile ("csrs mstatus, %0"
                  :: "r"(((uintptr_t)mpp) << MSTATUS_MPP_OFF));
}

/* Restore MPV=0, MPP=0. */
static inline void g14_clear_mpv_mpp(void)
{
    asm volatile ("csrc mstatus, %0"
                  :: "r"((uintptr_t)(MSTATUS_MPV | MSTATUS_MPP_MASK)));
}

/* Single-instruction ld inside an MPRV=1 window. */
static inline uint64_t g14_mprv_ld_d(uintptr_t addr)
{
    uint64_t v;
    asm volatile (
        "csrs mstatus, %1\n\t"
        "ld   %0, 0(%2)\n\t"
        "csrc mstatus, %1\n\t"
        : "=&r"(v)
        : "r"((uintptr_t)MSTATUS_MPRV_BIT), "r"(addr)
        : "memory");
    return v;
}

/* ===================================================================
 * Common non-identity setup helper. Returns:
 *   *out_va  - VA the test will load
 *   *out_spa - SPA where the magic value is planted
 * Layout: VA != GPA != SPA, all inside the test region.
 * =================================================================== */
static void g14_setup(two_stage_ctx_t *ctx, uintptr_t *out_va,
                      uintptr_t *out_spa, uintptr_t vs_flags)
{
    uintptr_t va  = (uintptr_t)test_fault_page;
    uintptr_t gpa = va + PAGE_SIZE_4K;
    uintptr_t spa = va + 2 * PAGE_SIZE_4K;

    /* Plant SPA = magic, VA = different magic (visible if no xlate). */
    *(volatile uint64_t *)spa = 0xCAFEBABE12345678ULL;
    *(volatile uint64_t *)va  = 0xAAAA0000AAAA0000ULL;

    ts2_setup_non_identity(ctx, G14_VSMODE, G14_GMODE,
                           va, gpa, spa, vs_flags, G14_G_RWXU);
    /* Activate vsatp/hgatp without entering V=1. */
    two_stage_enable(ctx, /*vmid*/0);

    *out_va  = va;
    *out_spa = spa;
}

/* ===================================================================
 * TS-MPRV-01: MPRV=0 -> no translation; ld va reads physical.
 * =================================================================== */
TEST_REGISTER(test_ts_mprv_01_no_xlate);
bool test_ts_mprv_01_no_xlate(void) {
    TEST_BEGIN("TS-MPRV-01: MPRV=0 -> direct physical access");
    REQUIRE_VSATP_MODE(G14_VSMODE);
    REQUIRE_HGATP_MODE(G14_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va, spa;
    g14_setup(&ctx, &va, &spa, G14_VS_RWX);

    /* MPV=1, MPP=S, but MPRV=0 -> no translation. */
    g14_set_mpv_mpp(/*mpv*/1, /*mpp*/PRIV_S);
    uint64_t v = *(volatile uint64_t *)va;  /* MPRV=0 path */
    g14_clear_mpv_mpp();

    ts2_finish(&ctx);
    TEST_ASSERT_EQ("direct physical (VA-physical) value",
                   v, 0xAAAA0000AAAA0000ULL);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-MPRV-02: MPRV=1 + MPV=1 + MPP=S -> two-stage translation,
 *             ld returns SPA value via VS+G stages.
 * =================================================================== */
TEST_REGISTER(test_ts_mprv_02_vs_two_stage);
bool test_ts_mprv_02_vs_two_stage(void) {
    TEST_BEGIN("TS-MPRV-02: MPRV+MPV=1+MPP=S -> VS-level two-stage");
    REQUIRE_VSATP_MODE(G14_VSMODE);
    REQUIRE_HGATP_MODE(G14_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va, spa;
    g14_setup(&ctx, &va, &spa, G14_VS_RWX);  /* VS U=0, S-level access */

    g14_set_mpv_mpp(/*mpv*/1, /*mpp*/PRIV_S);
    uint64_t v = g14_mprv_ld_d(va);
    g14_clear_mpv_mpp();

    ts2_finish(&ctx);
    TEST_ASSERT_EQ("two-stage translated to SPA value",
                   v, 0xCAFEBABE12345678ULL);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-MPRV-03: MPRV=1 + MPV=1 + MPP=U -> VU-level two-stage,
 *             U=1 PTE allows access; loads SPA value.
 * =================================================================== */
TEST_REGISTER(test_ts_mprv_03_vu_two_stage);
bool test_ts_mprv_03_vu_two_stage(void) {
    TEST_BEGIN("TS-MPRV-03: MPRV+MPV=1+MPP=U + VS U=1 -> VU two-stage");
    REQUIRE_VSATP_MODE(G14_VSMODE);
    REQUIRE_HGATP_MODE(G14_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va, spa;
    g14_setup(&ctx, &va, &spa, G14_VS_RWXU);  /* VS U=1 -> VU OK */

    g14_set_mpv_mpp(/*mpv*/1, /*mpp*/PRIV_U);
    uint64_t v = g14_mprv_ld_d(va);
    g14_clear_mpv_mpp();

    ts2_finish(&ctx);
    TEST_ASSERT_EQ("VU two-stage -> SPA value",
                   v, 0xCAFEBABE12345678ULL);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-MPRV-04: MPRV=1 + MPP=M -> MPRV is a no-op; direct physical.
 * =================================================================== */
TEST_REGISTER(test_ts_mprv_04_mpp_m);
bool test_ts_mprv_04_mpp_m(void) {
    TEST_BEGIN("TS-MPRV-04: MPRV=1 + MPP=M -> direct physical");
    REQUIRE_VSATP_MODE(G14_VSMODE);
    REQUIRE_HGATP_MODE(G14_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va, spa;
    g14_setup(&ctx, &va, &spa, G14_VS_RWX);

    g14_set_mpv_mpp(/*mpv*/1, /*mpp*/PRIV_M);
    uint64_t v = g14_mprv_ld_d(va);  /* MPRV=1 but MPP=M -> no xlate */
    g14_clear_mpv_mpp();

    ts2_finish(&ctx);
    TEST_ASSERT_EQ("MPP=M -> physical (VA-physical) value",
                   v, 0xAAAA0000AAAA0000ULL);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-MPRV-05: MPRV/MPV/MPP do NOT change HLV behaviour. HLV.D still
 * uses two-stage with hstatus.SPVP, regardless of MPRV settings.
 * =================================================================== */
TEST_REGISTER(test_ts_mprv_05_hlv_unaffected);
bool test_ts_mprv_05_hlv_unaffected(void) {
    TEST_BEGIN("TS-MPRV-05: HLV.D unaffected by MPRV/MPV/MPP");
    REQUIRE_VSATP_MODE(G14_VSMODE);
    REQUIRE_HGATP_MODE(G14_GMODE);

    two_stage_ctx_t ctx;
    uintptr_t va, spa;
    g14_setup(&ctx, &va, &spa, G14_VS_RWX);

    /* Set MPRV=1 + MPV=0 + MPP=M (so MPRV is harmless), then HLV.D. */
    g14_set_mpv_mpp(/*mpv*/0, /*mpp*/PRIV_M);
    asm volatile ("csrs mstatus, %0" :: "r"((uintptr_t)MSTATUS_MPRV_BIT));

    /* HLV needs SPVP=S (VS-level) and a regular S-level VS PTE. */
    hstatus_set_spvp(PRIV_S);
    uint64_t v = hlv_d(va);

    asm volatile ("csrc mstatus, %0" :: "r"((uintptr_t)MSTATUS_MPRV_BIT));
    g14_clear_mpv_mpp();
    ts2_finish(&ctx);

    TEST_ASSERT_EQ("HLV.D returns SPA value", v, 0xCAFEBABE12345678ULL);
    HYP_TEST_END();
}
