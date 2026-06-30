/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/tests_2stage/test_group01_vs_only.c
 *
 * Group 1: VS-stage only (hgatp = Bare)
 *
 * When hgatp.MODE == Bare, the second-stage translation is the
 * identity, so the only translation in effect is the VS-stage walked
 * by vsatp. VS-stage faults therefore raise a regular page-fault
 * (cause 12/13/15), NOT a guest-page-fault.
 *
 * Cases (test_plan: TS-VS-01..10):
 *   TS-VS-01  vsatp=Sv39, 4K identity, read+write succeeds
 *   TS-VS-02  vsatp=Sv39, 2MB superpage succeeds
 *   TS-VS-03  vsatp=Sv39, 1GB superpage succeeds
 *   TS-VS-04  vsatp=Sv48, 4K identity succeeds
 *   TS-VS-05  vsatp=Sv57, 4K identity succeeds
 *   TS-VS-06  VS-stage U=0 (S-level) read succeeds
 *   TS-VS-07  VS-stage U=1 (V=1 nominal-S) + SUM=0  -> load-page-fault
 *   TS-VS-08  VS-stage PTE V=0                       -> load-page-fault
 *   TS-VS-09  VS-stage PTE R=0,W=1 (reserved combo)  -> load-page-fault
 *   TS-VS-10  VS-stage PTE X=1,R=0,W=0 + load        -> load-page-fault
 *
 * This file is intended to be #include'd by a per-suite
 * tests/test_register.c that has previously defined SUITE_VSATP_MODE
 * if needed (default Sv39 from the framework header).
 *
 * The file does NOT use printf inside VS-mode; all UART output is
 * issued from M-mode after the inner trampoline returns.
 * =================================================================== */

#include "two_stage_helpers.h"

/* ----- helpers ----- */

/* All Group 1 cases run with G-stage disabled. */
#define G1_GMODE   HGATP_MODE_BARE

/* ===================================================================
 * TS-VS-01: vsatp=Sv39, 4K identity, simple R/W cycle
 * =================================================================== */
