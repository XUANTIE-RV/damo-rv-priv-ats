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
 * Group 8.c: PM and MPRV Interaction - M-mode PM (Smmpm)
 *
 * ZPM-MPRV-04, ZPM-MPRV-05
 *
 * When MPRV=0, load/store use M-mode's own PM settings (mseccfg.PMM).
 * When MPRV is toggled, PM behavior changes with the effective
 * privilege mode.
 * =================================================================== */

static volatile uint64_t mprv_data __attribute__((aligned(8)))
    = 0xAAAABBBBCCCCDDDDULL;

/* ----- ZPM-MPRV-04: MPRV=0 uses M-mode PM (not MPP) ----- */
TEST_REGISTER(test_zpm_mprv_zero_uses_mmode);
bool test_zpm_mprv_zero_uses_mmode(void) {
    TEST_BEGIN("ZPM-MPRV-04: MPRV=0 uses M-mode PM");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    /* Set S-mode PM differently to ensure it's not used */
    if (detect_smnpm()) pm_set_smode(PMM_DISABLED);

    mprv_data = 0xDDDDEEEEFFFF0000ULL;
    uintptr_t base = (uintptr_t)&mprv_data;
    uintptr_t tagged = pm_tag_address(base, 0x33, 7);

    /* Ensure MPRV=0 */
    uintptr_t mstatus = CSRR(mstatus);
    CSRW(mstatus, mstatus & ~MSTATUS_MPRV_BIT);

    /* M-mode load with MPRV=0 should use M-mode PM (PA zero-extend) */
    trap_expect_begin();
    uint64_t val = *(volatile uint64_t *)tagged;
    bool trapped = trap_was_triggered();
    trap_expect_end();

    CSRW(mstatus, mstatus);

    if (trapped) {
        TEST_ASSERT("MPRV=0 tagged load should not fault (PM not working?)", false);
        pm_set_mmode(PMM_DISABLED);
        TEST_END();
    }
    TEST_ASSERT_EQ("MPRV=0 uses M-mode PM",
                    val, 0xDDDDEEEEFFFF0000ULL);

    pm_set_mmode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-MPRV-05: PM behavior changes with MPRV toggle ----- */
TEST_REGISTER(test_zpm_mprv_toggle);
bool test_zpm_mprv_toggle(void) {
    TEST_BEGIN("ZPM-MPRV-05: PM behavior changes with MPRV toggle");
    if (!zpm_cap_smmpm_supported) TEST_SKIP("Smmpm not detected in CAP");
    if (!detect_smnpm()) TEST_SKIP("Smnpm not detected");

    /* Set M-mode PM=PMLEN7, S-mode PM=DISABLED */
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported for M-mode");
    pm_set_smode(PMM_DISABLED);

    /* Configure PMP to allow S-mode access to the entire memory region.
     * MPRV=1 MPP=S causes load/store to use S-mode privilege for PMP
     * checks. Without this, compiler-generated stack accesses between
     * CSRW mstatus and the test load will fault via PMP. */
    pmp_entry_t pmp_allow_all = PMP_ENTRY_NAPOT(0, (uintptr_t)1 << 63, PMP_RWX);
    pmp_set_entry(0, &pmp_allow_all);

    mprv_data = 0xABCDABCDABCDABCDULL;
    uintptr_t base = (uintptr_t)&mprv_data;
    uintptr_t tagged = pm_tag_address(base, 0x7F, 7);

    /* MPRV=0: M-mode PM active -> tagged load should work */
    uintptr_t mstatus_saved = CSRR(mstatus);
    CSRW(mstatus, mstatus_saved & ~MSTATUS_MPRV_BIT);
    uint64_t val_mprv0 = *(volatile uint64_t *)tagged;

    /* MPRV=1, MPP=S: S-mode PM (DISABLED) -> tagged load may fault.
     *
     * CRITICAL: Use inline asm to bracket the MPRV=1 window tightly.
     * With MPRV=1 MPP=S, all M-mode load/store use S-mode privilege.
     * The compiler must not insert any stack load/store between
     * setting MPRV and clearing it. */
    uintptr_t new_ms = mstatus_saved;
    new_ms |= MSTATUS_MPRV_BIT;
    new_ms = (new_ms & ~MSTATUS_MPP_MASK)
             | ((uintptr_t)PRIV_S << MSTATUS_MPP_OFF);

    trap_expect_begin();

    uint64_t val_mprv1;
    asm volatile(
        "csrw mstatus, %[new_ms]\n\t"   /* MPRV=1 MPP=S */
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[val], 0(%[addr])\n\t"      /* load via tagged addr */
        ".option pop\n\t"
        "csrw mstatus, %[old_ms]\n\t"    /* restore: MPRV=0 */
        : [val] "=r"(val_mprv1)
        : [new_ms] "r"(new_ms),
          [old_ms] "r"(mstatus_saved),
          [addr] "r"(tagged)
        : "memory"
    );

    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT_EQ("MPRV=0: M-mode PM works",
                    val_mprv0, 0xABCDABCDABCDABCDULL);
    TEST_ASSERT("MPRV=1 MPP=S: different PM behavior (fault or wrong val)",
                 trapped || val_mprv1 != 0xABCDABCDABCDABCDULL);

    pmp_clear_entry(0);
    pm_set_mmode(PMM_DISABLED);
    pm_set_smode(PMM_DISABLED);
    TEST_END();
}
