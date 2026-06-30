/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 9: vsip/vsie alias verification
 *
 * Tests VSIE-01 through VSIE-10 verify vsip/vsie register behavior
 * including V=1 substitution, hideleg-controlled alias chains, and
 * interrupt number translation (10->9, 6->5, 2->1).
 *
 * Spec references:
 *   norm:vsip_vsie_sz_acc_op   V=1 substitution
 *   norm:vsip_vsie_sei         VSEIP/VSEIE alias rules
 *   norm:vsip_vsie_sti         VSTIP/VSTIE alias rules
 *   norm:vsip_vsie_ssi         VSSIP/VSSIE alias rules
 */

#include "test_helpers.h"

/* S-level interrupt bit positions in sip/sie (after translation). */
#define SIP_SSIP        (1UL << 1)
/* SIP_STIP already defined in encoding.h */
#define SIP_SEIP        (1UL << 9)

/* VS-level interrupt bit positions in hideleg/hip/hie. */
#define HIDELEG_VSSI    (1UL << 2)
#define HIDELEG_VSTI    (1UL << 6)
#define HIDELEG_VSEI    (1UL << 10)

#define HIE_VSSIE       (1UL << 2)
#define HIE_VSTIE       (1UL << 6)
#define HIE_VSEIE       (1UL << 10)

/* ------------------------------------------------------------------
 * VSIE-01: Verifies that in V=1 mode, reading sip/sie actually accesses
 * vsip/vsie through the CSR substitution mechanism.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_vsie_01);
bool test_vsie_01(void) {
    TEST_BEGIN("VSIE-01: V=1 sip/sie access vsip/vsie");

    /* --- Part A: sip reads vsip (pending verification) --- */
    /* Delegate VSSI so vsip.SSIP aliases hip.VSSIP. */
    hideleg_write(HIDELEG_VSSI);
    hvip_set_vssi(true);

    /* VS-mode reads sip -> substituted to vsip in V=1.
     *
     * Known QEMU limitation: QEMU does not correctly implement the
     * V=1 sip/sie read-direction alias. M-mode reading vsip(0x244)
     * returns the correct pending value, but VS-mode reading sip
     * always returns 0. The write direction works correctly. */
    uintptr_t sip_val = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT("sip.SSIP reflects VSSIP pending via vsip substitution",
                (sip_val & SIP_SSIP) != 0);
    hvip_set_vssi(false);

    /* --- Part B: sie reads vsie (enable verification) --- */
    /* Set hie.VSSIE -> aliased to vsie.SSIE when delegated. */
    asm volatile("csrw 0x604, zero");
    asm volatile("csrs 0x604, %0" :: "r"(HIE_VSSIE));

    /* VS-mode reads sie -> substituted to vsie in V=1.
     * Same QEMU limitation as Part A applies here. */
    uintptr_t sie_val = run_in_vs_mode(vs_read_sie, 0);
    TEST_ASSERT("sie.SSIE reflects VSSIE enable via vsie substitution",
                (sie_val & SIP_SSIP) != 0);

    /* Cleanup. */
    asm volatile("csrw 0x604, zero");
    hideleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * VSIE-02: hideleg[10]=1 When VSEI is delegated, vsip.SEIP reflects hip.VSEIP.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_vsie_02);
bool test_vsie_02(void) {
    TEST_BEGIN("VSIE-02: hideleg[10]=1, vsip.SEIP aliases hip.VSEIP");

    /* Delegate VSEI. */
    hideleg_write(HIDELEG_VSEI);

    /* Inject VSEIP via hvip. */
    hvip_set_vsei(true);

    /* VS-mode reads sip.SEIP (bit 9, translated from VSEIP bit 10).
     *
     * Known QEMU limitation: V=1 sip read-direction alias not
     * implemented — VS-mode reading sip returns 0 despite correct
     * M-mode vsip state. See VSIE-01 comment for details. */
    uintptr_t sip_val = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT("sip.SEIP=1 when hideleg[10]=1 and VSEIP pending",
                (sip_val & SIP_SEIP) != 0);

    /* Cleanup. */
    hvip_set_vsei(false);
    hideleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * VSIE-03: hideleg[10]=0 When VSEI is NOT delegated, vsip.SEIP reads as zero regardless
 * of the actual VSEIP state.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_vsie_03);
