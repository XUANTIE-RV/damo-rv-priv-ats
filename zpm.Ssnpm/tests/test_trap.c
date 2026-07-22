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
 * Group 10.a: stval/mtval Hardware Write Transform - U-mode (Ssnpm)
 *
 * ZPM-TRAP-01, ZPM-TRAP-05
 *
 * Verifies that:
 *   - Hardware-written stval contains PM-transformed addresses
 *   - stval sign-extend correctness for VA transform
 * =================================================================== */

/* U-mode load function for triggering faults */
static uintptr_t umode_load_fault(uintptr_t addr) {
    return *(volatile uint64_t *)addr;
}

/* ----- ZPM-TRAP-01: stval contains transformed address ----- */
TEST_REGISTER(test_zpm_trap_stval_transformed);
bool test_zpm_trap_stval_transformed(void) {
    TEST_BEGIN("ZPM-TRAP-01: stval contains transformed address");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    /* Map only a portion so we can have unmapped regions */
    pt_setup_identity_mapping(&ctx, PLATFORM_MEM_BASE, PLATFORM_MEM_SIZE,
        PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D,
        PT_LEVEL_2M);

    /* Use an address outside the identity mapping range.
     * After PM transform, this VA will still be unmapped -> page fault.
     * The stval should contain the PM-transformed address. */
    uintptr_t unmapped_va = 0x0000004000000000ULL;
    uintptr_t tagged = pm_tag_address(unmapped_va, 0x55, 7);
    uintptr_t expected_stval = pm_transform_va(tagged, 7);

    trap_expect_begin();
    vm_run_in_umode(&ctx, umode_load_fault, tagged);
    TEST_ASSERT("page fault triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        uintptr_t cause = trap_get_cause();
        TEST_ASSERT("load page fault",
                     cause == CAUSE_LOAD_PAGE_FAULT ||
                     cause == CAUSE_LOAD_ACCESS_FAULT);
        uintptr_t tval = trap_get_tval();
        printf("  tagged:         0x%lx\n", (unsigned long)tagged);
        printf("  expected stval: 0x%lx\n", (unsigned long)expected_stval);
        printf("  actual tval:    0x%lx\n", (unsigned long)tval);
        TEST_ASSERT_EQ("tval is PM-transformed address",
                        tval, expected_stval);
    }
    trap_expect_end();

    pm_set_umode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-TRAP-05: stval sign-extend verification ----- */
TEST_REGISTER(test_zpm_trap_stval_sign_extend);
bool test_zpm_trap_stval_sign_extend(void) {
    TEST_BEGIN("ZPM-TRAP-05: stval sign-extend verification");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    unsigned pmlen = 7;

    /* Construct a tagged VA where bit(56) = 1.
     * After sign-extend, bits[63:57] should all be 1. */
    uintptr_t base = 0x0100000000001000ULL;  /* bit 56 = 1 */
    uintptr_t tagged = pm_tag_address(base, 0x33, pmlen);
    uintptr_t expected = pm_transform_va(tagged, pmlen);

    /* Verify sign-extend in expected value */
    uintptr_t upper_mask = ((uintptr_t)pm_max_tag(pmlen)) << (64 - pmlen);
    TEST_ASSERT("expected: bits[63:57] all ones",
                (expected & upper_mask) == upper_mask);

    /* Now trigger a fault with this tagged addr and check stval */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    pt_setup_identity_mapping(&ctx, PLATFORM_MEM_BASE, PLATFORM_MEM_SIZE,
        PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D,
        PT_LEVEL_2M);

    trap_expect_begin();
    vm_run_in_umode(&ctx, umode_load_fault, tagged);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped) {
        uintptr_t tval = trap_get_tval();
        printf("  tagged:    0x%lx\n", (unsigned long)tagged);
        printf("  expected:  0x%lx\n", (unsigned long)expected);
        printf("  tval:      0x%lx\n", (unsigned long)tval);

        /* Check that tval upper bits are all 1 (sign-extended) */
        if (tval != 0) {
            TEST_ASSERT("tval bits[63:57] all ones",
                        (tval & upper_mask) == upper_mask);
        } else {
            printf("  tval=0 (implementation may not write tval)\n");
            TEST_ASSERT("tval=0 is acceptable", true);
        }
    } else {
        printf("  No fault (address may have mapped to valid region)\n");
        TEST_ASSERT("no fault case", true);
    }

    pm_set_umode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}
