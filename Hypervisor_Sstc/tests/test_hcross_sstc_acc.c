/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_sstc_acc.c - Group 9.2: VS-mode Access Control
 *
 * HCROSS-SSTC-03: henvcfg.STCE=0, VS-mode access -> virtual-inst
 * HCROSS-SSTC-04: henvcfg.STCE=1, VS-mode access -> no exception
 * HCROSS-SSTC-05: hcounteren.TM=0, VS-mode access -> virtual-inst
 *
 * Migrated from sstc_test_plan.md Group 3 (SSTC-ACC-08/09/10).
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-03: henvcfg.STCE=0, VS-mode -> virtual-inst            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_03);
bool test_hcross_sstc_03(void)
{
    TEST_BEGIN("HCROSS-SSTC-03: henvcfg.STCE=0, VS-mode -> virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);
    henvcfg_clear(HENVCFG_STCE);
    hcounteren_set(HCOUNTEREN_TM);
    stimecmp_write((uintptr_t)-1);

    trap_expect_begin();
    uintptr_t cause = run_in_vs_mode(_vs_stimecmp_read, 0);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("virtual-instruction on VS stimecmp read",
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();
    (void)cause;

    stimecmp_write((uintptr_t)-1);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-04: henvcfg.STCE=1, VS-mode -> no exception            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_04);
bool test_hcross_sstc_04(void)
{
    TEST_BEGIN("HCROSS-SSTC-04: henvcfg.STCE=1, VS-mode -> no exception");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);
    henvcfg_set(HENVCFG_STCE);
    hcounteren_set(HCOUNTEREN_TM);
    stimecmp_write((uintptr_t)-1);

    trap_expect_begin();
    run_in_vs_mode(_vs_stimecmp_read, 0);
    TEST_ASSERT("no trap on VS stimecmp read", !trap_was_triggered());
    trap_expect_end();

    stimecmp_write((uintptr_t)-1);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-05: hcounteren.TM=0, VS-mode -> virtual-inst           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_05);
bool test_hcross_sstc_05(void)
{
    TEST_BEGIN("HCROSS-SSTC-05: hcounteren.TM=0, VS-mode -> virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);
    henvcfg_set(HENVCFG_STCE);
    hcounteren_clear(HCOUNTEREN_TM);
    stimecmp_write((uintptr_t)-1);

    trap_expect_begin();
    uintptr_t cause = run_in_vs_mode(_vs_stimecmp_read, 0);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("virtual-instruction on VS stimecmp read (TM=0)",
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();
    (void)cause;

    stimecmp_write((uintptr_t)-1);
    TEST_END();
}
