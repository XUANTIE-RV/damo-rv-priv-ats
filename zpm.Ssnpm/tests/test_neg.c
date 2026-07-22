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
 * Group 7.a: PM Non-Application Scenarios - U-mode (Ssnpm)
 *
 * ZPM-NEG-01, ZPM-NEG-02
 *
 * Verifies that PM does NOT apply to:
 *   - Instruction fetch
 *   - Page-table walk (implicit access)
 * =================================================================== */

/* Shared U-mode load helper for page-table walk test */
static uintptr_t umode_ptwalk_load(uintptr_t addr) {
    return *(volatile uint64_t *)addr;
}

/* ----- ZPM-NEG-01: Instruction fetch not affected by PM ----- */

/* U-mode function that jumps to a tagged PC.
 * Since fetch is not PM-transformed, tagged PC -> invalid -> fault.
 *
 * We set _exec_return_addr before the jump so that the trap handler
 * can redirect mepc to the recovery label instead of trying to read
 * the (invalid/tagged) faulting epc with next_instruction(). */
extern uintptr_t _exec_return_addr;

static uintptr_t umode_jump_tagged(uintptr_t tagged_pc) {
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "la t0, 1f\n\t"
        "la t1, _exec_return_addr\n\t"
        STORE " t0, 0(t1)\n\t"
        "jr %0\n\t"
        "1:\n\t"
        STORE " zero, 0(t1)\n\t"
        ".option pop\n\t"
        :: "r"(tagged_pc) : "t0", "t1", "memory"
    );
    return 0;  /* reached only if trap handler recovers here */
}

TEST_REGISTER(test_zpm_neg_fetch_no_pm);
bool test_zpm_neg_fetch_no_pm(void) {
    TEST_BEGIN("ZPM-NEG-01: Instruction fetch not affected by PM");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    pt_setup_identity_mapping(&ctx, PLATFORM_MEM_BASE, PLATFORM_MEM_SIZE,
        PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D,
        PT_LEVEL_2M);

    /* Tag a known valid code address.
     * If PM applied to fetch, tagged addr would resolve to valid code.
     * Since PM does NOT apply to fetch, tagged PC is invalid -> fault. */
    uintptr_t valid_pc = (uintptr_t)umode_jump_tagged;
    uintptr_t tagged_pc = pm_tag_address(valid_pc, 0x7F, 7);

    trap_expect_begin();
    vm_run_in_umode(&ctx, umode_jump_tagged, tagged_pc);
    TEST_ASSERT("fault on tagged fetch", trap_was_triggered());
    if (trap_was_triggered()) {
        uintptr_t cause = trap_get_cause();
        TEST_ASSERT("instruction fault",
                     cause == CAUSE_INST_PAGE_FAULT ||
                     cause == CAUSE_INST_ACCESS_FAULT);
    }
    trap_expect_end();

    pm_set_umode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}

/* ----- ZPM-NEG-02: Page-table walk not affected by PM ----- */
TEST_REGISTER(test_zpm_neg_ptwalk_no_pm);
bool test_zpm_neg_ptwalk_no_pm(void) {
    TEST_BEGIN("ZPM-NEG-02: Page-table walk not affected by PM");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");

    /* This test verifies PM does not affect implicit accesses
     * by checking that normal VM operations work correctly
     * even when PM is enabled with non-zero PMLEN. */
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    pt_setup_identity_mapping(&ctx, PLATFORM_MEM_BASE, PLATFORM_MEM_SIZE,
        PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D,
        PT_LEVEL_2M);

    /* If PM were applied to page-table walks, the PPN addresses
     * in PTEs would be corrupted and translation would fail.
     * A successful untagged load proves page walks are unaffected. */
    static volatile uint64_t ptwalk_data = 0x1122334455667788ULL;

    uintptr_t base = (uintptr_t)&ptwalk_data;
    /* Use untagged address - if PT walk were PM-affected, even this
     * might fail because PTEs contain physical addresses.
     * Reuse the umode_load helper from NEG-01's test infrastructure. */
    uintptr_t result = vm_run_in_umode(&ctx, umode_ptwalk_load, base);

    /* The fact that vm_run_in_umode succeeds at all proves
     * page-table walks are not PM-affected */
    TEST_ASSERT_EQ("page-table walk unaffected by PM",
                    result, 0x1122334455667788ULL);

    pm_set_umode(PMM_DISABLED);
    pt_pool_reset();
    TEST_END();
}