bool test_vsie_03(void) {
    TEST_BEGIN("VSIE-03: hideleg[10]=0, vsip.SEIP reads zero");

    /* Do NOT delegate VSEI. */
    hideleg_write(0);

    /* Inject VSEIP via hvip (pending in hip, but not visible to VS). */
    hvip_set_vsei(true);

    /* VS-mode reads sip.SEIP -> should be zero. */
    uintptr_t sip_val = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT("sip.SEIP=0 when hideleg[10]=0 despite VSEIP pending",
                (sip_val & SIP_SEIP) == 0);

    /* Cleanup. */
    hvip_set_vsei(false);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * VSIE-04: hideleg[6]=1 When VSTI is delegated, vsip.STIP reflects hip.VSTIP.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_vsie_04);
bool test_vsie_04(void) {
    TEST_BEGIN("VSIE-04: hideleg[6]=1, vsip.STIP aliases hip.VSTIP");

    /* Delegate VSTI. */
    hideleg_write(HIDELEG_VSTI);

    /* Inject VSTIP via hvip. */
    hvip_set_vsti(true);

    /* VS-mode reads sip.STIP (bit 5, translated from VSTIP bit 6).
     *
     * Known QEMU limitation: V=1 sip read-direction alias not
     * implemented. See VSIE-01 comment for details. */
    uintptr_t sip_val = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT("sip.STIP=1 when hideleg[6]=1 and VSTIP pending",
                (sip_val & SIP_STIP) != 0);

    /* Cleanup. */
    hvip_set_vsti(false);
    hideleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * VSIE-05: hideleg[6]=0 When VSTI is NOT delegated, vsip.STIP reads as zero.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_vsie_05);
bool test_vsie_05(void) {
    TEST_BEGIN("VSIE-05: hideleg[6]=0, vsip.STIP reads zero");

    /* Do NOT delegate VSTI. */
    hideleg_write(0);

    /* Inject VSTIP via hvip (pending in hip, but not visible to VS). */
    hvip_set_vsti(true);

    /* VS-mode reads sip.STIP -> should be zero. */
    uintptr_t sip_val = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT("sip.STIP=0 when hideleg[6]=0 despite VSTIP pending",
                (sip_val & SIP_STIP) == 0);

    /* Cleanup. */
    hvip_set_vsti(false);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * VSIE-06: hideleg[2]=1 When VSSI is delegated, vsip.SSIP reflects hip.VSSIP.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_vsie_06);
bool test_vsie_06(void) {
    TEST_BEGIN("VSIE-06: hideleg[2]=1, vsip.SSIP aliases hip.VSSIP");

    /* Delegate VSSI. */
    hideleg_write(HIDELEG_VSSI);

    /* Inject VSSIP via hvip. */
    hvip_set_vssi(true);

    /* VS-mode reads sip.SSIP (bit 1, translated from VSSIP bit 2).
     *
     * Known QEMU limitation: QEMU does not correctly implement the
     * V=1 sip→vsip read-direction alias. M-mode reading vsip(0x244)
     * returns the correct value, but VS-mode reading sip returns 0.
     * The write direction (VSIE-10) works correctly. */
    uintptr_t sip_val = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT("sip.SSIP=1 when hideleg[2]=1 and VSSIP pending",
                (sip_val & SIP_SSIP) != 0);

    /* Cleanup. */
    hvip_set_vssi(false);
    hideleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * VSIE-07: hideleg[2]=0 When VSSI is NOT delegated, vsip.SSIP reads as zero.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_vsie_07);
