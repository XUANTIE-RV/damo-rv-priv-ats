/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"
#include "pm/pm_addr.h"
#include "zpm_cap_result.h"

/* ===================================================================
 * Group 1.c: Smmpm Hardware Capability Detection
 *
 * ZPM-CAP-03, ZPM-CAP-06
 *
 * Detects whether Smmpm is implemented and which PMLEN values
 * (7/16) it supports.
 *
 * Global results are stored so that subsequent groups can skip
 * tests when Smmpm is not present.
 * =================================================================== */

/* Global CAP detection result */
bool zpm_cap_smmpm_supported = false;

/* ----- ZPM-CAP-03: Detect Smmpm implementation ----- */
TEST_REGISTER(test_zpm_cap_detect_smmpm);
bool test_zpm_cap_detect_smmpm(void) {
    TEST_BEGIN("ZPM-CAP-03: Detect Smmpm implementation");

    bool has_smmpm = detect_smmpm();
    zpm_cap_smmpm_supported = has_smmpm;
    TEST_ASSERT("Platform does not implement Smmpm", has_smmpm);

    TEST_END();
}

/* ----- ZPM-CAP-06: Smmpm supported PMLEN ----- */
TEST_REGISTER(test_zpm_cap_smmpm_pmlen);
bool test_zpm_cap_smmpm_pmlen(void) {
    TEST_BEGIN("ZPM-CAP-06: Smmpm supported PMLEN values");
    if (!detect_smmpm()) TEST_SKIP("Smmpm not implemented");

    /*
     * Use non-destructive detection for M-mode.
     * mseccfg.PMM is sticky (ext_smepmp): once set to 3, it cannot
     * be lowered to 2. We only write PMLEN7 (2) here to detect
     * support without poisoning the field for subsequent tests.
     */
    unsigned result = pm_detect_supported_pmlen_mmode();
    bool pmlen7  = (result & (1U << 0)) != 0;
    bool pmlen16 = (result & (1U << 1)) != 0;

    printf("  PMLEN=7:  %s\n", pmlen7  ? "supported" : "not supported");
    printf("  PMLEN=16: %s\n", pmlen16 ? "supported" : "not supported");

    TEST_ASSERT("at least one PMLEN supported", pmlen7 || pmlen16);
    TEST_END();
}
