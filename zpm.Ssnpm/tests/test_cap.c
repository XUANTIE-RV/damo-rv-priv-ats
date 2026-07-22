/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"
#include "pm/pm_addr.h"
#include "zpm_cap_result.h"

/* ===================================================================
 * Group 1.a: Ssnpm Hardware Capability Detection
 *
 * ZPM-CAP-01, ZPM-CAP-04
 *
 * Detects whether Ssnpm is implemented and which PMLEN values
 * (7/16) it supports.
 *
 * Global results are stored so that subsequent groups can skip
 * tests when Ssnpm is not present.
 * =================================================================== */

/* Global CAP detection result */
bool zpm_cap_ssnpm_supported = false;

/* ----- ZPM-CAP-01: Detect Ssnpm implementation ----- */
TEST_REGISTER(test_zpm_cap_detect_ssnpm);
bool test_zpm_cap_detect_ssnpm(void) {
    TEST_BEGIN("ZPM-CAP-01: Detect Ssnpm implementation");

    bool has_ssnpm = detect_ssnpm();
    zpm_cap_ssnpm_supported = has_ssnpm;
    TEST_ASSERT("Platform does not implement Ssnpm", has_ssnpm);

    TEST_END();
}

/* ----- ZPM-CAP-04: Ssnpm supported PMLEN ----- */
TEST_REGISTER(test_zpm_cap_ssnpm_pmlen);
bool test_zpm_cap_ssnpm_pmlen(void) {
    TEST_BEGIN("ZPM-CAP-04: Ssnpm supported PMLEN values");
    if (!detect_ssnpm()) TEST_SKIP("Ssnpm not implemented");

    unsigned saved = pm_get_umode();

    pm_set_umode(PMM_PMLEN7);
    bool pmlen7 = (pm_get_umode() == PMM_PMLEN7);

    pm_set_umode(PMM_PMLEN16);
    bool pmlen16 = (pm_get_umode() == PMM_PMLEN16);

    pm_set_umode(saved);

    printf("  PMLEN=7:  %s\n", pmlen7  ? "supported" : "not supported");
    printf("  PMLEN=16: %s\n", pmlen16 ? "supported" : "not supported");

    TEST_ASSERT("at least one PMLEN supported", pmlen7 || pmlen16);
    TEST_END();
}
