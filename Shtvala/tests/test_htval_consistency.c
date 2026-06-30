/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 8: htval / stval consistency (HTVAL-CON-*)
 *
 * Spec anchor:
 *   norm:H_htval — on guest-page-fault, htval = GPA >> 2.
 *   norm:H_stval (Sstvala in HS context) — stval = faulting GVA.
 *
 * When VS-stage is Bare, GVA == GPA. We can therefore reconstruct
 * the faulting low 12 bits from stval and concatenate with htval<<2
 * to obtain the same address.
 *
 * Plan list:
 *   HTVAL-CON-01  reconstruct_address(htval,stval) == GPA  (load gpf)
 *   HTVAL-CON-02  reconstruct_address(htval,stval) == GPA  (store gpf)
 *   HTVAL-CON-03  reconstruct_address(htval,stval) == GPA  (fetch gpf)
 *
 * Note: with hedeleg=0 the trap is taken in M-mode, so the framework
 * exposes mtval as trap_get_tval() and mtval2 as trap_get_htval().
 * mtval2 is "htval" in the M-mode capture.
 * =================================================================== */

static bool _check_reconstruction(uintptr_t target) {
    if (!trap_was_triggered()) {
        printf("  no trap fired\n");
        return false;
    }
    uintptr_t htval = trap_get_htval();
    uintptr_t tval  = trap_get_tval();
    uintptr_t reconstructed = (htval << 2) | (tval & 0x3UL);
    /* htval drops the low 2 bits of GPA; the spec lets the low 2 bits
     * be either 0 or come from stval/mtval. We rebuild the page-
     * granular GPA and compare to target & ~3. */
    uintptr_t expect_pg = target & ~0x3UL;
    if ((reconstructed & ~0x3UL) != expect_pg) {
        printf("  reconstruct mismatch: htval=0x%lx tval=0x%lx -> 0x%lx, "
               "expected page 0x%lx\n",
               (unsigned long)htval, (unsigned long)tval,
               (unsigned long)reconstructed, (unsigned long)expect_pg);
        return false;
    }
    return true;
}

TEST_REGISTER(test_htval_con_01_load);
bool test_htval_con_01_load(void) {
    TEST_BEGIN("HTVAL-CON-01: htval||stval reconstructs faulting GPA (load)");
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    uintptr_t target = (uintptr_t)test_fault_page + 0x100;
    uintptr_t flags  = (G_FLAGS_RWXU_AD & ~PTE_R);

    bool fired = _fire_load_fault(target, flags);
    TEST_ASSERT("load gpf fired", fired);
    if (fired) TEST_ASSERT("reconstruction matches", _check_reconstruction(target));
    hyp_reset_state();
    HYP_TEST_END();
}

TEST_REGISTER(test_htval_con_02_store);
bool test_htval_con_02_store(void) {
    TEST_BEGIN("HTVAL-CON-02: htval||stval reconstructs faulting GPA (store)");
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    uintptr_t target = (uintptr_t)test_fault_page + 0x080;
    uintptr_t flags  = (G_FLAGS_RWXU_AD & ~PTE_W);

    bool fired = _fire_store_fault(target, flags);
    TEST_ASSERT("store gpf fired", fired);
    if (fired) TEST_ASSERT("reconstruction matches", _check_reconstruction(target));
    hyp_reset_state();
    HYP_TEST_END();
}

TEST_REGISTER(test_htval_con_03_fetch);
bool test_htval_con_03_fetch(void) {
    TEST_BEGIN("HTVAL-CON-03: htval||stval reconstructs faulting GPA (fetch)");
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    /* Fetch GVA must be 4B-aligned in RVI (we don't enable C here),
     * so the low 2 bits are always 0 — reconstruction is simply
     * (htval << 2). */
    uintptr_t target = (uintptr_t)test_fault_page;
    uintptr_t flags  = (G_FLAGS_RWXU_AD & ~PTE_X);

    bool fired = _fire_fetch_fault(target, flags);
    TEST_ASSERT("fetch gpf fired", fired);
    if (fired) TEST_ASSERT("reconstruction matches", _check_reconstruction(target));
    hyp_reset_state();
    HYP_TEST_END();
}
