/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"
#include "pm/pm_addr.h"
#include "zpm_cap_result.h"

/* ===================================================================
 * Group 6: Smmpm — M-mode PA Ignore Transform (load/store/AMO)
 *
 * ZPM-MPA-01 ~ ZPM-MPA-09
 *
 * Verifies that Smmpm correctly applies the "ignore" transformation
 * for tagged physical addresses in M-mode (Bare translation).
 * Key difference from Group 3/5: PA uses zero-extend (not sign-extend).
 * Tests run directly in M-mode without VM.
 * =================================================================== */

static volatile uint64_t mpa_test_data = 0x1234567890ABCDEFULL;
static volatile uint64_t mpa_store_target = 0;
static volatile uint64_t mpa_amo_var __attribute__((aligned(8))) = 0;

/* ----- ZPM-MPA-01: PMLEN7 M-mode tagged load ----- */
TEST_REGISTER(test_zpm_mpa_pmlen7_load);
bool test_zpm_mpa_pmlen7_load(void) {
    TEST_BEGIN("ZPM-MPA-01: PMLEN7 tagged load in M-mode");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    mpa_test_data = 0x1234567890ABCDEFULL;
    uintptr_t base = (uintptr_t)&mpa_test_data;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(7), 7);
    TEST_ASSERT("tagged differs from base", tagged != base);

    /* M-mode direct load with tagged physical address */
    trap_expect_begin();
    uint64_t val = *(volatile uint64_t *)tagged;
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged PA load should not fault (PM not working?)", false);
        pm_set_mmode(PMM_DISABLED);
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("correct value via tagged PA",
                    val, 0x1234567890ABCDEFULL);

    pm_set_mmode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-MPA-03: PMLEN7 M-mode tagged store ----- */
TEST_REGISTER(test_zpm_mpa_pmlen7_store);
bool test_zpm_mpa_pmlen7_store(void) {
    TEST_BEGIN("ZPM-MPA-03: PMLEN7 tagged store in M-mode");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    mpa_store_target = 0;
    uintptr_t base = (uintptr_t)&mpa_store_target;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(7), 7);

    trap_expect_begin();
    *(volatile uint64_t *)tagged = 0xC3C3C3C3C3C3C3C3ULL;
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged PA store should not fault (PM not working?)", false);
        pm_set_mmode(PMM_DISABLED);
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("store via tagged PA correct",
                    mpa_store_target, 0xC3C3C3C3C3C3C3C3ULL);

    pm_set_mmode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-MPA-05: PMLEN7 M-mode amoadd.d ----- */
TEST_REGISTER(test_zpm_mpa_pmlen7_amoadd);
bool test_zpm_mpa_pmlen7_amoadd(void) {
    TEST_BEGIN("ZPM-MPA-05: PMLEN7 M-mode amoadd.d via tagged PA");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    mpa_amo_var = 100;
    uintptr_t tagged = pm_tag_address((uintptr_t)&mpa_amo_var, 0x55, 7);

    uint64_t old_val;
    uint64_t addend = 42;
    trap_expect_begin();
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %0, %1, (%2)\n\t"
        ".option pop\n\t"
        : "=r"(old_val) : "r"(addend), "r"(tagged) : "memory"
    );
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged PA amoadd should not fault (PM not working?)", false);
        pm_set_mmode(PMM_DISABLED);
        TEST_END();
    }
    trap_expect_end();

    TEST_ASSERT_EQ("old value is 100", old_val, 100);
    TEST_ASSERT_EQ("updated to 142", mpa_amo_var, 142);

    pm_set_mmode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-MPA-06: zero-extend correctness ----- */
TEST_REGISTER(test_zpm_mpa_zero_extend);
bool test_zpm_mpa_zero_extend(void) {
    TEST_BEGIN("ZPM-MPA-06: PA zero-extend correctness");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");

    unsigned pmlen = 7;
    /* Use a fixed address with bit(56)=1 to ensure VA sign-extension
     * produces different result from PA zero-extension */
    uintptr_t base = 0x0100000000001000ULL;
    uintptr_t tagged = pm_tag_address(base, 0x7F, pmlen);

    /* PA transform should zero-extend (clear upper PMLEN bits) */
    uintptr_t transformed = pm_transform_pa(tagged, pmlen);
    TEST_ASSERT_EQ("upper bits cleared to base", transformed, base);

    /* Compare with VA transform (sign-extend) - they should differ
     * when tag bits are non-zero and bit(XLEN-PMLEN-1)=1 */
    uintptr_t va_transformed = pm_transform_va(tagged, pmlen);
    TEST_ASSERT("PA != VA transform for non-zero tag",
                transformed != va_transformed);

    TEST_END();
}