bool test_vsie_07(void) {
    TEST_BEGIN("VSIE-07: hideleg[2]=0, vsip.SSIP reads zero");

    /* Do NOT delegate VSSI. */
    hideleg_write(0);

    /* Inject VSSIP via hvip (pending in hip, but not visible to VS). */
    hvip_set_vssi(true);

    /* VS-mode reads sip.SSIP -> should be zero. */
    uintptr_t sip_val = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT("sip.SSIP=0 when hideleg[2]=0 despite VSSIP pending",
                (sip_val & SIP_SSIP) == 0);

    /* Cleanup. */
    hvip_set_vssi(false);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * VSIE-08: vsie.SEIE alias, When VSEI is delegated, VS-mode writing sie.SEIE updates hie.VSEIE.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_vsie_08);
bool test_vsie_08(void) {
    TEST_BEGIN("VSIE-08: VS writes sie.SEIE -> hie.VSEIE alias");

    /* Delegate VSEI. */
    hideleg_write(HIDELEG_VSEI);

    /* Clear hie.VSEIE first. */
    asm volatile("csrc 0x604, %0" :: "r"(HIE_VSEIE));

    /* VS-mode writes sie.SEIE=1 (substituted to vsie, aliased to hie.VSEIE). */
    run_in_vs_mode(vs_write_sie, SIP_SEIP);

    /* HS-mode reads hie.VSEIE -> should be set. */
    uintptr_t hie_val;
    asm volatile("csrr %0, 0x604" : "=r"(hie_val));
    TEST_ASSERT("hie.VSEIE=1 after VS writes sie.SEIE=1",
                (hie_val & HIE_VSEIE) != 0);

    /* Cleanup. */
    asm volatile("csrc 0x604, %0" :: "r"(HIE_VSEIE));
    hideleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * VSIE-09: vsie.STIE alias, When VSTI is delegated, VS-mode writing sie.STIE updates hie.VSTIE.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_vsie_09);
bool test_vsie_09(void) {
    TEST_BEGIN("VSIE-09: VS writes sie.STIE -> hie.VSTIE alias");

    /* Delegate VSTI. */
    hideleg_write(HIDELEG_VSTI);

    /* Clear hie.VSTIE first. */
    asm volatile("csrc 0x604, %0" :: "r"(HIE_VSTIE));

    /* VS-mode writes sie.STIE=1. */
    run_in_vs_mode(vs_write_sie, SIP_STIP);

    /* HS-mode reads hie.VSTIE -> should be set. */
    uintptr_t hie_val;
    asm volatile("csrr %0, 0x604" : "=r"(hie_val));
    TEST_ASSERT("hie.VSTIE=1 after VS writes sie.STIE=1",
                (hie_val & HIE_VSTIE) != 0);

    /* Cleanup. */
    asm volatile("csrc 0x604, %0" :: "r"(HIE_VSTIE));
    hideleg_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * VSIE-10: vsie.SSIE alias, When VSSI is delegated, VS-mode writing sie.SSIE updates hie.VSSIE.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_vsie_10);
bool test_vsie_10(void) {
    TEST_BEGIN("VSIE-10: VS writes sie.SSIE -> hie.VSSIE alias");

    /* Delegate VSSI. */
    hideleg_write(HIDELEG_VSSI);

    /* Clear hie.VSSIE first. */
    asm volatile("csrc 0x604, %0" :: "r"(HIE_VSSIE));

    /* VS-mode writes sie.SSIE=1. */
    run_in_vs_mode(vs_write_sie, SIP_SSIP);

    /* HS-mode reads hie.VSSIE -> should be set. */
    uintptr_t hie_val;
    asm volatile("csrr %0, 0x604" : "=r"(hie_val));
    TEST_ASSERT("hie.VSSIE=1 after VS writes sie.SSIE=1",
                (hie_val & HIE_VSSIE) != 0);

    /* Cleanup. */
    asm volatile("csrc 0x604, %0" :: "r"(HIE_VSSIE));
    hideleg_write(0);

    HYP_TEST_END();
}