TEST_REGISTER(test_ts_vs_01_sv39_4k);
bool test_ts_vs_01_sv39_4k(void)
{
    TEST_BEGIN("TS-VS-01: vsatp=Sv39 4K identity, R/W succeeds");
    REQUIRE_VSATP_MODE(SATP_MODE_SV39);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV39, G1_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    uintptr_t r = ts2_run_check_no_fault(&ctx, test_vs_read_write, va);
    TEST_ASSERT_EQ("read-back equals magic", r, 0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VS-02: vsatp=Sv39, 2MB superpage covers test region
 * =================================================================== */
TEST_REGISTER(test_ts_vs_02_sv39_2m);
bool test_ts_vs_02_sv39_2m(void)
{
    TEST_BEGIN("TS-VS-02: vsatp=Sv39 2MB superpage");
    REQUIRE_VSATP_MODE(SATP_MODE_SV39);

    two_stage_ctx_t ctx;
    /* Manual setup: skip the framework's default 4K mapping and use
     * a single 2MB leaf for the test region. */
    gpt_pool_reset();
    pt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, G1_GMODE);

    uintptr_t base2m = TEST_REGION_BASE & ~(PAGE_SIZE_2M - 1);
    /* Identity-map low memory + the test 2MB superpage. */
    for (uintptr_t a = (PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1));
         a < base2m;
         a += PAGE_SIZE_2M) {
        (void)pt_map_page(&ctx.vs_ctx, a, a, VS_FLAGS_RWX_S_AD,
                          PT_LEVEL_2M);
    }
    (void)pt_map_page(&ctx.vs_ctx, base2m, base2m,
                      VS_FLAGS_RWX_S_AD, PT_LEVEL_2M);

    uintptr_t va = (uintptr_t)test_data_area;
    uintptr_t r = ts2_run_check_no_fault(&ctx, test_vs_read_write, va);
    TEST_ASSERT_EQ("read-back equals magic", r, 0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VS-03: vsatp=Sv39, 1GB superpage at low memory
 * =================================================================== */
TEST_REGISTER(test_ts_vs_03_sv39_1g);
bool test_ts_vs_03_sv39_1g(void)
{
    TEST_BEGIN("TS-VS-03: vsatp=Sv39 1GB superpage");
    REQUIRE_VSATP_MODE(SATP_MODE_SV39);

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    pt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, G1_GMODE);

    /* Identity-map a single 1GB region that covers code, UART, and
     * the test region. PLATFORM_MEM_BASE is 0x80000000 -> 1GB = [0x80000000, 0xC0000000). */
    uintptr_t base1g = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    (void)pt_map_page(&ctx.vs_ctx, base1g, base1g,
                      VS_FLAGS_RWX_S_AD, PT_LEVEL_1G);

    uintptr_t va = (uintptr_t)test_data_area;
    uintptr_t r = ts2_run_check_no_fault(&ctx, test_vs_read_write, va);
    TEST_ASSERT_EQ("read-back equals magic", r, 0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VS-04: vsatp=Sv48, 4K identity
 * =================================================================== */
TEST_REGISTER(test_ts_vs_04_sv48_4k);
bool test_ts_vs_04_sv48_4k(void)
{
    TEST_BEGIN("TS-VS-04: vsatp=Sv48 4K identity");
    REQUIRE_VSATP_MODE(SATP_MODE_SV48);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV48, G1_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    uintptr_t r = ts2_run_check_no_fault(&ctx, test_vs_read_write, va);
    TEST_ASSERT_EQ("read-back equals magic", r, 0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VS-05: vsatp=Sv57, 4K identity
 * =================================================================== */
TEST_REGISTER(test_ts_vs_05_sv57_4k);
bool test_ts_vs_05_sv57_4k(void)
{
    TEST_BEGIN("TS-VS-05: vsatp=Sv57 4K identity");
    REQUIRE_VSATP_MODE(SATP_MODE_SV57);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_SV57, G1_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    uintptr_t r = ts2_run_check_no_fault(&ctx, test_vs_read_write, va);
    TEST_ASSERT_EQ("read-back equals magic", r, 0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VS-06: VS-stage U=0 + S-level (VS-mode) access succeeds
 * =================================================================== */
TEST_REGISTER(test_ts_vs_06_ubit_s_level_ok);
bool test_ts_vs_06_ubit_s_level_ok(void)
{
    TEST_BEGIN("TS-VS-06: VS-stage U=0 + S-level access OK");
    REQUIRE_VSATP_MODE(SUITE_VSATP_MODE);

    two_stage_ctx_t ctx;
    /* Default VS_FLAGS_RWX_S_AD has U=0; ts2_setup_full installs it. */
    ts2_setup_full(&ctx, SUITE_VSATP_MODE, G1_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    uintptr_t r = ts2_run_check_no_fault(&ctx, test_vs_read_write, va);
    TEST_ASSERT_EQ("S-level access succeeds", r, 0);

    ts2_finish(&ctx);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VS-07: VS-stage PTE V=0 -> load page-fault (cause 13)
 * =================================================================== */
TEST_REGISTER(test_ts_vs_07_pte_invalid);
bool test_ts_vs_07_pte_invalid(void)
{
    TEST_BEGIN("TS-VS-07: VS-stage PTE V=0 -> load-page-fault");
    REQUIRE_VSATP_MODE(SUITE_VSATP_MODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    /* Override the VS-stage leaf for the fault page with V=0. */
    ts2_setup_with_vs_victim(&ctx, SUITE_VSATP_MODE, G1_GMODE,
                             va, /*flags=*/0);

    bool ok = ts2_run_check_fault(&ctx, test_vs_load_expect_fault, va,
                                  CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT("load-page-fault on V=0", ok);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VS-08: VS-stage PTE R=0,W=1 (reserved) -> page-fault
 * =================================================================== */
TEST_REGISTER(test_ts_vs_08_pte_reserved_rw);
bool test_ts_vs_08_pte_reserved_rw(void)
{
    TEST_BEGIN("TS-VS-08: VS-stage PTE R=0 W=1 reserved -> fault");
    REQUIRE_VSATP_MODE(SUITE_VSATP_MODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    uintptr_t bad_flags = PTE_V | PTE_W | PTE_A | PTE_D;  /* R=0, W=1 */
    ts2_setup_with_vs_victim(&ctx, SUITE_VSATP_MODE, G1_GMODE,
                             va, bad_flags);

    bool ok = ts2_run_check_fault(&ctx, test_vs_load_expect_fault, va,
                                  CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT("load-page-fault on R=0,W=1", ok);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VS-09: VS-stage PTE X-only + load -> load page-fault
 * =================================================================== */
TEST_REGISTER(test_ts_vs_09_pte_xonly_load);
bool test_ts_vs_09_pte_xonly_load(void)
{
    TEST_BEGIN("TS-VS-09: VS-stage X-only + load -> page-fault");
    REQUIRE_VSATP_MODE(SUITE_VSATP_MODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    /* X-only (no R,W). U=0 so VS-mode S-level is the access mode. */
    uintptr_t bad_flags = PTE_V | PTE_X | PTE_A | PTE_D;
    ts2_setup_with_vs_victim(&ctx, SUITE_VSATP_MODE, G1_GMODE,
                             va, bad_flags);

    bool ok = ts2_run_check_fault(&ctx, test_vs_load_expect_fault, va,
                                  CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT("load-page-fault on X-only", ok);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VS-10: VS-stage R-only + store -> store page-fault
 * =================================================================== */
TEST_REGISTER(test_ts_vs_10_pte_ronly_store);
bool test_ts_vs_10_pte_ronly_store(void)
{
    TEST_BEGIN("TS-VS-10: VS-stage R-only + store -> page-fault");
    REQUIRE_VSATP_MODE(SUITE_VSATP_MODE);

    two_stage_ctx_t ctx;
    uintptr_t va = (uintptr_t)test_fault_page;
    uintptr_t bad_flags = PTE_V | PTE_R | PTE_A | PTE_D;  /* no W */
    ts2_setup_with_vs_victim(&ctx, SUITE_VSATP_MODE, G1_GMODE,
                             va, bad_flags);

    bool ok = ts2_run_check_fault(&ctx, test_vs_store_expect_fault, va,
                                  CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT("store-page-fault on R-only", ok);
    HYP_TEST_END();
}
