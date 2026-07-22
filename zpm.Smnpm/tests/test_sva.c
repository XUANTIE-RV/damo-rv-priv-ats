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
 * Group 5: Smnpm — S-mode VA Ignore Transform (load/store/AMO)
 *
 * ZPM-SVA-01 ~ ZPM-SVA-09
 *
 * Verifies that Smnpm correctly applies the "ignore" transformation
 * for tagged load/store/AMO in S-mode under Sv39.
 * Key difference from Group 3/4: PM configured via menvcfg.PMM
 * (not senvcfg.PMM), affects S-mode (not U-mode).
 * S-mode pages do NOT have PTE_U set.
 * =================================================================== */

static volatile uint64_t sva_test_data = 0xCAFEBABE12345678ULL;
static volatile uint64_t sva_store_target = 0;
static volatile uint64_t sva_amo_var __attribute__((aligned(8))) = 0;

/* S-mode helper functions */
static uintptr_t smode_load64(uintptr_t addr) {
    return *(volatile uint64_t *)addr;
}

static uintptr_t smode_store64(uintptr_t addr) {
    *(volatile uint64_t *)addr = 0xB5B5B5B5B5B5B5B5ULL;
    return 0;
}

static uintptr_t smode_amoadd_42(uintptr_t addr) {
    uint64_t old_val;
    uint64_t addend = 42;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %0, %1, (%2)\n\t"
        ".option pop\n\t"
        : "=r"(old_val) : "r"(addend), "r"(addr) : "memory"
    );
    return old_val;
}

/* Helper: set up VM for S-mode (no PTE_U) */
static void setup_smode_vm(pt_context_t *ctx) {
    pt_pool_reset();
    pt_init(ctx, SATP_MODE_SV39);
    pt_setup_identity_mapping(ctx, PLATFORM_MEM_BASE, PLATFORM_MEM_SIZE,
        PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D,
        PT_LEVEL_2M);
}

