/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"
#include "pm/pm_addr.h"
#include "zpm_cap_result.h"

/* ===================================================================
 * Group 1.b: Smnpm Hardware Capability Detection
 *
 * ZPM-CAP-02, ZPM-CAP-05
 *
 * Detects whether Smnpm is implemented and which PMLEN values
 * (7/16) it supports.
 *
 * Global results are stored so that subsequent groups can skip
 * tests when Smnpm is not present.
 * =================================================================== */

/* Global CAP detection result */
bool zpm_cap_smnpm_supported = false;

/* ----- ZPM-CAP-02: Detect Smnpm implementation ----- */
TEST_REGISTER(test_zpm_cap_detect_smnpm);
bool test_zpm_cap_detect_smnpm(void) {
    TEST_BEGIN("ZPM-CAP-02: Detect Smnpm implementation");

    bool has_smnpm = detect_smnpm();
    zpm_cap_smnpm_supported = has_smnpm;
    TEST_ASSERT("Platform does not implement Smnpm", has_smnpm);

    TEST_END();
}

/* ----- ZPM-CAP-05: Smnpm supported PMLEN ----- */
TEST_REGISTER(test_zpm_cap_smnpm_pmlen);
bool test_zpm_cap_smnpm_pmlen(void) {
    TEST_BEGIN("ZPM-CAP-05: Smnpm supported PMLEN values");
    if (!detect_smnpm()) TEST_SKIP("Smnpm not implemented");

    unsigned saved = pm_get_smode();

    pm_set_smode(PMM_PMLEN7);
    bool pmlen7 = (pm_get_smode() == PMM_PMLEN7);

    pm_set_smode(PMM_PMLEN16);
    bool pmlen16 = (pm_get_smode() == PMM_PMLEN16);

    pm_set_smode(saved);

    printf("  PMLEN=7:  %s\n", pmlen7  ? "supported" : "not supported");
    printf("  PMLEN=16: %s\n", pmlen16 ? "supported" : "not supported");

    TEST_ASSERT("at least one PMLEN supported", pmlen7 || pmlen16);
    TEST_END();
}
