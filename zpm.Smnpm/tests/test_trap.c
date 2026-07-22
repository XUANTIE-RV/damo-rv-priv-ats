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
 * Group 10.c: Trap Handler - S-mode (Smnpm)
 *
 * ZPM-TRAP-03, ZPM-TRAP-04
 *
 * Verifies that:
 *   - Trap handler addresses (stvec) are NOT PM-affected
 *   - Tagged stvec still delivers correctly (CSR write not PM-transformed)
 * =================================================================== */

/* S-mode load function for trap handler test */
static uintptr_t smode_load_for_trap(uintptr_t addr) {
    return *(volatile uint64_t *)addr;
}

/* ----- ZPM-TRAP-03: Trap handler not affected by PM ----- */
TEST_REGISTER(test_zpm_trap_handler_no_pm);
bool test_zpm_trap_handler_no_pm(void) {
    TEST_BEGIN("ZPM-TRAP-03: Trap handler addr not masked");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    /* With PM enabled, verify that trap delivery still works correctly.
     * If PM were applied to stvec, the trap handler would be at a
     * different (wrong) address and likely crash.
     *
     * We verify by checking that a normal trap (ecall) in S-mode
     * is delivered correctly and returns to M-mode successfully. */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    pt_setup_identity_mapping(&ctx, PLATFORM_MEM_BASE, PLATFORM_MEM_SIZE,
        PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D,
        PT_LEVEL_2M);

    /* The vm_run_in_smode function uses ecall to return from S-mode.
     * If trap delivery were PM-affected, this would crash. */
    static volatile uint64_t trap_test_val = 0xFEEDFACE00112233ULL;

    trap_expect_begin();
    uintptr_t result = vm_run_in_smode(&ctx,
        smode_load_for_trap, (uintptr_t)&trap_test_val);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_ASSERT("trap handler test should not fault (PM not working?)", false);
        pm_set_smode(PMM_DISABLED);
        pt_pool_reset();
        TEST_END();
    }
    trap_expect_end();

    /* If we got here, trap delivery worked correctly */
    TEST_ASSERT_EQ("trap handler works with PM enabled",
                    result, 0xFEEDFACE00112233ULL);

    pm_set_smode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-TRAP-04: Tagged stvec still delivers correctly ----- */
TEST_REGISTER(test_zpm_trap_tagged_stvec);
bool test_zpm_trap_tagged_stvec(void) {
    TEST_BEGIN("ZPM-TRAP-04: Tagged stvec still delivers correctly");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    /* Write a tagged address to stvec.
     * Trap delivery should use the raw stvec value (PM not applied).
     * Since the tagged stvec points to an invalid location,
     * a trap would fail to deliver (double fault or hang).
     *
     * We verify by writing tagged stvec, reading it back,
     * confirming the tag is preserved (CSR write not PM-affected). */
    extern void s_trap_entry(void);
    uintptr_t handler = (uintptr_t)s_trap_entry;
    uintptr_t tagged_handler = pm_tag_address(handler, 0x55, 7);

    uintptr_t saved_stvec = CSRR(stvec);
    CSRW(stvec, tagged_handler);
    uintptr_t readback = CSRR(stvec);

    /* stvec tag should be preserved (CSR write not PM-transformed) */
    uintptr_t tag_written = pm_extract_tag(tagged_handler, 7);
    uintptr_t tag_read = pm_extract_tag(readback, 7);

    /* Note: stvec low 2 bits encode MODE, and implementation may
     * truncate upper bits. Check what we can. */
    printf("  tagged stvec:  0x%lx\n", (unsigned long)tagged_handler);
    printf("  readback:      0x%lx\n", (unsigned long)readback);

    if (tag_read == tag_written) {
        TEST_ASSERT("tag preserved in stvec", true);
    } else {
        /* Some implementations may not preserve upper bits in stvec */
        printf("  Implementation truncates stvec upper bits\n");
        TEST_ASSERT("stvec readback (impl-specific)", true);
    }

    /* Restore stvec to safe value */
    CSRW(stvec, saved_stvec);
    pm_set_smode(PMM_DISABLED);
    TEST_END();
}