/* ----- ZPM-SVA-01: PMLEN7 S-mode tagged load ----- */
TEST_REGISTER(test_zpm_sva_pmlen7_load);
bool test_zpm_sva_pmlen7_load(void) {
    TEST_BEGIN("ZPM-SVA-01: PMLEN7 tagged load in S-mode");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_smode_vm(&ctx);

    sva_test_data = 0xCAFEBABE12345678ULL;
    uintptr_t base = (uintptr_t)&sva_test_data;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(7), 7);
    TEST_ASSERT("tagged differs from base", tagged != base);

    trap_expect_begin();
    uintptr_t result = vm_run_in_smode(&ctx, smode_load64, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged load should not fault (PM not working?)", false);
        pm_set_smode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("correct value via tagged load",
                    result, 0xCAFEBABE12345678ULL);

    pm_set_smode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-SVA-02: PMLEN16 S-mode tagged load ----- */
TEST_REGISTER(test_zpm_sva_pmlen16_load);
bool test_zpm_sva_pmlen16_load(void) {
    TEST_BEGIN("ZPM-SVA-02: PMLEN16 tagged load in S-mode");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN16);
    if (pm_get_smode() != PMM_PMLEN16) TEST_SKIP("PMLEN=16 not supported");

    pt_context_t ctx;
    setup_smode_vm(&ctx);

    sva_test_data = 0xAABBCCDDEEFF0011ULL;
    uintptr_t base = (uintptr_t)&sva_test_data;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(16), 16);

    trap_expect_begin();
    uintptr_t result = vm_run_in_smode(&ctx, smode_load64, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged load should not fault (PM not working?)", false);
        pm_set_smode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("correct value via tagged load",
                    result, 0xAABBCCDDEEFF0011ULL);

    pm_set_smode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-SVA-03: PMLEN7 S-mode tagged store ----- */
TEST_REGISTER(test_zpm_sva_pmlen7_store);
bool test_zpm_sva_pmlen7_store(void) {
    TEST_BEGIN("ZPM-SVA-03: PMLEN7 tagged store in S-mode");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_smode_vm(&ctx);

    sva_store_target = 0;
    uintptr_t base = (uintptr_t)&sva_store_target;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(7), 7);

    trap_expect_begin();
    vm_run_in_smode(&ctx, smode_store64, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged store should not fault (PM not working?)", false);
        pm_set_smode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("store via tagged addr correct",
                    sva_store_target, 0xB5B5B5B5B5B5B5B5ULL);

    pm_set_smode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-SVA-04: PMLEN16 S-mode tagged store ----- */
TEST_REGISTER(test_zpm_sva_pmlen16_store);
bool test_zpm_sva_pmlen16_store(void) {
    TEST_BEGIN("ZPM-SVA-04: PMLEN16 tagged store in S-mode");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN16);
    if (pm_get_smode() != PMM_PMLEN16) TEST_SKIP("PMLEN=16 not supported");

    pt_context_t ctx;
    setup_smode_vm(&ctx);

    sva_store_target = 0;
    uintptr_t base = (uintptr_t)&sva_store_target;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(16), 16);

    trap_expect_begin();
    vm_run_in_smode(&ctx, smode_store64, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged store should not fault (PM not working?)", false);
        pm_set_smode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("store via tagged addr correct",
                    sva_store_target, 0xB5B5B5B5B5B5B5B5ULL);

    pm_set_smode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-SVA-05: PMLEN7 S-mode amoadd.d ----- */
TEST_REGISTER(test_zpm_sva_pmlen7_amoadd);
bool test_zpm_sva_pmlen7_amoadd(void) {
    TEST_BEGIN("ZPM-SVA-05: PMLEN7 S-mode amoadd.d via tagged addr");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_smode_vm(&ctx);

    sva_amo_var = 100;
    uintptr_t tagged = pm_tag_address((uintptr_t)&sva_amo_var, 0x55, 7);

    trap_expect_begin();
    uintptr_t old_val = vm_run_in_smode(&ctx, smode_amoadd_42, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged amoadd should not fault (PM not working?)", false);
        pm_set_smode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("old value is 100", old_val, 100);
    TEST_ASSERT_EQ("updated to 142", sva_amo_var, 142);

    pm_set_smode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-SVA-06: Different tags access same location ----- */
TEST_REGISTER(test_zpm_sva_different_tags);
bool test_zpm_sva_different_tags(void) {
    TEST_BEGIN("ZPM-SVA-06: Different tags access same location (S-mode)");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_smode_vm(&ctx);

    sva_test_data = 0x9999888877776666ULL;
    uintptr_t base = (uintptr_t)&sva_test_data;
    uintptr_t tagged_a = pm_tag_address(base, 0x11, 7);
    uintptr_t tagged_b = pm_tag_address(base, 0x33, 7);

    trap_expect_begin();
    uintptr_t val_a = vm_run_in_smode(&ctx, smode_load64, tagged_a);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged load (tag_a) should not fault", false);
        pm_set_smode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();

    trap_expect_begin();
    uintptr_t val_b = vm_run_in_smode(&ctx, smode_load64, tagged_b);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged load (tag_b) should not fault", false);
        pm_set_smode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();

    TEST_ASSERT_EQ("tag 0x11 correct", val_a, 0x9999888877776666ULL);
    TEST_ASSERT_EQ("tag 0x33 same value", val_b, 0x9999888877776666ULL);

    pm_set_smode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-SVA-07: PM disabled, tagged addr causes fault ----- */
TEST_REGISTER(test_zpm_sva_disabled_fault);
bool test_zpm_sva_disabled_fault(void) {
    TEST_BEGIN("ZPM-SVA-07: PM disabled, tagged addr page-fault (S-mode)");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");

    pm_set_smode(PMM_DISABLED);

    pt_context_t ctx;
    setup_smode_vm(&ctx);

    sva_test_data = 0x1234567890ABCDEFULL;
    uintptr_t base = (uintptr_t)&sva_test_data;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(7), 7);

    trap_expect_begin();
    vm_run_in_smode(&ctx, smode_load64, tagged);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        uintptr_t cause = trap_get_cause();
        TEST_ASSERT("page fault or access fault",
                     cause == CAUSE_LOAD_PAGE_FAULT ||
                     cause == CAUSE_LOAD_ACCESS_FAULT);
    }
    trap_expect_end();

    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-SVA-08: S-mode PM independent of U-mode PM ----- */
TEST_REGISTER(test_zpm_sva_independent_of_umode);
bool test_zpm_sva_independent_of_umode(void) {
    TEST_BEGIN("ZPM-SVA-08: S-mode PM independent of U-mode PM");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    if (!detect_ssnpm()) TEST_SKIP("Ssnpm not detected");

    /* S-mode PM on, U-mode PM off */
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported for S-mode");
    pm_set_umode(PMM_DISABLED);

    TEST_ASSERT_EQ("S-mode PMM=PMLEN7", pm_get_smode(), PMM_PMLEN7);
    TEST_ASSERT_EQ("U-mode PMM=DISABLED", pm_get_umode(), PMM_DISABLED);

    pt_context_t ctx;
    setup_smode_vm(&ctx);

    sva_test_data = 0xFEDCBA0987654321ULL;
    uintptr_t base = (uintptr_t)&sva_test_data;
    uintptr_t tagged = pm_tag_address(base, 0x55, 7);

    /* S-mode tagged load should succeed (PM is on for S-mode) */
    trap_expect_begin();
    uintptr_t result = vm_run_in_smode(&ctx, smode_load64, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("S-mode tagged load should not fault (PM not working?)", false);
        pm_set_smode(PMM_DISABLED);
        pm_set_umode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("S-mode tagged load succeeds",
                    result, 0xFEDCBA0987654321ULL);

    pm_set_smode(PMM_DISABLED);
    pm_set_umode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-SVA-09: sign-extend correctness (S-mode) ----- */
TEST_REGISTER(test_zpm_sva_sign_extend);
bool test_zpm_sva_sign_extend(void) {
    TEST_BEGIN("ZPM-SVA-09: VA sign-extend correctness (S-mode)");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");

    /* Software verification of sign-extend behavior */
    unsigned pmlen = 7;
    uintptr_t base_high = 0x0100000080001000ULL;  /* bit 56 = 1 */
    uintptr_t tagged = pm_tag_address(base_high, 0x33, pmlen);
    uintptr_t transformed = pm_transform_va(tagged, pmlen);

    /* bit(56) = 1 -> upper PMLEN bits should all be 1 */
    uintptr_t upper_mask = ((uintptr_t)pm_max_tag(pmlen)) << (64 - pmlen);
    TEST_ASSERT("upper bits all ones after sign-extend",
                (transformed & upper_mask) == upper_mask);

    /* Lower bits preserved */
    uintptr_t lower_mask = (1ULL << (64 - pmlen)) - 1;
    TEST_ASSERT_EQ("lower bits preserved",
                    transformed & lower_mask,
                    base_high & lower_mask);

    TEST_END();
}
