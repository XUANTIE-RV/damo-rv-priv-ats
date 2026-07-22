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
 * Group 3: Ssnpm — U-mode VA Ignore Transform (load/store)
 *
 * ZPM-UVA-01 ~ ZPM-UVA-08
 *
 * Verifies that Ssnpm correctly applies the "ignore" transformation
 * (sign-extend from bit XLEN-PMLEN-1) for tagged load/store in
 * U-mode under Sv39 virtual memory.
 * =================================================================== */

/* Shared test data */
static volatile uint64_t uva_test_data = 0xDEADBEEFCAFE1234ULL;
static volatile uint64_t uva_store_target = 0;

/* U-mode helper functions (executed via vm_run_in_umode) */
static uintptr_t umode_load64(uintptr_t addr) {
    return *(volatile uint64_t *)addr;
}

static uintptr_t umode_store64(uintptr_t addr) {
    *(volatile uint64_t *)addr = 0xA5A5A5A5A5A5A5A5ULL;
    return 0;
}

/* Helper: set up VM context for U-mode with identity mapping */
static void setup_umode_vm(pt_context_t *ctx) {
    pt_pool_reset();
    pt_init(ctx, SATP_MODE_SV39);
    pt_setup_identity_mapping(ctx, PLATFORM_MEM_BASE, PLATFORM_MEM_SIZE,
        PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D,
        PT_LEVEL_2M);
}

static void cleanup_vm(void) {
    pt_pool_reset();
}

/* ----- ZPM-UVA-01: PMLEN7 tagged load ----- */
TEST_REGISTER(test_zpm_uva_pmlen7_load);
bool test_zpm_uva_pmlen7_load(void) {
    TEST_BEGIN("ZPM-UVA-01: PMLEN7 tagged load in U-mode");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    uva_test_data = 0xDEADBEEFCAFE1234ULL;
    uintptr_t base = (uintptr_t)&uva_test_data;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(7), 7);
    TEST_ASSERT("tagged differs from base", tagged != base);

    trap_expect_begin();
    uintptr_t result = vm_run_in_umode(&ctx, umode_load64, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged load should not fault (PM not working?)", false);
        pm_set_umode(PMM_DISABLED);
        cleanup_vm();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("correct value via tagged load",
                    result, 0xDEADBEEFCAFE1234ULL);

    pm_set_umode(PMM_DISABLED);
    cleanup_vm();
    TEST_END();
}

/* ----- ZPM-UVA-02: PMLEN16 tagged load ----- */
TEST_REGISTER(test_zpm_uva_pmlen16_load);
bool test_zpm_uva_pmlen16_load(void) {
    TEST_BEGIN("ZPM-UVA-02: PMLEN16 tagged load in U-mode");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN16);
    if (pm_get_umode() != PMM_PMLEN16) TEST_SKIP("PMLEN=16 not supported");

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    uva_test_data = 0x1122334455667788ULL;
    uintptr_t base = (uintptr_t)&uva_test_data;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(16), 16);
    TEST_ASSERT("tagged differs from base", tagged != base);

    trap_expect_begin();
    uintptr_t result = vm_run_in_umode(&ctx, umode_load64, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged load should not fault (PM not working?)", false);
        pm_set_umode(PMM_DISABLED);
        cleanup_vm();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("correct value via tagged load",
                    result, 0x1122334455667788ULL);

    pm_set_umode(PMM_DISABLED);
    cleanup_vm();
    TEST_END();
}

/* ----- ZPM-UVA-03: PMLEN7 tagged store ----- */
TEST_REGISTER(test_zpm_uva_pmlen7_store);
bool test_zpm_uva_pmlen7_store(void) {
    TEST_BEGIN("ZPM-UVA-03: PMLEN7 tagged store in U-mode");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    uva_store_target = 0;
    uintptr_t base = (uintptr_t)&uva_store_target;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(7), 7);

    trap_expect_begin();
    vm_run_in_umode(&ctx, umode_store64, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged store should not fault (PM not working?)", false);
        pm_set_umode(PMM_DISABLED);
        cleanup_vm();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("store via tagged addr correct",
                    uva_store_target, 0xA5A5A5A5A5A5A5A5ULL);

    pm_set_umode(PMM_DISABLED);
    cleanup_vm();
    TEST_END();
}

/* ----- ZPM-UVA-04: PMLEN16 tagged store ----- */
TEST_REGISTER(test_zpm_uva_pmlen16_store);
bool test_zpm_uva_pmlen16_store(void) {
    TEST_BEGIN("ZPM-UVA-04: PMLEN16 tagged store in U-mode");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN16);
    if (pm_get_umode() != PMM_PMLEN16) TEST_SKIP("PMLEN=16 not supported");

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    uva_store_target = 0;
    uintptr_t base = (uintptr_t)&uva_store_target;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(16), 16);

    trap_expect_begin();
    vm_run_in_umode(&ctx, umode_store64, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged store should not fault (PM not working?)", false);
        pm_set_umode(PMM_DISABLED);
        cleanup_vm();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("store via tagged addr correct",
                    uva_store_target, 0xA5A5A5A5A5A5A5A5ULL);

    pm_set_umode(PMM_DISABLED);
    cleanup_vm();
    TEST_END();
}

