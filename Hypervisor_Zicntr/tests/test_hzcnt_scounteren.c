/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 6.2: scounteren continued control of VU-mode when V=1 (cycle)
 *
 * HZCNT-07 ~ HZCNT-09
 *
 * Spec anchors:
 *   H_scsrs_nomatch_vu_counter (norm:H_scsrs_nomatch specialization)
 *     — scounteren has no matching VS CSR; when V=1 it keeps its
 *       usual function with VS/VU substituting for HS/U, i.e. it
 *       still gates VU-mode counter visibility.
 *   hcounteren_gate_v1_counter (norm:hcounteren_op specialization)
 *     — hcounteren.CY=0 with mcounteren.CY=1 blocks V=1 cycle reads
 *       with virtual-instruction (cause=22), taking precedence over
 *       the scounteren layer for VU-mode.
 *     — scounteren does NOT gate VS-mode counter access.
 *
 * Hypervisor_CSR_test_plan.md VCSR-17 covers only the hcounteren=1
 * branch; this group adds the hcounteren=0 block branch (HZCNT-08)
 * and the VS-mode negative check (HZCNT-09).
 * =================================================================== */

/* ---- HZCNT-07: scounteren controls VU cycle (hcounteren.CY=1) ---- */

TEST_REGISTER(test_hzcnt_07);
bool test_hzcnt_07(void)
{
    TEST_BEGIN("HZCNT-07: scounteren gates VU cycle when hcen.CY=1");
    REQUIRE_H_EXT();
    REQUIRE_ZICNTR();

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();
    uintptr_t saved_scen = scounteren_read();

    if (!mcounteren_bit_set_sticky(CNT_BIT_CY)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren.CY is read-only zero");
    }
    if (!hcounteren_bit_set_sticky(CNT_BIT_CY)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("hcounteren.CY is read-only zero");
    }

    /* Phase 1: scounteren.CY=1 -> VU read succeeds. */
    if (!scounteren_bit_set_sticky(CNT_BIT_CY)) {
        scounteren_write(saved_scen);
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("scounteren.CY is read-only zero");
    }
    trap_expect_begin();
    (void)run_in_vu_mode(_vs_read_cycle, 0);
    TEST_ASSERT("VU cycle read no trap (scounteren.CY=1)",
                !trap_was_triggered());
    trap_expect_end();

    /* Phase 2: scounteren.CY=0 -> VU read traps. */
    if (!scounteren_bit_clear_sticky(CNT_BIT_CY)) {
        scounteren_write(saved_scen);
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("scounteren.CY is not clearable");
    }
    trap_expect_begin();
    (void)run_in_vu_mode(_vs_read_cycle, 0);
    TEST_ASSERT("VU cycle read trapped (scounteren.CY=0)",
                trap_was_triggered());
    if (trap_was_triggered()) {
        /* VU-mode with scounteren bit clear (mcounteren=1) reports
         * virtual-instruction per the hypervisor counteren rules. */
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();

    scounteren_write(saved_scen);
    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}

/* ---- HZCNT-08: hcounteren.CY=0 blocks VU even with scounteren=1 ---- */

TEST_REGISTER(test_hzcnt_08);
bool test_hzcnt_08(void)
{
    TEST_BEGIN("HZCNT-08: hcounteren.CY=0 blocks VU cycle (cause=22)");
    REQUIRE_H_EXT();
    REQUIRE_ZICNTR();

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();
    uintptr_t saved_scen = scounteren_read();

    if (!mcounteren_bit_set_sticky(CNT_BIT_CY)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren.CY is read-only zero");
    }
    if (!hcounteren_bit_clear_sticky(CNT_BIT_CY)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("hcounteren.CY is not clearable");
    }
    if (!scounteren_bit_set_sticky(CNT_BIT_CY)) {
        scounteren_write(saved_scen);
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("scounteren.CY is read-only zero");
    }

    trap_expect_begin();
    (void)run_in_vu_mode(_vs_read_cycle, 0);
    TEST_ASSERT("VU cycle read trapped (hcounteren.CY=0)",
                trap_was_triggered());
    if (trap_was_triggered()) {
        /* hcounteren layer blocks first: virtual-instruction. This is
         * the branch not covered by Hypervisor_CSR VCSR-17. */
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();

    scounteren_write(saved_scen);
    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}

/* ---- HZCNT-09: scounteren does NOT gate VS-mode ---- */

TEST_REGISTER(test_hzcnt_09);
bool test_hzcnt_09(void)
{
    TEST_BEGIN("HZCNT-09: scounteren.CY=0 does not affect VS-mode");
    REQUIRE_H_EXT();
    REQUIRE_ZICNTR();

    uintptr_t saved_mcen = mcounteren_read();
    uintptr_t saved_hcen = hcounteren_read();
    uintptr_t saved_scen = scounteren_read();

    if (!mcounteren_bit_set_sticky(CNT_BIT_CY)) {
        mcounteren_write(saved_mcen);
        TEST_SKIP("mcounteren.CY is read-only zero");
    }
    if (!hcounteren_bit_set_sticky(CNT_BIT_CY)) {
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("hcounteren.CY is read-only zero");
    }
    if (!scounteren_bit_clear_sticky(CNT_BIT_CY)) {
        scounteren_write(saved_scen);
        hcounteren_write(saved_hcen);
        mcounteren_write(saved_mcen);
        TEST_SKIP("scounteren.CY is not clearable");
    }

    /* scounteren constrains VU only; VS-mode reads must succeed. */
    trap_expect_begin();
    (void)run_in_vs_mode(_vs_read_cycle, 0);
    TEST_ASSERT("VS cycle read no trap (scounteren.CY=0)",
                !trap_was_triggered());
    trap_expect_end();

    scounteren_write(saved_scen);
    hcounteren_write(saved_hcen);
    mcounteren_write(saved_mcen);
    HYP_TEST_END();
}
