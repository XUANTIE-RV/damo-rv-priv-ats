/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hypervisor x Zicboz: htinst / G-stage / stval tests (Groups 4, 5, 6)
 *
 * Tests cbo.zero htinst standard transformation, G-stage faults,
 * and virtual-instruction stval behavior.
 *
 * Reference: Hypervisor_CMO_test_plan.md Groups 4, 5, 6
 * Spec: norm:h_trans_cache, norm:cbz_unperm_fault, norm:fault_excep_csr
 */

/* ===================================================================
 * HCXINST-04: cbo.zero page-fault htinst standard transformation
 * =================================================================== */
TEST_REGISTER(test_hcxinst_04);
bool test_hcxinst_04(void)
{
    TEST_BEGIN("HCXINST-04: cbo.zero page-fault htinst");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim, 0 /* invalid */);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(1);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_zero, victim);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=23 (store guest-page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
        uintptr_t htinst = trap_get_htinst();
        bool ok = (htinst == HTINST_CBO_ZERO) || (htinst == 0);
        TEST_ASSERT("htinst=0x0040200F or 0", ok);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCXINST-09: cbo.zero virtual-instruction htinst
 * =================================================================== */
TEST_REGISTER(test_hcxinst_09);
bool test_hcxinst_09(void)
{
    TEST_BEGIN("HCXINST-09: cbo.zero virtual-inst htinst");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_zero, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        uintptr_t htinst = trap_get_htinst();
        bool ok = (htinst == HTINST_CBO_ZERO) || (htinst == 0);
        TEST_ASSERT("htinst=0x0040200F or 0", ok);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCXINST-11: VU-mode cbo.zero virtual-instruction htinst
 * =================================================================== */
TEST_REGISTER(test_hcxinst_11);
bool test_hcxinst_11(void)
{
    TEST_BEGIN("HCXINST-11: VU-mode cbo.zero virtual-inst htinst");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_zero, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        uintptr_t htinst = trap_get_htinst();
        bool ok = (htinst == HTINST_CBO_ZERO) || (htinst == 0);
        TEST_ASSERT("htinst=0x0040200F or 0", ok);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCGSTAGE-04: cbo.zero G-stage no write permission guest-page-fault
 * =================================================================== */
TEST_REGISTER(test_hcgstage_04);
bool test_hcgstage_04(void)
{
    TEST_BEGIN("HCGSTAGE-04: cbo.zero G-stage read-only GPF");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    /* G-stage: read-only (no write permission) */
    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim,
                            PTE_V | PTE_R | PTE_U | PTE_A | PTE_D);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(1);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_zero, victim);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=23 (store guest-page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCGSTAGE-06: cbo.zero G-stage GPF stval=rs1
 * =================================================================== */
TEST_REGISTER(test_hcgstage_06);
bool test_hcgstage_06(void)
{
    TEST_BEGIN("HCGSTAGE-06: cbo.zero G-stage GPF stval=rs1");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim, 0);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(1);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_zero, victim);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=23",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
        TEST_ASSERT_EQ("stval=rs1 (victim address)",
                       trap_get_tval(), victim);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCGSTAGE-07: cbo.zero G-stage GPF htval
 * =================================================================== */
TEST_REGISTER(test_hcgstage_07);
bool test_hcgstage_07(void)
{
    TEST_BEGIN("HCGSTAGE-07: cbo.zero G-stage GPF htval");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim, 0);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(1);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_zero, victim);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=23",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
        /* htval = faulting GPA >> 2 */
        uintptr_t expected_htval = victim >> 2;
        uintptr_t htval = trap_get_htval();
        bool ok = (htval == expected_htval) || (htval == 0);
        TEST_ASSERT("htval = GPA>>2 or 0", ok);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HCGSTAGE-10: cbo.zero read-only G-stage triggers GPF (needs write)
 * =================================================================== */
TEST_REGISTER(test_hcgstage_10);
bool test_hcgstage_10(void)
{
    TEST_BEGIN("HCGSTAGE-10: cbo.zero read-only G-stage GPF");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    uintptr_t victim = TEST_REGION_BASE;

    /* G-stage: read-only */
    ts2_setup_with_g_victim(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE,
                            victim,
                            PTE_V | PTE_R | PTE_U | PTE_A | PTE_D);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(1);

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_zero, victim);
    trap_expect_end();

    /* cbo.zero requires write permission */
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=23 (store guest-page-fault)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HVINST-04: cbo.zero virtual-instruction stval
 * =================================================================== */
TEST_REGISTER(test_hvinst_04);
bool test_hvinst_04(void)
{
    TEST_BEGIN("HVINST-04: cbo.zero virtual-inst stval");

    H_REQUIRED_OR_SKIP();
    ZICBOZ_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbze(1);
    henvcfg_set_cbze(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_zero, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        uintptr_t stval = trap_get_tval();
        TEST_ASSERT("stval=insn encoding or 0",
                    stval_is_cbo_insn(stval, CBO_OP_ZERO));
    }

    ts2_finish(&ctx);
    TEST_END();
}