/* ----- ZPM-UVA-05: Different tags access same location ----- */
TEST_REGISTER(test_zpm_uva_different_tags);
bool test_zpm_uva_different_tags(void) {
    TEST_BEGIN("ZPM-UVA-05: Different tags access same location");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    uva_test_data = 0xBEEFBEEFBEEFBEEFULL;
    uintptr_t base = (uintptr_t)&uva_test_data;
    uintptr_t tagged_a = pm_tag_address(base, 0x55, 7);
    uintptr_t tagged_b = pm_tag_address(base, 0x7F, 7);

    TEST_ASSERT("tags differ", tagged_a != tagged_b);

    trap_expect_begin();
    uintptr_t val_a = vm_run_in_umode(&ctx, umode_load64, tagged_a);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged load (tag_a) should not fault", false);
        pm_set_umode(PMM_DISABLED);
        cleanup_vm();
        TEST_END();
    }
    trap_expect_end();

    trap_expect_begin();
    uintptr_t val_b = vm_run_in_umode(&ctx, umode_load64, tagged_b);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged load (tag_b) should not fault", false);
        pm_set_umode(PMM_DISABLED);
        cleanup_vm();
        TEST_END();
    }
    trap_expect_end();

    TEST_ASSERT_EQ("tag 0x55 reads correct value",
                    val_a, 0xBEEFBEEFBEEFBEEFULL);
    TEST_ASSERT_EQ("tag 0x7F reads same value",
                    val_b, 0xBEEFBEEFBEEFBEEFULL);

    pm_set_umode(PMM_DISABLED);
    cleanup_vm();
    TEST_END();
}

/* ----- ZPM-UVA-06: tag=0 equivalent to no tag ----- */
TEST_REGISTER(test_zpm_uva_tag_zero);
bool test_zpm_uva_tag_zero(void) {
    TEST_BEGIN("ZPM-UVA-06: tag=0 equivalent to no tag");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    uva_test_data = 0xCAFEBABEDEADFACEULL;
    uintptr_t base = (uintptr_t)&uva_test_data;
    uintptr_t tagged_zero = pm_tag_address(base, 0, 7);

    /* tag=0 should not alter the address for typical user-space addresses */
    trap_expect_begin();
    uintptr_t result = vm_run_in_umode(&ctx, umode_load64, tagged_zero);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tag=0 load should not fault", false);
        pm_set_umode(PMM_DISABLED);
        cleanup_vm();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("tag=0 load correct",
                    result, 0xCAFEBABEDEADFACEULL);

    pm_set_umode(PMM_DISABLED);
    cleanup_vm();
    TEST_END();
}

/* ----- ZPM-UVA-07: PM disabled, tagged addr causes fault ----- */
TEST_REGISTER(test_zpm_uva_disabled_fault);
bool test_zpm_uva_disabled_fault(void) {
    TEST_BEGIN("ZPM-UVA-07: PM disabled, tagged addr page-fault");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");

    /* Ensure PM is disabled */
    pm_set_umode(PMM_DISABLED);

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    uva_test_data = 0x1234567890ABCDEFULL;
    uintptr_t base = (uintptr_t)&uva_test_data;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(7), 7);

    /* With PM disabled, the tagged address should not be transformed.
     * The tagged VA has upper bits set, so it won't match any valid
     * Sv39 mapping -> expect page fault.
     * vm_run_in_umode will handle the fault via trap mechanism. */
    trap_expect_begin();
    vm_run_in_umode(&ctx, umode_load64, tagged);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        uintptr_t cause = trap_get_cause();
        TEST_ASSERT("page fault or access fault",
                     cause == CAUSE_LOAD_PAGE_FAULT ||
                     cause == CAUSE_LOAD_ACCESS_FAULT);
    }
    trap_expect_end();

    cleanup_vm();
    TEST_END();
}

/* ----- ZPM-UVA-08: sign-extend correctness ----- */
TEST_REGISTER(test_zpm_uva_sign_extend);
bool test_zpm_uva_sign_extend(void) {
    TEST_BEGIN("ZPM-UVA-08: VA sign-extend correctness");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");

    /* Verify the software transform matches expected sign-extend behavior */
    unsigned pmlen = 7;
    uintptr_t base_low = 0x0000000080001000ULL;  /* bit 56 = 0 */
    uintptr_t tagged_low = pm_tag_address(base_low, 0x7F, pmlen);
    uintptr_t transformed_low = pm_transform_va(tagged_low, pmlen);

    /* bit(XLEN-PMLEN-1) = bit(56) = 0, so upper bits should be 0 */
    TEST_ASSERT_EQ("bit56=0: sign-extend produces zeros in upper bits",
                    transformed_low, base_low);

    uintptr_t base_high = 0x0100000080001000ULL;  /* bit 56 = 1 */
    uintptr_t tagged_high = pm_tag_address(base_high, 0x55, pmlen);
    uintptr_t transformed_high = pm_transform_va(tagged_high, pmlen);

    /* bit(56) = 1, so upper PMLEN bits should all be 1 */
    uintptr_t expected_upper = ((uintptr_t)pm_max_tag(pmlen)) << (64 - pmlen);
    TEST_ASSERT("bit56=1: upper bits all ones",
                (transformed_high & expected_upper) == expected_upper);

    /* The lower (XLEN-PMLEN) bits should be preserved */
    uintptr_t lower_mask = (1ULL << (64 - pmlen)) - 1;
    TEST_ASSERT_EQ("lower bits preserved",
                    transformed_high & lower_mask,
                    base_high & lower_mask);

    pm_set_umode(PMM_DISABLED);
    TEST_END();
}