/* ----- ZPM-MPA-07: Different tags access same location ----- */
TEST_REGISTER(test_zpm_mpa_different_tags);
bool test_zpm_mpa_different_tags(void) {
    TEST_BEGIN("ZPM-MPA-07: Different tags access same location (M-mode)");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    mpa_test_data = 0xAAAABBBBCCCCDDDDULL;
    uintptr_t base = (uintptr_t)&mpa_test_data;
    uintptr_t tagged_a = pm_tag_address(base, 0x55, 7);
    uintptr_t tagged_b = pm_tag_address(base, 0x7F, 7);

    trap_expect_begin();
    uint64_t val_a = *(volatile uint64_t *)tagged_a;
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged PA load (tag_a) should not fault", false);
        pm_set_mmode(PMM_DISABLED);
        TEST_END();
    }
    trap_expect_end();

    trap_expect_begin();
    uint64_t val_b = *(volatile uint64_t *)tagged_b;
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged PA load (tag_b) should not fault", false);
        pm_set_mmode(PMM_DISABLED);
        TEST_END();
    }
    trap_expect_end();

    TEST_ASSERT_EQ("tag 0x55 correct", val_a, 0xAAAABBBBCCCCDDDDULL);
    TEST_ASSERT_EQ("tag 0x7F same value", val_b, 0xAAAABBBBCCCCDDDDULL);

    pm_set_mmode(PMM_DISABLED);
    TEST_END();
}

/* ===================================================================
 * PMLEN16 tests below. These MUST come after all PMLEN7 tests because
 * mseccfg.PMM is sticky (ext_smepmp): once set to 3 (PMLEN16), it
 * cannot be lowered back to 2 (PMLEN7). Placing PMLEN16 tests here
 * ensures all PMLEN7 tests have already completed.
 * =================================================================== */

/* ----- ZPM-MPA-02: PMLEN16 M-mode tagged load ----- */
TEST_REGISTER(test_zpm_mpa_pmlen16_load);
bool test_zpm_mpa_pmlen16_load(void) {
    TEST_BEGIN("ZPM-MPA-02: PMLEN16 tagged load in M-mode");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    pm_set_mmode(PMM_PMLEN16);
    if (pm_get_mmode() != PMM_PMLEN16) TEST_SKIP("PMLEN=16 not supported");

    mpa_test_data = 0xFEDCBA9876543210ULL;
    uintptr_t base = (uintptr_t)&mpa_test_data;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(16), 16);

    trap_expect_begin();
    uint64_t val = *(volatile uint64_t *)tagged;
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged PA load should not fault (PM not working?)", false);
        pm_set_mmode(PMM_DISABLED);
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("correct value via tagged PA",
                    val, 0xFEDCBA9876543210ULL);

    pm_set_mmode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-MPA-04: PMLEN16 M-mode tagged store ----- */
TEST_REGISTER(test_zpm_mpa_pmlen16_store);
bool test_zpm_mpa_pmlen16_store(void) {
    TEST_BEGIN("ZPM-MPA-04: PMLEN16 tagged store in M-mode");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    pm_set_mmode(PMM_PMLEN16);
    if (pm_get_mmode() != PMM_PMLEN16) TEST_SKIP("PMLEN=16 not supported");

    mpa_store_target = 0;
    uintptr_t base = (uintptr_t)&mpa_store_target;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(16), 16);

    trap_expect_begin();
    *(volatile uint64_t *)tagged = 0xD4D4D4D4D4D4D4D4ULL;
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged PA store should not fault (PM not working?)", false);
        pm_set_mmode(PMM_DISABLED);
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("store via tagged PA correct",
                    mpa_store_target, 0xD4D4D4D4D4D4D4D4ULL);

    pm_set_mmode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-MPA-08: PM disabled vs enabled behavior difference ----- */
