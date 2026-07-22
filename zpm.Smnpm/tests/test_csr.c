/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"
#include "zpm_cap_result.h"

/* ===================================================================
 * Group 2b: CSR PMM Field Control - menvcfg.PMM (Smnpm)
 *
 * ZPM-CSR-06 ~ ZPM-CSR-10
 *
 * Verifies read/write consistency, WARL constraints (reserved
 * encoding 0b01 rejected), PMM operations don't affect other
 * fields in the same CSR, and read-only zero when extension
 * is not implemented.
 * =================================================================== */

/* ----- ZPM-CSR-06: menvcfg.PMM writable 0->PMLEN7 ----- */
TEST_REGISTER(test_zpm_csr_menvcfg_write_pmlen7);
bool test_zpm_csr_menvcfg_write_pmlen7(void) {
    TEST_BEGIN("ZPM-CSR-06: menvcfg.PMM writable 0->PMLEN7");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");

    pm_set_smode(PMM_DISABLED);
    TEST_ASSERT("PMM starts at 0", pm_get_smode() == PMM_DISABLED);

    pm_set_smode(PMM_PMLEN7);
    unsigned readback = pm_get_smode();
    if (readback == PMM_PMLEN7) {
        TEST_ASSERT("PMM=PMLEN7 readback", readback == PMM_PMLEN7);
    } else {
        printf("  PMLEN=7 not supported (WARL), got %u\n", readback);
        TEST_ASSERT("WARL: value is legal", readback == PMM_DISABLED ||
                     readback == PMM_PMLEN16);
    }

    pm_set_smode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-CSR-07: menvcfg.PMM writable 0->PMLEN16 ----- */
TEST_REGISTER(test_zpm_csr_menvcfg_write_pmlen16);
bool test_zpm_csr_menvcfg_write_pmlen16(void) {
    TEST_BEGIN("ZPM-CSR-07: menvcfg.PMM writable 0->PMLEN16");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");

    pm_set_smode(PMM_DISABLED);
    pm_set_smode(PMM_PMLEN16);
    unsigned readback = pm_get_smode();
    if (readback == PMM_PMLEN16) {
        TEST_ASSERT("PMM=PMLEN16 readback", readback == PMM_PMLEN16);
    } else {
        printf("  PMLEN=16 not supported (WARL), got %u\n", readback);
        TEST_ASSERT("WARL: value is legal", readback == PMM_DISABLED ||
                     readback == PMM_PMLEN7);
    }

    pm_set_smode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-CSR-08: menvcfg.PMM clearable ----- */
TEST_REGISTER(test_zpm_csr_menvcfg_clear);
bool test_zpm_csr_menvcfg_clear(void) {
    TEST_BEGIN("ZPM-CSR-08: menvcfg.PMM clearable");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");

    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() == PMM_DISABLED)
        pm_set_smode(PMM_PMLEN16);

    pm_set_smode(PMM_DISABLED);
    TEST_ASSERT_EQ("PMM cleared to 0", pm_get_smode(), PMM_DISABLED);

    TEST_END();
}

/* ----- ZPM-CSR-09: menvcfg.PMM reserved value rejected ----- */
TEST_REGISTER(test_zpm_csr_menvcfg_reserved);
bool test_zpm_csr_menvcfg_reserved(void) {
    TEST_BEGIN("ZPM-CSR-09: menvcfg.PMM reserved value rejected");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");

    pm_set_smode(PMM_DISABLED);
    unsigned before = pm_get_smode();

    pm_set_smode(PMM_RESERVED);
    unsigned after = pm_get_smode();

    TEST_ASSERT_EQ("reserved write ignored", after, before);

    pm_set_smode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-CSR-10: menvcfg.PMM doesn't affect PBMTE/ADUE ----- */
TEST_REGISTER(test_zpm_csr_menvcfg_other_fields);
bool test_zpm_csr_menvcfg_other_fields(void) {
    TEST_BEGIN("ZPM-CSR-10: menvcfg.PMM doesn't affect PBMTE/ADUE");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");

    uintptr_t before = CSRR(0x30A) & ~MENVCFG_PMM_MASK;

    pm_set_smode(PMM_PMLEN7);
    uintptr_t during = CSRR(0x30A) & ~MENVCFG_PMM_MASK;

    pm_set_smode(PMM_DISABLED);
    uintptr_t after = CSRR(0x30A) & ~MENVCFG_PMM_MASK;

    TEST_ASSERT_EQ("other fields unchanged (during)", during, before);
    TEST_ASSERT_EQ("other fields unchanged (after)", after, before);

    TEST_END();
}
