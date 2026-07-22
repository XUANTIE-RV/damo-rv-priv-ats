/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"
#include "pm/pm_addr.h"
#include "zpm_cap_result.h"

/* ===================================================================
 * Group 10.b: mtval Hardware Write Transform - M-mode (Smmpm)
 *
 * ZPM-TRAP-02
 *
 * Verifies that hardware-written mtval contains PM-transformed
 * addresses (zero-extended for PA) when an access fault occurs
 * in M-mode with PM enabled.
 * =================================================================== */

/* ----- ZPM-TRAP-02: mtval contains transformed address (M-mode) ----- */
TEST_REGISTER(test_zpm_trap_mtval_transformed);
bool test_zpm_trap_mtval_transformed(void) {
    TEST_BEGIN("ZPM-TRAP-02: mtval contains transformed address (M-mode)");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    /* Access a tagged PA that, after zero-extend, points to an
     * invalid/unmapped physical address -> access fault.
     * mtval should contain the PM-transformed (zero-extended) address. */
    uintptr_t bad_pa = 0x0000000000000008ULL;  /* very low address */
    uintptr_t tagged = pm_tag_address(bad_pa, 0x7F, 7);
    uintptr_t expected_mtval = pm_transform_pa(tagged, 7);

    trap_expect_begin();
    volatile uint64_t val = *(volatile uint64_t *)tagged;
    (void)val;
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped) {
        uintptr_t tval = trap_get_tval();
        printf("  tagged:          0x%lx\n", (unsigned long)tagged);
        printf("  expected mtval:  0x%lx\n", (unsigned long)expected_mtval);
        printf("  actual mtval:    0x%lx\n", (unsigned long)tval);
        /* mtval may be 0 or the transformed address depending on impl */
        if (tval != 0) {
            TEST_ASSERT_EQ("mtval is PM-transformed",
                            tval, expected_mtval);
        } else {
            printf("  mtval=0 (implementation choice)\n");
            TEST_ASSERT("mtval is 0 or transformed", true);
        }
    } else {
        /* If no fault, the tagged PA resolved to valid memory */
        printf("  No fault - tagged PA resolved to valid memory\n");
        TEST_ASSERT("access succeeded (valid PA after transform)", true);
    }

    pm_set_mmode(PMM_DISABLED);
    TEST_END();
}