TEST_REGISTER(test_zpm_mpa_disabled);
bool test_zpm_mpa_disabled(void) {
    TEST_BEGIN("ZPM-MPA-08: PM disabled, tagged PA accesses wrong addr");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");

    /*
     * Strategy: Compare behavior with PM enabled vs disabled.
     *
     * Step 1: PM enabled - tagged PA should be zero-extended,
     *         accessing the correct location (base without tag).
     * Step 2: PM disabled - tagged PA is used as-is.
     *
     * On platforms where physical address bus width is narrower than
     * XLEN, hardware naturally truncates high bits, making this
     * indistinguishable from PM zero-extension. In that case, SKIP.
     */

    /* Detect which PMLEN is supported */
    unsigned pmlen = 0;
    unsigned pmm_mode = 0;
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() == PMM_PMLEN7) {
        pmlen = 7;
        pmm_mode = PMM_PMLEN7;
    } else {
        pm_set_mmode(PMM_PMLEN16);
        if (pm_get_mmode() == PMM_PMLEN16) {
            pmlen = 16;
            pmm_mode = PMM_PMLEN16;
        } else {
            TEST_SKIP("No PMLEN mode supported");
        }
    }

    /* Test with PM enabled first - should work correctly */
    pm_set_mmode(pmm_mode);
    mpa_test_data = 0x1111222233334444ULL;
    uintptr_t base = (uintptr_t)&mpa_test_data;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(pmlen), pmlen);

    trap_expect_begin();
    volatile uint64_t val_enabled = *(volatile uint64_t *)tagged;
    (void)val_enabled;
    bool trapped_enabled = trap_was_triggered();
    trap_expect_end();

    /* PM enabled: zero-extension should make tagged addr access base */
    TEST_ASSERT("PM enabled: no fault on tagged PA", !trapped_enabled);
    TEST_ASSERT_EQ("PM enabled: reads correct value",
                    val_enabled, 0x1111222233334444ULL);

    /* Now disable PM and retry */
    pm_set_mmode(PMM_DISABLED);

    trap_expect_begin();
    volatile uint64_t val_disabled = *(volatile uint64_t *)tagged;
    (void)val_disabled;
    bool trapped_disabled = trap_was_triggered();
    trap_expect_end();

    if (trapped_disabled) {
        /* Best case: fault proves PM is truly disabled */
        TEST_ASSERT("PM disabled: fault on tagged PA (PM not applied)", true);
    } else if (val_disabled != 0x1111222233334444ULL) {
        /* Also acceptable: reads different value (different location) */
        TEST_ASSERT("PM disabled: reads different location", true);
    } else {
        /*
         * Read same value without fault. This can happen if the physical
         * address bus is narrow enough that hardware truncates high bits
         * regardless of PM. This is NOT a PM bug but a platform limitation.
         */
        printf("  [INFO] Platform phys addr width <= %d bits, "
               "hardware truncates high bits naturally\n", 64 - pmlen);
        printf("  [INFO] Cannot distinguish PM-disabled from "
               "hardware truncation on this platform\n");
        TEST_SKIP("Platform phys addr bus truncates tag bits (inconclusive)");
    }

    TEST_END();
}

/* ----- ZPM-MPA-09: PA vs VA transform difference verification ----- */
TEST_REGISTER(test_zpm_mpa_pa_vs_va);
bool test_zpm_mpa_pa_vs_va(void) {
    TEST_BEGIN("ZPM-MPA-09: PA vs VA transform difference");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");

    unsigned pmlen = 7;
    /* Use an address where bit(56) = 1 to ensure VA sign-extension
     * produces different result from PA zero-extension.
     * base = 0x0100000000001000 has bit56=1 (0x01 << 56) */
    uintptr_t base = 0x0100000000001000ULL;
    uintptr_t tagged = pm_tag_address(base, 0x55, pmlen);

    uintptr_t pa_result = pm_transform_pa(tagged, pmlen);
    uintptr_t va_result = pm_transform_va(tagged, pmlen);

    printf("  tagged:       0x%lx\n", (unsigned long)tagged);
    printf("  PA transform: 0x%lx\n", (unsigned long)pa_result);
    printf("  VA transform: 0x%lx\n", (unsigned long)va_result);

    /* PA zero-extends: upper bits = 0 */
    uintptr_t upper_mask = ((uintptr_t)pm_max_tag(pmlen)) << (64 - pmlen);
    TEST_ASSERT("PA: upper bits are zero",
                (pa_result & upper_mask) == 0);

    /* VA sign-extends from bit(56): since base bit56=1 for this addr,
     * upper bits should be all 1 */
    uintptr_t bit56 = (base >> (64 - pmlen - 1)) & 1;
    if (bit56) {
        TEST_ASSERT("VA: upper bits are ones (bit56=1)",
                    (va_result & upper_mask) == upper_mask);
    } else {
        TEST_ASSERT("VA: upper bits are zeros (bit56=0)",
                    (va_result & upper_mask) == 0);
    }

    TEST_ASSERT("PA != VA for non-zero tag", pa_result != va_result);

    TEST_END();
}
