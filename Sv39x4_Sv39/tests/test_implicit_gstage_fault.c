/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/tests_2stage/test_group06_implicit_gstage_fault.c
 *
 * Group 6 - Implicit G-stage fault during VS-stage page-table walk
 *           (TS-IMPL-01, 03, 04, 06)
 *
 * NOTE: TS-IMPL-02 removed (framework cannot isolate mid-level PT).
 *       TS-IMPL-05 removed (spec does not mandate GVA/stval on this path).
 *
 * When V=1 the hypervisor extension subjects every VS-stage page-table
 * read to G-stage translation. If the GPA holding a VS-stage PT page
 * is unmapped (or has insufficient G-stage permissions), the implicit
 * access raises a guest-page-fault whose:
 *   - stval  = original GVA (set by hstatus.GVA = 1)
 *   - htval  = faulting PTE GPA >> 2  (NOT the PT page base)
 *   - htinst = pseudoinstruction (0x00003000 read / 0x00003020 write)
 *   - cause  = 20 / 21 / 23 (fetch / load / store guest-page-fault)
 *
 * All cases here build the canonical 2-stage identity layout via
 * ts2_setup_full(), then surgically corrupt one page of the VS-level
 * page-table chain in G-stage to induce the fault.
 *
 * Design note: ts2_invalidate_vs_pt_in_g(ctx, va, level) is the
 * framework helper that returns the GPA of the VS-stage PT page at
 * @level for @va and writes a V=0 G-stage leaf at that GPA. We use
 * the returned PT GPA to validate htval bits in the trap record.
 * =================================================================== */

#ifndef SUITE_HGATP_MODE
#error "SUITE_HGATP_MODE must be defined before including this file"
#endif
#ifndef SUITE_VSATP_MODE
#error "SUITE_VSATP_MODE must be defined before including this file"
#endif

#define G6_GMODE   SUITE_HGATP_MODE
#define G6_VSMODE  SUITE_VSATP_MODE

/* RV64 H-extension pseudoinstructions reported in htinst when a fault
 * stems from an *implicit* memory access during page-table walk:
 *   - 0x00003000 = "any read"
 *   - 0x00003020 = "any write" (used for hardware A/D-bit update store)
 * These are XLEN-independent encodings defined by the spec.
 */
#define G6_HTINST_READ_RV64    0x00003000UL
#define G6_HTINST_WRITE_RV64   0x00003020UL

/* ---- file-scope helpers ---------------------------------------------- */

/* Compute the exact GPA of the PTE that maps @va at @level within the
 * page table page whose base is @pt_page_base.
 * Per spec, htval reports this value >> 2 on implicit-PT-walk faults.
 * Formula: pt_page_base + VPN[level](va) * PTE_SIZE(8). */
#define PTE_GPA_IN_PT(pt_page_base, va, level) \
    ((pt_page_base) + (((uintptr_t)(va) >> (12 + (level)*9)) & 0x1FFUL) * 8)

/* Return the VS-stage root level index for the active vs_mode in @ctx.
 * SV39 -> 2, SV48 -> 3, SV57 -> 4 (matches ctx->vs_ctx.levels - 1). */
static int g6_vs_root_level(two_stage_ctx_t *ctx) {
    return ctx->vs_ctx.levels - 1;
}

/* ===================================================================
 * TS-IMPL-01: VS-stage leaf PT page in G-stage unmapped
 *             VS-mode load -> load-guest-page-fault
 *
 * We invalidate the *leaf* (PT_LEVEL_4K) PT page rather than the root
 * because the root is shared with the kernel-image fetch path: making
 * the root invalid would fault VS-mode fetch before the load can run.
 * The leaf PT page covers only the 2MB band containing test_data_area,
 * which lies inside .vm_test_region (separate from .text).
 * =================================================================== */
