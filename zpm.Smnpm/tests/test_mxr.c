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
 * Group 9.a: PM and MXR Interaction - S-mode (Smnpm)
 *
 * ZPM-MXR-01, ZPM-MXR-02
 *
 * When MXR is in effect at the effective privilege mode, PM does
 * NOT apply. This group verifies this interaction for S-mode
 * scenarios.
 * =================================================================== */

static volatile uint64_t mxr_data __attribute__((aligned(8)))
    = 0xAA11BB22CC33DD44ULL;

/* S-mode load helper */
static uintptr_t smode_load64(uintptr_t addr) {
    return *(volatile uint64_t *)addr;
}

/* Helper: set up VM for S-mode */
static void setup_smode_vm(pt_context_t *ctx) {
    pt_pool_reset();
    pt_init(ctx, SATP_MODE_SV39);
    pt_setup_identity_mapping(ctx, PLATFORM_MEM_BASE, PLATFORM_MEM_SIZE,
        PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D,
        PT_LEVEL_2M);
}

/* ----- ZPM-MXR-01: MXR=1 disables PM ----- */
TEST_REGISTER(test_zpm_mxr_disables_pm);
bool test_zpm_mxr_disables_pm(void) {
    TEST_BEGIN("ZPM-MXR-01: MXR=1 disables PM");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_smode_vm(&ctx);

    mxr_data = 0xAA11BB22CC33DD44ULL;
    uintptr_t base = (uintptr_t)&mxr_data;
    uintptr_t tagged = pm_tag_address(base, 0x7F, 7);

    /* Enable MXR */
    uintptr_t mstatus = CSRR(mstatus);
    CSRW(mstatus, mstatus | MSTATUS_MXR_BIT);

    /* With MXR=1, PM should NOT apply.
     * Tagged load should use raw tagged address -> likely fault. */
    trap_expect_begin();
    vm_run_in_smode(&ctx, smode_load64, tagged);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* Restore mstatus */
    CSRW(mstatus, mstatus);

    TEST_ASSERT("MXR=1: tagged addr not transformed (fault expected)",
                 trapped);

    pm_set_smode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-MXR-02: MXR=0, PM normal ----- */
TEST_REGISTER(test_zpm_mxr_zero_pm_normal);
bool test_zpm_mxr_zero_pm_normal(void) {
    TEST_BEGIN("ZPM-MXR-02: MXR=0, PM normal");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    setup_smode_vm(&ctx);

    mxr_data = 0x5566778899AABBCCULL;
    uintptr_t base = (uintptr_t)&mxr_data;
    uintptr_t tagged = pm_tag_address(base, 0x55, 7);

    /* Ensure MXR=0 */
    uintptr_t mstatus = CSRR(mstatus);
    CSRW(mstatus, mstatus & ~MSTATUS_MXR_BIT);

    /* With MXR=0 and PM enabled, tagged load should succeed */
    trap_expect_begin();
    uintptr_t result = vm_run_in_smode(&ctx, smode_load64, tagged);
    if (trap_was_triggered()) {
        trap_expect_end();
        CSRW(mstatus, mstatus);
        TEST_ASSERT("MXR=0 tagged load should not fault (PM not working?)", false);
        pm_set_smode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();

    CSRW(mstatus, mstatus);

    TEST_ASSERT_EQ("MXR=0: PM transforms correctly",
                    result, 0x5566778899AABBCCULL);

    pm_set_smode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}
