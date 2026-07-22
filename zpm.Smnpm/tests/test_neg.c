/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"
#include "pm/pm_addr.h"
#include "vm/vm.h"
#include "zpm_cap_result.h"

/* ===================================================================
 * Group 7.b: PM Non-Application Scenarios - S-mode (Smnpm)
 *
 * ZPM-NEG-03, ZPM-NEG-04, ZPM-NEG-05
 *
 * Verifies that PM does NOT apply to:
 *   - CSR software read/write
 *   - CSR WARL width
 *   - SFENCE.VMA address parameter
 * =================================================================== */

/* ----- ZPM-NEG-03: CSR write not affected by PM ----- */
TEST_REGISTER(test_zpm_neg_csr_write_no_pm);
bool test_zpm_neg_csr_write_no_pm(void) {
    TEST_BEGIN("ZPM-NEG-03: CSR write not affected by PM");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    /* Construct a tagged address */
    uintptr_t base = 0x0000008012345678ULL;
    uintptr_t tagged = pm_tag_address(base, 0x7F, 7);

    /* Write tagged value to sepc CSR */
    uintptr_t saved_sepc = CSRR(0x141);
    CSRW(0x141, tagged);  /* sepc */
    uintptr_t readback = CSRR(0x141);

    /* CSR software read/write is not affected by PM.
     * The tag bits should be preserved in the CSR value.
     * Note: sepc is WARL, low bit may be cleared, but upper tag
     * bits should be preserved. */
    uintptr_t tag_readback = pm_extract_tag(readback, 7);
    uintptr_t tag_written = pm_extract_tag(tagged, 7);
    TEST_ASSERT_EQ("tag preserved in CSR", tag_readback, tag_written);

    /* Restore sepc */
    CSRW(0x141, saved_sepc);
    pm_set_smode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-NEG-04: CSR WARL width not affected by PM ----- */
TEST_REGISTER(test_zpm_neg_csr_warl_no_pm);
bool test_zpm_neg_csr_warl_no_pm(void) {
    TEST_BEGIN("ZPM-NEG-04: CSR WARL width not affected by PM");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");

    /* Read a WARL CSR without PM, then with PM, compare behavior */
    uintptr_t saved_sepc = CSRR(0x141);

    /* Write all-ones without PM */
    pm_set_smode(PMM_DISABLED);
    CSRW(0x141, ~(uintptr_t)0);
    uintptr_t warl_no_pm = CSRR(0x141);

    /* Write all-ones with PM */
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) {
        CSRW(0x141, saved_sepc);
        TEST_SKIP("PMLEN=7 not supported");
    }
    CSRW(0x141, ~(uintptr_t)0);
    uintptr_t warl_with_pm = CSRR(0x141);

    TEST_ASSERT_EQ("WARL width unchanged by PM", warl_with_pm, warl_no_pm);

    CSRW(0x141, saved_sepc);
    pm_set_smode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-NEG-05: SFENCE.VMA addr not affected by PM ----- */
TEST_REGISTER(test_zpm_neg_sfence_no_pm);
bool test_zpm_neg_sfence_no_pm(void) {
    TEST_BEGIN("ZPM-NEG-05: SFENCE.VMA addr not affected by PM");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    /* Execute sfence.vma with a tagged address.
     * This should NOT fault - sfence.vma always executes successfully
     * regardless of the address value. The address is used as a TLB
     * invalidation hint and is not PM-transformed. */
    uintptr_t tagged_addr = pm_tag_address(0x80001000ULL, 0x55, 7);

    trap_expect_begin();
    asm volatile("sfence.vma %0, zero" :: "r"(tagged_addr) : "memory");
    TEST_ASSERT("sfence.vma with tagged addr no fault",
                !trap_was_triggered());
    trap_expect_end();

    pm_set_smode(PMM_DISABLED);
    TEST_END();
}
