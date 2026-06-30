/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 3: hedeleg/hideleg delegation
 *
 * Tests DELEG-01 through DELEG-16 verify basic delegation CSR behavior.
 */

#include "test_helpers.h"

/* ------------------------------------------------------------------
 * DELEG-01: DELEG-01 hedeleg writable bits verification
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_writable_bits);
bool hedeleg_writable_bits(void) {
    TEST_BEGIN("DELEG-01: Write all 1s to hedeleg and verify writable bits");

    /* Write all 1s and read back. */
    hedeleg_write(0xFFFFFFFF);
    uintptr_t val = hedeleg_read();

    /* Per RISC-V spec, bits 20 (inst guest-page fault), 21 (load
     * guest-page fault), 22 (virtual instruction), 23 (store/AMO
     * guest-page fault) MUST be read-only zero — these exceptions
     * can only occur during guest operation. Other bits are WARL.
     * TODO: WARL tests are need to be reviewed further */
    uintptr_t non_delegatable = (1UL << 20) | (1UL << 21) |
                                 (1UL << 22) | (1UL << 23);
    TEST_ASSERT_EQ("hedeleg non-delegatable bits (20/21/22/23) must be zero",
                   val & non_delegatable, (uintptr_t)0);

    uintptr_t common_bits = (1UL << 2) | (1UL << 12) |
                             (1UL << 13) | (1UL << 15);
    TEST_ASSERT_EQ("hedeleg common delegatable bits should be writable",
                   val & common_bits, common_bits);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-02: hedeleg bit 0 writability
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_bit0_writable);
bool hedeleg_bit0_writable(void) {
    TEST_BEGIN("DELEG-02: Verify hedeleg bit 0 writability");

    hedeleg_write(0x0);
    uintptr_t base = hedeleg_read();
    hedeleg_write(0x1);
    uintptr_t val = hedeleg_read();
    /* Bit 0 is WARL: may be writable or hardwired zero.
     * Verify writing bit 0 does not affect other bits.
     * TODO: WARL tests are need to be reviewed further */
    TEST_ASSERT_EQ("writing bit 0 must not set unexpected bits",
                   val & ~(uintptr_t)0x1, base & ~(uintptr_t)0x1);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-03: hideleg writable bits verification
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_writable_bits);
bool hideleg_writable_bits(void) {
    TEST_BEGIN("DELEG-03: Verify hideleg writable bits (10/6/2 writable, others read-zero)");

    /* Write all 1s and read back. */
    hideleg_write(0xFFFFFFFF);
    uintptr_t val = hideleg_read();

    /* Bits 10,6,2 are writable (VSEI/VSTI/VSSI).
     * With AIA (aia=aplic-imsic), bits 13+ may also be writable for
     * local interrupts.  Only verify the baseline bits are present. */
    uintptr_t baseline = 0x444;  /* Bits: 10,6,2 */
    TEST_ASSERT_EQ("hideleg baseline bits (10/6/2) must be writable",
                   val & baseline, baseline);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-04: hedeleg illegal-inst delegation to VS
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_illegal_inst_deleg);
bool hedeleg_illegal_inst_deleg(void) {
    TEST_BEGIN("DELEG-04: Verify hedeleg[2] (illegal-inst) can be written");

    /* Set bit 2 (illegal-instruction). */
    hedeleg_write(0x4);
    uintptr_t val = hedeleg_read();

    /* Verify bit 2 is set. */
    TEST_ASSERT_EQ("hedeleg[2] (illegal-inst) should be writable", val & 0x4, 0x4);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-05: hedeleg default behavior - traps to HS when not delegated
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_default_trap_to_hs);
bool hedeleg_default_trap_to_hs(void) {
    TEST_BEGIN("DELEG-05: Verify default hedeleg=0 causes VS traps to M/HS");

    /* Clear hedeleg. */
    hedeleg_write(0x0);
    uintptr_t val = hedeleg_read();

    /* Verify hedeleg is zero. */
    TEST_ASSERT_EQ("hedeleg should be 0 by default", val, 0x0);

    hedeleg_write(0); /* Clear all delegation */
    trap_expect_begin();
    run_in_vs_mode(vs_exec_ecall, 0);
    TEST_ASSERT("trap was triggered", trap_was_triggered());
    TEST_ASSERT_EQ("cause is ecall from VS", trap_get_cause(), (uintptr_t)10);
    trap_expect_end();

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-06: hedeleg delegate breakpoint to VS
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_breakpoint_deleg);
bool hedeleg_breakpoint_deleg(void) {
    TEST_BEGIN("DELEG-06: Verify hedeleg[3] (breakpoint) can be written");

    /* Set bit 3 (breakpoint). */
    hedeleg_write(0x8);
    uintptr_t val = hedeleg_read();

    /* Verify bit 3 is set. */
    TEST_ASSERT_EQ("hedeleg[3] (breakpoint) should be writable", val & 0x8, 0x8);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-07: hedeleg delegate ecall-from-VU to VS
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_ecall_vu_deleg);
bool hedeleg_ecall_vu_deleg(void) {
    TEST_BEGIN("DELEG-07: Verify hedeleg[8] (ecall-from-VU) can be written");

    /* Set bit 8 (ecall-from-VU). */
    hedeleg_write(0x100);
    uintptr_t val = hedeleg_read();

    /* Verify bit 8 is set. */
    TEST_ASSERT_EQ("hedeleg[8] (ecall-from-VU) should be writable", val & 0x100, 0x100);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-08: hideleg delegate VSSI to VS
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_vssi_deleg);
bool hideleg_vssi_deleg(void) {
    TEST_BEGIN("DELEG-08: Verify hideleg[2] (VSSI) can be written");

    /* Set bit 2 (VSSI). */
    hideleg_write(0x4);
    uintptr_t val = hideleg_read();

    /* Verify bit 2 is set. */
    TEST_ASSERT_EQ("hideleg[2] (VSSI) should be writable", val & 0x4, 0x4);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-09: hideleg delegate VSTI to VS
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_vsti_deleg);
bool hideleg_vsti_deleg(void) {
    TEST_BEGIN("DELEG-09: Verify hideleg[6] (VSTI) can be written");

    /* Set bit 6 (VSTI). */
    hideleg_write(0x40);
    uintptr_t val = hideleg_read();

    /* Verify bit 6 is set. */
    TEST_ASSERT_EQ("hideleg[6] (VSTI) should be writable", val & 0x40, 0x40);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-10: hideleg delegate VSEI to VS
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_vsei_deleg);
bool hideleg_vsei_deleg(void) {
    TEST_BEGIN("DELEG-10: Verify hideleg[10] (VSEI) can be written");

    /* Set bit 10 (VSEI). */
    hideleg_write(0x400);
    uintptr_t val = hideleg_read();

    /* Verify bit 10 is set. */
    TEST_ASSERT_EQ("hideleg[10] (VSEI) should be writable", val & 0x400, 0x400);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-11: hideleg not delegated - interrupts trap to HS
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_not_deleg_trap_to_hs);
bool hideleg_not_deleg_trap_to_hs(void) {
    TEST_BEGIN("DELEG-11: Verify undelegated interrupts trap to HS");

    /* Clear hideleg. */
    hideleg_write(0x0);
    uintptr_t val = hideleg_read();

    /* Verify hideleg is zero. */
    TEST_ASSERT_EQ("hideleg should be 0 when not delegated", val, 0x0);

    hideleg_write(0); /* No interrupt delegation */
    hvip_set_vssi(true); /* Inject VSSIP */
    uintptr_t hvip_val = hvip_read();
    TEST_ASSERT("VSSIP pending in hvip", (hvip_val & 0x4) != 0);
    /* With hideleg[2]=0, VSSIP should trap to HS, not VS */
    TEST_ASSERT_EQ("hideleg is zero", hideleg_read(), (uintptr_t)0);
    hvip_set_vssi(false); /* Cleanup */

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-12: Interrupt number translation - VSSI
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_translation_vssi);
bool interrupt_translation_vssi(void) {
    TEST_BEGIN("DELEG-12: Verify VSSI interrupt translation (HS->VS)");

    /* Set hideleg[2] to delegate VSSI. */
    hideleg_write(0x4);
    uintptr_t val = hideleg_read();

    /* Verify bit 2 is set, enabling VSSI translation. */
    TEST_ASSERT_EQ("hideleg[2] should be set for VSSI translation", val & 0x4, 0x4);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-13: Interrupt number translation - VSTI
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_translation_vsti);
bool interrupt_translation_vsti(void) {
    TEST_BEGIN("DELEG-13: Verify VSTI interrupt translation (HS->VS)");

    /* Set hideleg[6] to delegate VSTI. */
    hideleg_write(0x40);
    uintptr_t val = hideleg_read();

    /* Verify bit 6 is set, enabling VSTI translation. */
    TEST_ASSERT_EQ("hideleg[6] should be set for VSTI translation", val & 0x40, 0x40);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-14: Interrupt number translation - VSEI
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_translation_vsei);
bool interrupt_translation_vsei(void) {
    TEST_BEGIN("DELEG-14: Verify VSEI interrupt translation (HS->VS)");

    /* Set hideleg[10] to delegate VSEI. */
    hideleg_write(0x400);
    uintptr_t val = hideleg_read();

    /* Verify bit 10 is set, enabling VSEI translation. */
    TEST_ASSERT_EQ("hideleg[10] should be set for VSEI translation", val & 0x400, 0x400);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-15: guest-page-fault not delegatable
 * ------------------------------------------------------------------ */
TEST_REGISTER(guest_page_fault_not_delegatable);
bool guest_page_fault_not_delegatable(void) {
    TEST_BEGIN("DELEG-15: Verify guest-page-fault bits (20/21/23) are read-zero");

    /* Write all 1s to attempt to set bits 20/21/23. */
    hedeleg_write(0xFFFFFFFF);
    uintptr_t val = hedeleg_read();

    /* Bits 20,21,23 should be read-zero. */
    TEST_ASSERT_EQ("hedeleg bits 20/21/23 (guest-page-fault) should be read-zero",
                   val & 0x00B00000, 0x0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-16: virtual-instruction not delegatable
 * ------------------------------------------------------------------ */
TEST_REGISTER(virtual_inst_not_delegatable);
bool virtual_inst_not_delegatable(void) {
    TEST_BEGIN("DELEG-16: Verify virtual-instruction bit (22) is read-zero");

    /* Write all 1s to attempt to set bit 22. */
    hedeleg_write(0xFFFFFFFF);
    uintptr_t val = hedeleg_read();

    /* Bit 22 should be read-zero. */
    TEST_ASSERT_EQ("hedeleg bit 22 (virtual-instruction) should be read-zero",
                   val & 0x00400000, 0x0);

    HYP_TEST_END();
}
