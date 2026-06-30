/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 6: Exception priority verification
 *
 * Spec anchors:
 *   norm:HSyncExcPrio — G-stage fault > VS-stage fault
 *   hypervisor.adoc:2043-2051 — G-stage check before VS-stage
 *
 * 2 tests: SHA-PRIO-01 ~ SHA-PRIO-02
 * =================================================================== */

/* VS-mode helper: perform a load from an address */
static uintptr_t _prio_vs_do_load(uintptr_t addr) {
    uintptr_t val;
    asm volatile ("ld %0, 0(%1)" : "=r"(val) : "r"(addr));
    return val;
}

/* VS-mode helper: perform a store to an address */
static uintptr_t _prio_vs_do_store(uintptr_t addr) {
    uintptr_t val = 0x42;
    asm volatile ("sd %0, 0(%1)" :: "r"(val), "r"(addr));
    return 0;
}

/* ---- SHA-PRIO-01: G-stage fault > VS-stage fault (load) ---- */

TEST_REGISTER(test_sha_prio_gstage_over_vsstage);
bool test_sha_prio_gstage_over_vsstage(void) {
    TEST_BEGIN("SHA-PRIO-01: G-stage fault takes priority over VS-stage fault");
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Set up G-stage identity mapping for kernel, but leave a target
     * GPA unmapped. VS-mode (vsatp=Bare) accesses the unmapped GPA.
     * Expected: G-stage load GPF (cause=21), htval = GPA >> 2. */

    uintptr_t target_gpa = 0x50000000UL;

    gpt_pool_reset();
    two_stage_ctx_t ctx;
    two_stage_init(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Identity-map kernel region */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    two_stage_setup_identity(&ctx, base, PLATFORM_MEM_SIZE,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);
    /* target_gpa is NOT mapped in G-stage */

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, _prio_vs_do_load, target_gpa);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = fired ? trap_get_htval() : 0;
    trap_expect_end();

    two_stage_cleanup(&ctx);

    TEST_ASSERT("G-stage fault triggered", fired);
    if (fired) {
        /* G-stage load page fault, not VS-stage (cause=13) */
        TEST_ASSERT_EQ("cause == load guest-page-fault (21)",
                       cause, (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
        TEST_ASSERT_EQ("htval == target GPA >> 2",
                       htval, target_gpa >> 2);
        TEST_ASSERT("htval != 0 (Shtvala guarantee)", htval != 0);
    }

    HYP_TEST_END();
}

/* ---- SHA-PRIO-02: G-stage store fault (W=0) fires correctly ---- */

TEST_REGISTER(test_sha_prio_store_gpf_with_no_write);
bool test_sha_prio_store_gpf_with_no_write(void) {
    TEST_BEGIN("SHA-PRIO-02: G-stage store GPF (W=0) fires with correct htval");
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Map a target GPA outside the kernel 2MB region at 4KB granularity
     * with R+X but NO W permission. VS-mode (vsatp=Bare) attempts a
     * store to this GPA.
     * Expected: G-stage store GPF (cause=23), htval = GPA >> 2.
     *
     * We use GPA 0x50000000 which is outside the kernel memory region,
     * so there's no conflict with the 2MB identity mapping. We map it
     * separately at 4KB granularity with restricted permissions. */

    uintptr_t target_gpa = 0x50000000UL;

    gpt_pool_reset();
    two_stage_ctx_t ctx;
    two_stage_init(&ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Identity-map kernel region with full permissions (2MB pages) */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    two_stage_setup_identity(&ctx, base, PLATFORM_MEM_SIZE,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    /* Map target page at 4KB with R+X but NO W.
     * This GPA (0x50000000) is outside the kernel 2MB region,
     * so no conflict with existing leaf PTEs. The backing SPA
     * doesn't need to be real memory — the G-stage permission
     * check (W=0 -> store GPF) fires before any actual access. */
    uintptr_t no_w_flags = PTE_V | PTE_R | PTE_X | PTE_U | PTE_A | PTE_D;
    two_stage_g_map_page(&ctx, target_gpa, target_gpa,
                         no_w_flags, PT_LEVEL_4K);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, _prio_vs_do_store, target_gpa);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    uintptr_t htval = fired ? trap_get_htval() : 0;
    trap_expect_end();

    two_stage_cleanup(&ctx);

    TEST_ASSERT("G-stage store fault triggered", fired);
    if (fired) {
        TEST_ASSERT_EQ("cause == store guest-page-fault (23)",
                       cause, (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
        TEST_ASSERT_EQ("htval == target GPA >> 2",
                       htval, target_gpa >> 2);
        TEST_ASSERT("htval != 0 (Shtvala guarantee)", htval != 0);
    }

    HYP_TEST_END();
}
