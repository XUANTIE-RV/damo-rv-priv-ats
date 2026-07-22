/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"
#include "pm/pm_cfg.h"
#include "pm/pm_addr.h"
#include "pmp/pmp_cfg.h"
#include "zpm_cap_result.h"

/* ===================================================================
 * Group 8.b: PM and MPRV Interaction - U-mode PM (Ssnpm)
 *
 * ZPM-MPRV-03
 *
 * When MPRV=1 and MPP=U, load/store use the effective privilege
 * mode (U-mode) specified by MPP. The U-mode PM settings
 * (senvcfg.PMM) should apply.
 * =================================================================== */

static volatile uint64_t mprv_data __attribute__((aligned(8)))
    = 0xAAAABBBBCCCCDDDDULL;

/* ----- ZPM-MPRV-03: MPRV=1 MPP=U uses U-mode PM ----- */
TEST_REGISTER(test_zpm_mprv_umode_pm);
bool test_zpm_mprv_umode_pm(void) {
    TEST_BEGIN("ZPM-MPRV-03: MPRV=1 MPP=U uses U-mode PM");
    if (!zpm_cap_ssnpm_supported) TEST_SKIP("Ssnpm not detected in CAP");
    pm_set_umode(PMM_PMLEN7);
    if (pm_get_umode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    if (detect_smmpm()) pm_set_mmode(PMM_DISABLED);

    /* Configure PMP to allow U-mode access to the entire memory region.
     * MPRV=1 MPP=U causes load/store to use U-mode privilege for PMP checks. */
    pmp_entry_t pmp_allow_all = PMP_ENTRY_NAPOT(0, (uintptr_t)1 << 63, PMP_RWX);
    pmp_set_entry(0, &pmp_allow_all);

    mprv_data = 0x5555666677778888ULL;
    uintptr_t base = (uintptr_t)&mprv_data;
    uintptr_t tagged = pm_tag_address(base, 0x55, 7);

    uintptr_t mstatus_saved = CSRR(mstatus);
    uintptr_t new_mstatus = mstatus_saved;
    new_mstatus |= MSTATUS_MPRV_BIT;
    new_mstatus = (new_mstatus & ~MSTATUS_MPP_MASK)
                  | ((uintptr_t)PRIV_U << MSTATUS_MPP_OFF);

    trap_expect_begin();

    uint64_t val;
    asm volatile(
        "csrw mstatus, %[new_ms]\n\t"   /* MPRV=1 MPP=U */
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[val], 0(%[addr])\n\t"      /* load via tagged addr */
        ".option pop\n\t"
        "csrw mstatus, %[old_ms]\n\t"    /* restore: MPRV=0 */
        : [val] "=r"(val)
        : [new_ms] "r"(new_mstatus),
          [old_ms] "r"(mstatus_saved),
          [addr] "r"(tagged)
        : "memory"
    );

    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped) {
        TEST_ASSERT("MPRV tagged load should not fault (PM not working?)", false);
        pmp_clear_entry(0);
        pm_set_umode(PMM_DISABLED);
        TEST_END();
    }
    TEST_ASSERT_EQ("MPRV load via U-mode PM correct",
                    val, 0x5555666677778888ULL);

    pmp_clear_entry(0);
    pm_set_umode(PMM_DISABLED);
    TEST_END();
}
