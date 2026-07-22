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
 * Group 4: Ssnpm — U-mode VA Ignore Transform (AMO)
 *
 * ZPM-UAMO-01 ~ ZPM-UAMO-04
 *
 * Verifies that AMO instructions (amoadd.d, amoswap.d) correctly
 * use PM-transformed addresses in U-mode under Sv39.
 * =================================================================== */

/* Shared test data (must be naturally aligned for AMO) */
static volatile uint64_t amo_var __attribute__((aligned(8))) = 0;

/* U-mode AMO helpers */
static uintptr_t umode_amoadd_42(uintptr_t addr) {
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

static uintptr_t umode_amoswap(uintptr_t addr) {
    uint64_t old_val;
    uint64_t new_val = 0xAAAABBBBCCCCDDDDULL;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoswap.d %0, %1, (%2)\n\t"
        ".option pop\n\t"
        : "=r"(old_val) : "r"(new_val), "r"(addr) : "memory"
    );
    return old_val;
}

/* Helper: set up VM context for U-mode */
static void setup_umode_vm(pt_context_t *ctx) {
    pt_pool_reset();
    pt_init(ctx, SATP_MODE_SV39);
    pt_setup_identity_mapping(ctx, PLATFORM_MEM_BASE, PLATFORM_MEM_SIZE,
        PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D,
        PT_LEVEL_2M);
}

/* ----- ZPM-UAMO-01: PMLEN7 amoadd.d ----- */
TEST_REGISTER(test_zpm_uamo_pmlen7_amoadd);
bool test_zpm_uamo_pmlen7_amoadd(void) {
    TEST_BEGIN("ZPM-UAMO-01: PMLEN7 amoadd.d via tagged addr");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    amo_var = 100;
    uintptr_t tagged = pm_tag_address((uintptr_t)&amo_var, 0x55, 7);

    trap_expect_begin();
    uintptr_t old_val = vm_run_in_umode(&ctx, umode_amoadd_42, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged amoadd should not fault (PM not working?)", false);
        pm_set_umode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();

    TEST_ASSERT_EQ("old value is 100", old_val, 100);
    TEST_ASSERT_EQ("updated to 142", amo_var, 142);

    pm_set_umode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-UAMO-02: PMLEN16 amoadd.d ----- */
TEST_REGISTER(test_zpm_uamo_pmlen16_amoadd);
bool test_zpm_uamo_pmlen16_amoadd(void) {
    TEST_BEGIN("ZPM-UAMO-02: PMLEN16 amoadd.d via tagged addr");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN16);
    if (pm_get_umode() != PMM_PMLEN16) TEST_SKIP("PMLEN=16 not supported");

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    amo_var = 200;
    uintptr_t tagged = pm_tag_address((uintptr_t)&amo_var, 0xABCD, 16);

    trap_expect_begin();
    uintptr_t old_val = vm_run_in_umode(&ctx, umode_amoadd_42, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged amoadd should not fault (PM not working?)", false);
        pm_set_umode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();

    TEST_ASSERT_EQ("old value is 200", old_val, 200);
    TEST_ASSERT_EQ("updated to 242", amo_var, 242);

    pm_set_umode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-UAMO-03: PMLEN7 amoswap.d ----- */
TEST_REGISTER(test_zpm_uamo_pmlen7_amoswap);
bool test_zpm_uamo_pmlen7_amoswap(void) {
    TEST_BEGIN("ZPM-UAMO-03: PMLEN7 amoswap.d via tagged addr");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    amo_var = 0x1111222233334444ULL;
    uintptr_t tagged = pm_tag_address((uintptr_t)&amo_var, 0x7F, 7);

    trap_expect_begin();
    uintptr_t old_val = vm_run_in_umode(&ctx, umode_amoswap, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged amoswap should not fault (PM not working?)", false);
        pm_set_umode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();

    TEST_ASSERT_EQ("old value returned correctly",
                    old_val, 0x1111222233334444ULL);
    TEST_ASSERT_EQ("new value written correctly",
                    amo_var, 0xAAAABBBBCCCCDDDDULL);

    pm_set_umode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-UAMO-04: Different tags AMO same location ----- */
TEST_REGISTER(test_zpm_uamo_different_tags);
bool test_zpm_uamo_different_tags(void) {
    TEST_BEGIN("ZPM-UAMO-04: Different tags AMO same location");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_umode_vm(&ctx);

    amo_var = 0;
    uintptr_t base = (uintptr_t)&amo_var;
    uintptr_t tagged_a = pm_tag_address(base, 0x11, 7);
    uintptr_t tagged_b = pm_tag_address(base, 0x22, 7);

    /* First amoadd with tag 0x11: 0 + 42 = 42 */
    trap_expect_begin();
    uintptr_t old_a = vm_run_in_umode(&ctx, umode_amoadd_42, tagged_a);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged amoadd (tag_a) should not fault", false);
        pm_set_umode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("first amoadd old=0", old_a, 0);

    /* Second amoadd with tag 0x22: 42 + 42 = 84 */
    trap_expect_begin();
    uintptr_t old_b = vm_run_in_umode(&ctx, umode_amoadd_42, tagged_b);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("tagged amoadd (tag_b) should not fault", false);
        pm_set_umode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();
    TEST_ASSERT_EQ("second amoadd old=42", old_b, 42);

    TEST_ASSERT_EQ("both accumulated to 84", amo_var, 84);

    pm_set_umode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}
