/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4.2: hstatus.VTW gating (VS/VU-mode wrs.nto)
 *
 * HZWRS-04 ~ HZWRS-07
 *
 * Spec anchors:
 *   norm:Zawrs_virtual_instr_excp
 *     - VS/VU + hstatus.VTW=1 + mstatus.TW=0 + wrs.nto not completing
 *       within the bounded time limit -> virtual-instruction (22).
 *   norm:vtw_virtinstr (hypervisor.adoc)
 *     - implementations MAY always raise virtual-instruction for
 *       VTW=1, even with pending globally-disabled interrupts; hence
 *       HZWRS-06 is a record-only case.
 *   norm:H_virtinst_xtval
 *     - on a virtual-instruction trap, stval is written the same as
 *       for an illegal-instruction trap (instruction encoding or 0).
 *   hstatus.SPV
 *     - written to 1 on trap entry into HS-mode from V=1; verified
 *       via the delegated-to-HS delivery path (HZWRS-07).
 * =================================================================== */

/* ---- HZWRS-04: VS + VTW=1/TW=0 wrs.nto -> virtual-instruction ---- */

TEST_REGISTER(test_hzwrs_04);
bool test_hzwrs_04(void)
{
    TEST_BEGIN("HZWRS-04: VS-mode VTW=1 wrs.nto -> virtual-instruction");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    uintptr_t saved_mie = hz_quiet_interrupts();
    hz_clear_tw();
    hz_set_vtw();
    (void)hz_reserve();

    hz_vs_expect(_vs_wrs_nto, true, CAUSE_VIRTUAL_INSTRUCTION);

    hz_clear_vtw();
    hz_restore_interrupts(saved_mie);

    HYP_TEST_END();
}

/* ---- HZWRS-05: VU + VTW=1/TW=0 wrs.nto -> virtual-instruction ---- */

TEST_REGISTER(test_hzwrs_05);
bool test_hzwrs_05(void)
{
    TEST_BEGIN("HZWRS-05: VU-mode VTW=1 wrs.nto -> virtual-instruction");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    uintptr_t saved_mie = hz_quiet_interrupts();
    hz_clear_tw();
    hz_set_vtw();
    (void)hz_reserve();

    hz_vu_expect(_vu_wrs_nto, true, CAUSE_VIRTUAL_INSTRUCTION);

    hz_clear_vtw();
    hz_restore_interrupts(saved_mie);

    HYP_TEST_END();
}

/* ---- HZWRS-06: VTW=1 with pending locally enabled IRQ (record) ---- */

TEST_REGISTER(test_hzwrs_06);
bool test_hzwrs_06(void)
{
    TEST_BEGIN("HZWRS-06: VTW=1 + pending locally enabled IRQ (record)");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    /* Record-only case (norm:vtw_virtinstr): both completing
     * immediately (norm:Zawrs_exec_resume_rules) and trapping with
     * virtual-instruction (the VTW interception permission) are legal
     * implementations. Record the choice; constrain only the trap
     * type when a trap is taken. The wake source is a VS-level
     * software interrupt: hvip.VSSIP injected, routed to VS-level by
     * hideleg[1] and locally enabled by vsie.SSIE. */
    uintptr_t saved_mie = hz_quiet_interrupts();
    hz_clear_tw();
    hz_set_vtw();
    (void)hz_reserve();
    hz_suppress_globals();
    hz_set_vs_soft_pending();

    trap_expect_begin();
    (void)run_in_vs_mode(_vs_wrs_nto, 0);
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();

    hz_clear_vs_soft_pending();
    hz_clear_vtw();
    hz_restore_interrupts(saved_mie);

    if (trapped) {
        printf("[I] implementation takes the VTW interception: wrs.nto "
               "raised an exception despite the pending locally enabled "
               "VS-level IRQ\n");
        TEST_ASSERT_EQ("VTW interception trap type", cause,
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    } else {
        printf("[I] implementation completes wrs.nto immediately with "
               "the pending locally enabled IRQ (no VTW interception)\n");
        TEST_ASSERT("wrs.nto completed without exception", true);
    }

    HYP_TEST_END();
}

/* ---- HZWRS-07: VTW trap report (cause/stval/SPV) ---- */

TEST_REGISTER(test_hzwrs_07);
bool test_hzwrs_07(void)
{
    TEST_BEGIN("HZWRS-07: VTW trap report (cause=22, stval, SPV=1)");
    REQUIRE_H_EXT();
    REQUIRE_ZAWRS();

    /* Delegate virtual-instruction to HS-mode so hstatus.SPV is
     * written by hardware on trap entry (SPV is only written for
     * traps taken into HS-mode) and captured by the framework's
     * HS-mode handler (trap_get_spv). */
    CSRS(medeleg, (1UL << CAUSE_VIRTUAL_INSTRUCTION));

    uintptr_t saved_mie = hz_quiet_interrupts();
    hz_clear_tw();
    hz_set_vtw();
    (void)hz_reserve();

    hz_vs_expect(_vs_wrs_nto, true, CAUSE_VIRTUAL_INSTRUCTION);

    if (trap_was_triggered()) {
        /* norm:H_virtinst_xtval: stval written as for an
         * illegal-instruction trap - the trapping instruction
         * encoding, or zero if the implementation writes zero. */
        uintptr_t tv = trap_get_tval();
        TEST_ASSERT("stval is 0 or the wrs.nto encoding",
                    tv == 0 || tv == WRS_NTO_ENC);
        TEST_ASSERT("hstatus.SPV=1 (trap taken from V=1 into HS)",
                    trap_get_spv());
    }

    hz_clear_vtw();
    hz_restore_interrupts(saved_mie);
    CSRC(medeleg, (1UL << CAUSE_VIRTUAL_INSTRUCTION));

    HYP_TEST_END();
}
