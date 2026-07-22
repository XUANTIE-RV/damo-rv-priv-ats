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
 * Group 8.a: PM and MPRV Interaction - S-mode PM (Smnpm)
 *
 * ZPM-MPRV-01, ZPM-MPRV-02
 *
 * When MPRV=1 and MPP=S, load/store use the effective privilege
 * mode (S-mode) specified by MPP. The S-mode PM settings
 * (menvcfg.PMM) should apply, not M-mode's own PM settings.
 * =================================================================== */

static volatile uint64_t mprv_data __attribute__((aligned(8)))
    = 0xAAAABBBBCCCCDDDDULL;

/* ----- ZPM-MPRV-01: MPRV=1 MPP=S uses S-mode PM ----- */
TEST_REGISTER(test_zpm_mprv_smode_pm);
bool test_zpm_mprv_smode_pm(void) {
    TEST_BEGIN("ZPM-MPRV-01: MPRV=1 MPP=S uses S-mode PM");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");
    pm_set_smode(PMM_PMLEN7);
    if (pm_get_smode() != PMM_PMLEN7) TEST_SKIP("PMLEN=7 not supported");

    /* Disable M-mode PM to isolate MPRV effect */
    if (detect_smmpm()) pm_set_mmode(PMM_DISABLED);

    /* Configure PMP to allow S-mode access to the entire memory region.
     * MPRV=1 MPP=S causes load/store to use S-mode privilege for PMP checks. */
    pmp_entry_t pmp_allow_all = PMP_ENTRY_NAPOT(0, (uintptr_t)1 << 63, PMP_RWX);
    pmp_set_entry(0, &pmp_allow_all);

    mprv_data = 0xAAAABBBBCCCCDDDDULL;
    uintptr_t base = (uintptr_t)&mprv_data;
    uintptr_t tagged = pm_tag_address(base, 0x7F, 7);

    /* Save mstatus, set MPRV=1 MPP=S.
     *
     * CRITICAL: We must do the entire MPRV=1 -> load -> MPRV=0 sequence
     * in inline asm so the compiler does not insert any stack load/store
     * between setting MPRV and clearing it. With MPRV=1 MPP=S, all
     * M-mode load/store use S-mode privilege, which may fault via PMP. */
    uintptr_t mstatus_saved = CSRR(mstatus);
    uintptr_t new_mstatus = mstatus_saved;
    new_mstatus |= MSTATUS_MPRV_BIT;
    new_mstatus = (new_mstatus & ~MSTATUS_MPP_MASK)
                  | ((uintptr_t)PRIV_S << MSTATUS_MPP_OFF);

    trap_expect_begin();

    uint64_t val;
    asm volatile(
        "csrw mstatus, %[new_ms]\n\t"   /* MPRV=1 MPP=S */
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
        pm_set_smode(PMM_DISABLED);
        TEST_END();
    }
    TEST_ASSERT_EQ("MPRV load via S-mode PM correct",
                    val, 0xAAAABBBBCCCCDDDDULL);

    pmp_clear_entry(0);
    pm_set_smode(PMM_DISABLED);
    TEST_END();
}

/* ----- ZPM-MPRV-02: MPRV=1 MPP=S but S-mode PM disabled ----- */
TEST_REGISTER(test_zpm_mprv_smode_pm_disabled);
bool test_zpm_mprv_smode_pm_disabled(void) {
    TEST_BEGIN("ZPM-MPRV-02: MPRV=1 MPP=S, S-mode PM disabled");
    if (!zpm_cap_smnpm_supported) TEST_SKIP("Smnpm not detected in CAP");

    /* S-mode PM off */
    pm_set_smode(PMM_DISABLED);
    if (detect_smmpm()) pm_set_mmode(PMM_DISABLED);

    mprv_data = 0x1111222233334444ULL;
    uintptr_t base = (uintptr_t)&mprv_data;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(7), 7);

    uintptr_t mstatus_saved = CSRR(mstatus);
    uintptr_t new_mstatus = mstatus_saved;
    new_mstatus |= MSTATUS_MPRV_BIT;
    new_mstatus = (new_mstatus & ~MSTATUS_MPP_MASK)
                  | ((uintptr_t)PRIV_S << MSTATUS_MPP_OFF);

    /* With S-mode PM disabled, tagged addr is not transformed.
     * In M-mode Bare, this may access wrong location or fault.
     * Use inline asm to bracket the MPRV=1 window tightly. */
    trap_expect_begin();

    uint64_t val;
    asm volatile(
        "csrw mstatus, %[new_ms]\n\t"
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[val], 0(%[addr])\n\t"
        ".option pop\n\t"
        "csrw mstatus, %[old_ms]\n\t"
        : [val] "=r"(val)
        : [new_ms] "r"(new_mstatus),
          [old_ms] "r"(mstatus_saved),
          [addr] "r"(tagged)
        : "memory"
    );

    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped) {
        TEST_ASSERT("fault: tagged addr not transformed", true);
    } else {
        TEST_ASSERT("raw tagged access (PM disabled)",
                     val != 0x1111222233334444ULL || tagged == base);
    }

    TEST_END();
}
