/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4: Straddle / cross-page load (HTVAL-STR-*)
 *
 * Spec anchor:
 *   norm:H_htval — htval reports the GPA where the fault is taken.
 *
 * For an access that straddles two pages, the implementation may
 * deliver the misaligned-address exception OR may split the access
 * and only one half faults. The Shtvala normative requirement is
 * about htval contents, not about WHICH half faults; we just check
 * htval is consistent with the trapping address.
 *
 * Plan list:
 *   HTVAL-STR-01  load straddle with high half on V=0 page
 *   HTVAL-STR-02  store straddle with high half on W=0 page
 *   HTVAL-STR-03  load 8B unaligned, only 1B in V=0 page
 *
 * Strategy: place the access at GPA = page_n_end - 1, with the
 * fault page being the *next* page. We expect either:
 *   (a) cause=21 (load gpf) with htval ∈ {(fault_page)>>2, (op_addr)>>2}, or
 *   (b) cause=4  (load misaligned) — in which case Shtvala doesn't
 *       prescribe htval (no GPF). We accept either outcome but
 *       fail if cause is GPF and htval is unrelated.
 * =================================================================== */

/* VS-mode helpers performing the straddle access. Each takes the
 * starting address of the access and triggers a multi-byte load/
 * store across the page boundary. */
static uintptr_t vs_straddle_load(uintptr_t arg) {
    volatile uint64_t *p = (volatile uint64_t *)arg;
    (void)*p;
    return trap_get_cause();
}
static uintptr_t vs_straddle_store(uintptr_t arg) {
    volatile uint64_t *p = (volatile uint64_t *)arg;
    *p = HYP_TEST_MAGIC;
    return trap_get_cause();
}

/* Helper: fire a straddle load whose first byte sits on a normal
 * page and whose remaining 7 bytes spill into `victim_page`
 * configured with `victim_flags`. Returns true iff a trap fired. */
static bool _fire_straddle(uintptr_t (*helper)(uintptr_t),
                           uintptr_t straddle_addr,
                           uintptr_t victim_page,
                           uintptr_t victim_flags)
{
    two_stage_ctx_t ctx;
    _setup_with_victim(&ctx, victim_page, victim_flags);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, helper, straddle_addr);
    bool fired = trap_was_triggered();
    trap_expect_end();

    two_stage_cleanup(&ctx);
    return fired;
}

static bool _check_htval_for_straddle(uintptr_t straddle_addr,
                                      uintptr_t victim_page)
{
    uintptr_t cause = trap_get_cause();
    if (cause == CAUSE_LOAD_GUEST_PAGE_FAULT ||
        cause == CAUSE_STORE_GUEST_PAGE_FAULT ||
        cause == CAUSE_INST_GUEST_PAGE_FAULT) {
        uintptr_t htval = trap_get_htval();
        uintptr_t op_lo = straddle_addr >> 2;
        uintptr_t vp_lo = victim_page  >> 2;
        if (htval != op_lo && htval != vp_lo) {
            printf("  htval 0x%lx is neither op>>2=0x%lx nor victim>>2=0x%lx\n",
                   (unsigned long)htval,
                   (unsigned long)op_lo,
                   (unsigned long)vp_lo);
            return false;
        }
        return true;
    }
    /* Misalign or other: Shtvala doesn't apply, accept. */
    return true;
}

TEST_REGISTER(test_htval_str_01_load_straddle);
bool test_htval_str_01_load_straddle(void) {
    TEST_BEGIN("HTVAL-STR-01: load straddle into V=0 page reports htval = GPA>>2");
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Place the 8-byte access starting 1 byte before the fault page.
     * test_data_area is the page right before test_fault_page. */
    uintptr_t straddle = (uintptr_t)test_data_area + PAGE_SIZE_4K - 1;
    uintptr_t victim   = (uintptr_t)test_fault_page;

    bool fired = _fire_straddle(vs_straddle_load, straddle, victim, /*flags=*/0);
    TEST_ASSERT("trap fired", fired);
    if (fired) {
        bool ok = _check_htval_for_straddle(straddle, victim);
        TEST_ASSERT("htval consistent with trap", ok);
    }
    hyp_reset_state();
    HYP_TEST_END();
}

TEST_REGISTER(test_htval_str_02_store_straddle);
bool test_htval_str_02_store_straddle(void) {
    TEST_BEGIN("HTVAL-STR-02: store straddle into W=0 page reports htval = GPA>>2");
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    uintptr_t straddle = (uintptr_t)test_data_area + PAGE_SIZE_4K - 1;
    uintptr_t victim   = (uintptr_t)test_fault_page;
    uintptr_t flags    = (G_FLAGS_RWXU_AD & ~PTE_W);

    bool fired = _fire_straddle(vs_straddle_store, straddle, victim, flags);
    TEST_ASSERT("trap fired", fired);
    if (fired) {
        bool ok = _check_htval_for_straddle(straddle, victim);
        TEST_ASSERT("htval consistent with trap", ok);
    }
    hyp_reset_state();
    HYP_TEST_END();
}

TEST_REGISTER(test_htval_str_03_misalign_one_byte);
bool test_htval_str_03_misalign_one_byte(void) {
    TEST_BEGIN("HTVAL-STR-03: 8B load with only 1B in V=0 page reports htval");
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Same setup as STR-01 but explicitly accepts cause = misalign
     * (the spec leaves split-vs-misalign as impl-defined). */
    uintptr_t straddle = (uintptr_t)test_data_area + PAGE_SIZE_4K - 1;
    uintptr_t victim   = (uintptr_t)test_fault_page;

    bool fired = _fire_straddle(vs_straddle_load, straddle, victim, /*flags=*/0);
    TEST_ASSERT("trap fired", fired);
    if (fired) {
        uintptr_t cause = trap_get_cause();
        bool gpf = (cause == CAUSE_LOAD_GUEST_PAGE_FAULT);
        bool mis = (cause == CAUSE_LOAD_ADDR_MISALIGN);
        TEST_ASSERT("cause is GPF or misalign", gpf || mis);
        if (gpf) {
            bool ok = _check_htval_for_straddle(straddle, victim);
            TEST_ASSERT("htval consistent with GPF", ok);
        }
    }
    hyp_reset_state();
    HYP_TEST_END();
}
