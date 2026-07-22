/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"
#include "pm/pm_addr.h"
#include "zpm_cap_result.h"

/* ===================================================================
 * Group 9.b: PM and MXR Interaction - M-mode (Smmpm)
 *
 * ZPM-MXR-03, ZPM-MXR-04
 *
 * When MXR is in effect at the effective privilege mode, PM does
 * NOT apply. This group verifies this interaction for M-mode
 * Bare scenarios.
 * =================================================================== */

static volatile uint64_t mxr_data __attribute__((aligned(8)))
    = 0xAA11BB22CC33DD44ULL;

/* ----- ZPM-MXR-03: MXR=1 Bare mode also disables PM ----- */
TEST_REGISTER(test_zpm_mxr_bare_disables_pm);
bool test_zpm_mxr_bare_disables_pm(void) {
    TEST_BEGIN("ZPM-MXR-03: MXR=1 Bare mode also disables PM");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    mxr_data = 0xDEADFACECAFEBEEFULL;
    uintptr_t base = (uintptr_t)&mxr_data;
    uintptr_t tagged = pm_tag_address(base, 0x7F, 7);

    /* Enable MXR in M-mode */
    uintptr_t mstatus = CSRR(mstatus);
    CSRW(mstatus, mstatus | MSTATUS_MXR_BIT);

    /* MXR=1 in M-mode Bare should disable PM.
     * Tagged PA used as-is -> likely fault or wrong value. */
    trap_expect_begin();
    volatile uint64_t val = *(volatile uint64_t *)tagged;
    (void)val;
    bool trapped = trap_was_triggered();
    trap_expect_end();

    CSRW(mstatus, mstatus);

    if (trapped) {
        TEST_ASSERT("MXR=1 Bare: PM disabled, tagged PA faults", true);
    } else {
        TEST_ASSERT("MXR=1 Bare: PM disabled, wrong value or same",
                     val != 0xDEADFACECAFEBEEFULL || tagged == base);
    }

    pm_set_mmode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-MXR-04: Dynamic MXR toggle changes PM behavior ----- */
TEST_REGISTER(test_zpm_mxr_toggle);
bool test_zpm_mxr_toggle(void) {
    TEST_BEGIN("ZPM-MXR-04: Dynamic MXR toggle changes PM behavior");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    mxr_data = 0x1234ABCD5678EF01ULL;
    uintptr_t base = (uintptr_t)&mxr_data;
    uintptr_t tagged = pm_tag_address(base, 0x33, 7);
    uintptr_t mstatus = CSRR(mstatus);

    /* MXR=0: PM active, tagged load should work */
    CSRW(mstatus, mstatus & ~MSTATUS_MXR_BIT);
    trap_expect_begin();
    uint64_t val_no_mxr = *(volatile uint64_t *)tagged;
    if (trap_was_triggered()) {
        trap_expect_end();
        CSRW(mstatus, mstatus);
        TEST_ASSERT("MXR=0 tagged load should not fault (PM not working?)", false);
        pm_set_mmode(PMM_DISABLED);
        TEST_END();
    }
    trap_expect_end();

    /* MXR=1: PM disabled, tagged load should fail or read wrong */
    CSRW(mstatus, mstatus | MSTATUS_MXR_BIT);
    trap_expect_begin();
    volatile uint64_t val_mxr = *(volatile uint64_t *)tagged;
    (void)val_mxr;
    bool trapped = trap_was_triggered();
    trap_expect_end();

    CSRW(mstatus, mstatus);

    TEST_ASSERT_EQ("MXR=0: tagged load correct",
                    val_no_mxr, 0x1234ABCD5678EF01ULL);
    TEST_ASSERT("MXR=1: PM behavior changed",
                 trapped || val_mxr != 0x1234ABCD5678EF01ULL);

    pm_set_mmode(PMM_DISABLED);
    TEST_END();
}