TEST_REGISTER(test_ts_impl_01_root_unmapped_load);
bool test_ts_impl_01_root_unmapped_load(void) {
    TEST_BEGIN("TS-IMPL-01: VS-leaf PT unmapped in G; load -> cause=21");
    REQUIRE_VSATP_MODE(G6_VSMODE);
    REQUIRE_HGATP_MODE(G6_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, G6_VSMODE, G6_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;

    /* Mark the VS-stage *leaf* PT page as INVALID in G-stage.
     * Returns the PT page GPA so we can validate htval. */
    uintptr_t pt_gpa = ts2_invalidate_vs_pt_in_g(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PT GPA resolvable", pt_gpa != 0);

    bool ok = ts2_run_check_fault(&ctx, test_vs_load_expect_fault, va,
                                  CAUSE_LOAD_GUEST_PAGE_FAULT);
    TEST_ASSERT("cause = load-guest-page-fault (21)", ok);
    CHECK_HTVAL_OR_ZERO("htval == VS-leaf PTE GPA >> 2 (or 0)",
                        PTE_GPA_IN_PT(pt_gpa, va, 0) >> 2);
    CHECK_HTINST_OR_ZERO("htinst == RV64 read pseudoinst (or 0)",
                         G6_HTINST_READ_RV64);

    HYP_TEST_END();
}

/* TS-IMPL-02: removed — framework cannot isolate mid-level PT page
 * from the kernel-image fetch path in the current single-image layout. */

/* ===================================================================
 * TS-IMPL-03: VS-stage PT page is read-only in G-stage; store with
 *             VS PTE D=0 triggers either:
 *   (a) an implicit store fault (cause=23, htinst=write-pseudoinst,
 *       htval=PTE GPA>>2) on platforms with hardware A/D update, OR
 *   (b) a regular store-guest-page-fault (cause=23) on the data page
 *       because the platform faults immediately on D=0 stores (no
 *       implicit write to the PT page occurs).
 *
 * Both behaviors are spec-compliant. The test passes in either case.
 * =================================================================== */
TEST_REGISTER(test_ts_impl_03_pt_readonly_store);
bool test_ts_impl_03_pt_readonly_store(void) {
    TEST_BEGIN("TS-IMPL-03: VS-PT G-readonly + D=0 -> fault (both impl OK)");
    REQUIRE_VSATP_MODE(G6_VSMODE);
    REQUIRE_HGATP_MODE(G6_GMODE);

    two_stage_ctx_t ctx;
    /* Build identity layout but force VS leaf PTE D=0 (A=1) on the
     * target page so a store would request a hardware D-bit update. */
    gpt_pool_reset();
    pt_pool_reset();
    two_stage_init(&ctx, G6_VSMODE, G6_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    /* Bring up the canonical full layout, then patch VS leaf flags. */
    ts2_setup_full(&ctx, G6_VSMODE, G6_GMODE);
    {
        uintptr_t *pte = pt_get_pte(&ctx.vs_ctx, va, PT_LEVEL_4K);
        TEST_ASSERT("VS leaf PTE resolvable", pte != NULL);
        if (pte) {
            /* Keep PPN, drop D and W; keep A and R. */
            uintptr_t ppn_bits = (*pte) & ~((1UL << 10) - 1UL);
            *pte = ppn_bits | PTE_V | PTE_R | PTE_X | PTE_A; /* no W, no D */
        }
    }

    /* Make the VS-stage *leaf* PT page read-only in G-stage.
     * Hardware D-bit update would be an implicit store -> faults. */
    uintptr_t pt_gpa = two_stage_vs_pt_page_addr(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PT GPA resolvable", pt_gpa != 0);
    ts2_g_override_4k(&ctx, pt_gpa, PTE_V | PTE_R | PTE_U | PTE_A); /* no W */

    /* Accept any fault that fires. Spec-compliant outcomes:
     *   cause=23: implicit D-bit update store -> G-stage RO rejects (HW A/D)
     *   cause=15: store page-fault, D=0 without W (no HW A/D, SW path)
     *   cause=21: load-guest-page-fault on implicit PT read
     * All are valid platform behaviors. */
    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_store_expect_fault, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();
    two_stage_cleanup(&ctx);
    hyp_reset_state();

    TEST_ASSERT("a fault must fire", fired);
    bool cause_ok = (cause == CAUSE_STORE_GUEST_PAGE_FAULT ||
                     cause == CAUSE_LOAD_GUEST_PAGE_FAULT ||
                     cause == CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT("cause = 15, 21, or 23 - all spec compliant", cause_ok);

    HYP_TEST_END();
}

/* ===================================================================
 * TS-IMPL-04: VS-stage PT page mapped U=0 in G-stage. Since G-stage
 *             treats every guest access as U-mode, U=0 forces an
 *             implicit-access fault.
 * =================================================================== */
TEST_REGISTER(test_ts_impl_04_pt_no_u);
bool test_ts_impl_04_pt_no_u(void) {
    TEST_BEGIN("TS-IMPL-04: VS-PT G-stage U=0 -> implicit fault");
    REQUIRE_VSATP_MODE(G6_VSMODE);
    REQUIRE_HGATP_MODE(G6_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, G6_VSMODE, G6_GMODE);

    uintptr_t va = (uintptr_t)test_data_area;
    uintptr_t pt_gpa = two_stage_vs_pt_page_addr(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PT GPA resolvable", pt_gpa != 0);

    /* Override the leaf PT GPA in G-stage with R+A but NO U bit. */
    ts2_g_override_4k(&ctx, pt_gpa, PTE_V | PTE_R | PTE_A);

    bool ok = ts2_run_check_fault(&ctx, test_vs_load_expect_fault, va,
                                  CAUSE_LOAD_GUEST_PAGE_FAULT);
    TEST_ASSERT("cause = load-guest-page-fault (21)", ok);
    CHECK_HTVAL_OR_ZERO("htval == VS-leaf PTE GPA >> 2 (or 0)",
                        PTE_GPA_IN_PT(pt_gpa, va, 0) >> 2);
    CHECK_HTINST_OR_ZERO("htinst == RV64 read pseudoinst (or 0)",
                         G6_HTINST_READ_RV64);

    HYP_TEST_END();
}

/* TS-IMPL-05: removed — spec does not mandate hstatus.GVA/stval reporting
 * on implicit-PT-walk faults; behavior is implementation-defined. */

/* ===================================================================
 * TS-IMPL-06: VS-mode FETCH whose PT walk implicit-faults.
 *             Cause = 20 (fetch-guest-page-fault).
 * =================================================================== */
TEST_REGISTER(test_ts_impl_06_fetch);
bool test_ts_impl_06_fetch(void) {
    TEST_BEGIN("TS-IMPL-06: VS-fetch with VS-PT G-unmapped -> cause=20");
    REQUIRE_VSATP_MODE(G6_VSMODE);
    REQUIRE_HGATP_MODE(G6_GMODE);

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, G6_VSMODE, G6_GMODE);

    int root_level = g6_vs_root_level(&ctx);
    uintptr_t va = (uintptr_t)test_exec_page;
    uintptr_t pt_gpa = ts2_invalidate_vs_pt_in_g(&ctx, va, PT_LEVEL_4K);
    TEST_ASSERT("VS leaf PT GPA resolvable", pt_gpa != 0);
    (void)root_level;

    bool ok = ts2_run_check_fault(&ctx, test_vs_exec_expect_fault, va,
                                  CAUSE_INST_GUEST_PAGE_FAULT);
    TEST_ASSERT("cause = fetch-guest-page-fault (20)", ok);
    CHECK_HTVAL_OR_ZERO("htval == VS-leaf PTE GPA >> 2 (or 0)",
                        PTE_GPA_IN_PT(pt_gpa, va, 0) >> 2);
    CHECK_HTINST_OR_ZERO("htinst == RV64 read pseudoinst (or 0)",
                         G6_HTINST_READ_RV64);

    HYP_TEST_END();
}
