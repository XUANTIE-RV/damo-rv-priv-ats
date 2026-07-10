/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 17: mideleg/mip/mie enhancements
 *
 * Tests MIDLG-01 through MIDLG-07 verify delegation and interrupt enhancements.
 *
 * SPEC references:
 *   norm:mideleg_acc_h  - mideleg bits 10/6/2 read-only 1;
 *                         bit 12 read-only 1 when GEILEN>0
 *   norm:mideleg_hroz   - for mideleg=0 bits, corresponding
 *                         hideleg/hip/hie bits are read-only zeros
 *   norm:mip_mie_vs     - mip/mie have additional active VS bits
 *   norm:mip_mie_alias  - SGEIP/VSEIP/VSTIP/VSSIP in mip alias hip;
 *                         SGEIE/VSEIE/VSTIE/VSSIE in mie alias hie
 */

#include "test_helpers.h"

/* CSR addresses for hypervisor registers not exposed via named accessors. */
#define CSR_HIP       0x644
#define CSR_HIE       0x604
#define CSR_HGEIE     0x607

/* VS / SGE interrupt bit positions (shared by mip/mie/hip/hie). */
#define VS_BIT_VSSI   (1UL << 2)
#define VS_BIT_VSTI   (1UL << 6)
#define VS_BIT_VSEI   (1UL << 10)
#define VS_BIT_SGEI   (1UL << 12)
#define VS_ALL_BITS   (VS_BIT_VSSI | VS_BIT_VSTI | VS_BIT_VSEI | VS_BIT_SGEI)

/* ------------------------------------------------------------------
 * MIDLG-01: mideleg bits 10/6/2 read-only 1
 *
 * SPEC norm:mideleg_acc_h: bits 10, 6, and 2 of mideleg are each
 * read-only one when the hypervisor extension is implemented.
 * ------------------------------------------------------------------ */
TEST_REGISTER(mideleg_readonly_bits);
bool mideleg_readonly_bits(void)
{
    TEST_BEGIN("MIDLG-01: Verify mideleg bits 10/6/2 are read-only 1");

    /* Read mideleg. */
    uintptr_t val;
    asm volatile("csrr %0, mideleg" : "=r"(val));

    /* Verify bits 10, 6, 2 are 1. */
    uintptr_t mask = VS_BIT_VSEI | VS_BIT_VSTI | VS_BIT_VSSI;
    TEST_ASSERT_EQ("mideleg bits 10/6/2 should be 1", val & mask, mask);

    /* Try to clear these bits. */
    val &= ~mask;
    asm volatile("csrw mideleg, %0" :: "r"(val));

    /* Read back and verify they are still 1. */
    asm volatile("csrr %0, mideleg" : "=r"(val));
    TEST_ASSERT_EQ("mideleg bits 10/6/2 should still be 1 after clear attempt",
                   val & mask, mask);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MIDLG-02: mideleg bit 12 read-only 1 (GEILEN>0)
 *
 * SPEC norm:mideleg_acc_h: if GEILEN is nonzero, bit 12 of mideleg
 * is read-only one.  We detect GEILEN by writing all-ones to hgeie
 * and reading back.
 * ------------------------------------------------------------------ */
TEST_REGISTER(mideleg_bit12_geilen);
bool mideleg_bit12_geilen(void)
{
    TEST_BEGIN("MIDLG-02: Verify mideleg bit 12 read-only 1 when GEILEN>0");

    /* Detect GEILEN: write all 1s to hgeie and read back. */
    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)-1) : "memory");
    uintptr_t hgeie;
    asm volatile("csrr %0, 0x607" : "=r"(hgeie));
    /* Restore hgeie to 0 to avoid spurious guest external interrupts. */
    asm volatile("csrw 0x607, zero" ::: "memory");

    if (hgeie == 0)
    {
        TEST_SKIP("GEILEN=0 (hgeie reads zero), bit 12 not required to be RO1");
    }

    /* GEILEN>0: read mideleg and verify bit 12 is 1. */
    uintptr_t val;
    asm volatile("csrr %0, mideleg" : "=r"(val));
    TEST_ASSERT_EQ("mideleg bit 12 should be 1 when GEILEN>0",
                   val & VS_BIT_SGEI, VS_BIT_SGEI);

    /* Attempt to clear bit 12 and verify it remains 1. */
    val &= ~VS_BIT_SGEI;
    asm volatile("csrw mideleg, %0" :: "r"(val));

    asm volatile("csrr %0, mideleg" : "=r"(val));
    TEST_ASSERT_EQ("mideleg bit 12 should still be 1 after clear attempt",
                   val & VS_BIT_SGEI, VS_BIT_SGEI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MIDLG-03: hideleg/hip/hie read-only zero for mideleg=0 bits
 *
 * SPEC norm:mideleg_hroz: for bits of mideleg that are zero, the
 * corresponding bits in hideleg, hip, and hie are read-only zeros.
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_readonly_zero);
bool hideleg_readonly_zero(void)
{
    TEST_BEGIN("MIDLG-03: Verify hideleg/hip/hie bits RO-zero when mideleg=0");

    uintptr_t mideleg;
    asm volatile("csrr %0, mideleg" : "=r"(mideleg));

    /*
     * Find a writable candidate bit: scan bits [15:0] excluding the
     * always-delegated VS bits (10/6/2) and SGEI (12).  We look for
     * a bit that is currently 0 in mideleg.
     */
    uintptr_t exclude = VS_BIT_VSSI | VS_BIT_VSTI | VS_BIT_VSEI | VS_BIT_SGEI;
    uintptr_t candidate = 0;
    for (int i = 0; i < 16; i++)
    {
        uintptr_t b = 1UL << i;
        if ((b & exclude) == 0 && (mideleg & b) == 0)
        {
            candidate = b;
            break;
        }
    }

    if (candidate == 0)
    {
        TEST_SKIP("No mideleg=0 candidate bit found in [15:0]");
    }

    /* --- hideleg: attempt to set the candidate bit, expect read-only 0 --- */
    uintptr_t hideleg;
    asm volatile("csrr %0, hideleg" : "=r"(hideleg));
    hideleg |= candidate;
    asm volatile("csrw hideleg, %0" :: "r"(hideleg));
    asm volatile("csrr %0, hideleg" : "=r"(hideleg));
    TEST_ASSERT_EQ("hideleg bit should read-zero when mideleg bit=0",
                   hideleg & candidate, (uintptr_t)0);

    /* --- hip: attempt to set the candidate bit, expect read-only 0 --- */
    uintptr_t hip;
    asm volatile("csrr %0, 0x644" : "=r"(hip));
    hip |= candidate;
    asm volatile("csrw 0x644, %0" :: "r"(hip));
    asm volatile("csrr %0, 0x644" : "=r"(hip));
    TEST_ASSERT_EQ("hip bit should read-zero when mideleg bit=0",
                   hip & candidate, (uintptr_t)0);

    /* --- hie: attempt to set the candidate bit, expect read-only 0 --- */
    uintptr_t hie;
    asm volatile("csrr %0, 0x604" : "=r"(hie));
    hie |= candidate;
    asm volatile("csrw 0x604, %0" :: "r"(hie));
    asm volatile("csrr %0, 0x604" : "=r"(hie));
    TEST_ASSERT_EQ("hie bit should read-zero when mideleg bit=0",
                   hie & candidate, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MIDLG-04: mip VS interrupt bits are hip aliases (VSSIP/VSTIP/VSEIP)
 *
 * SPEC norm:mip_mie_alias: VSEIP, VSTIP, and VSSIP in mip are
 * aliases for the same bits in hip.
 * ------------------------------------------------------------------ */
TEST_REGISTER(mip_vSSIP_alias);
bool mip_vSSIP_alias(void)
{
    TEST_BEGIN("MIDLG-04: Verify mip VSSIP/VSTIP/VSEIP are hip aliases");

    /* ---- VSSIP (bit 2): set via hvip, verify mip reflects it ---- */
    hvip_set_vssi(true);

    uintptr_t mip, hip;
    asm volatile("csrr %0, mip" : "=r"(mip));
    asm volatile("csrr %0, 0x644" : "=r"(hip));
    TEST_ASSERT_EQ("mip.VSSIP should be 1 when hvip.VSSI set",
                   mip & VS_BIT_VSSI, VS_BIT_VSSI);
    TEST_ASSERT_EQ("hip.VSSIP should be 1 when hvip.VSSI set",
                   hip & VS_BIT_VSSI, VS_BIT_VSSI);

    hvip_set_vssi(false);
    asm volatile("csrr %0, mip" : "=r"(mip));
    asm volatile("csrr %0, 0x644" : "=r"(hip));
    TEST_ASSERT_EQ("mip.VSSIP should be 0 when hvip.VSSI cleared",
                   mip & VS_BIT_VSSI, (uintptr_t)0);
    TEST_ASSERT_EQ("hip.VSSIP should be 0 when hvip.VSSI cleared",
                   hip & VS_BIT_VSSI, (uintptr_t)0);

    /* ---- VSTIP (bit 6): set via hvip, verify mip reflects it ---- */
    hvip_set_vsti(true);

    asm volatile("csrr %0, mip" : "=r"(mip));
    asm volatile("csrr %0, 0x644" : "=r"(hip));
    TEST_ASSERT_EQ("mip.VSTIP should be 1 when hvip.VSTI set",
                   mip & VS_BIT_VSTI, VS_BIT_VSTI);
    TEST_ASSERT_EQ("hip.VSTIP should be 1 when hvip.VSTI set",
                   hip & VS_BIT_VSTI, VS_BIT_VSTI);

    hvip_set_vsti(false);
    asm volatile("csrr %0, mip" : "=r"(mip));
    asm volatile("csrr %0, 0x644" : "=r"(hip));
    TEST_ASSERT_EQ("mip.VSTIP should be 0 when hvip.VSTI cleared",
                   mip & VS_BIT_VSTI, (uintptr_t)0);
    TEST_ASSERT_EQ("hip.VSTIP should be 0 when hvip.VSTI cleared",
                   hip & VS_BIT_VSTI, (uintptr_t)0);

    /* ---- VSEIP (bit 10): set via hvip, verify mip reflects it ---- */
    hvip_set_vsei(true);

    asm volatile("csrr %0, mip" : "=r"(mip));
    asm volatile("csrr %0, 0x644" : "=r"(hip));
    TEST_ASSERT_EQ("mip.VSEIP should be 1 when hvip.VSEI set",
                   mip & VS_BIT_VSEI, VS_BIT_VSEI);
    TEST_ASSERT_EQ("hip.VSEIP should be 1 when hvip.VSEI set",
                   hip & VS_BIT_VSEI, VS_BIT_VSEI);

    hvip_set_vsei(false);
    asm volatile("csrr %0, mip" : "=r"(mip));
    asm volatile("csrr %0, 0x644" : "=r"(hip));
    TEST_ASSERT_EQ("mip.VSEIP should be 0 when hvip.VSEI cleared",
                   mip & VS_BIT_VSEI, (uintptr_t)0);
    TEST_ASSERT_EQ("hip.VSEIP should be 0 when hvip.VSEI cleared",
                   hip & VS_BIT_VSEI, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MIDLG-05: mip SGEIP is hip.SGEIP alias (GEILEN>0)
 *
 * SPEC norm:mip_mie_alias: SGEIP in mip is an alias for the same
 * bit in hip.  This test verifies the alias relationship when
 * guest external interrupts are implemented.
 * ------------------------------------------------------------------ */
TEST_REGISTER(mip_vSEIP_alias);
bool mip_vSEIP_alias(void)
{
    TEST_BEGIN("MIDLG-05: Verify mip SGEIP is hip.SGEIP alias");

    /* Detect GEILEN to see if SGEIP is supported. */
    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)-1) : "memory");
    uintptr_t hgeie;
    asm volatile("csrr %0, 0x607" : "=r"(hgeie));
    asm volatile("csrw 0x607, zero" ::: "memory");

    if (hgeie == 0)
    {
        TEST_SKIP("GEILEN=0, SGEIP not implemented");
    }

    /*
     * SGEIP (bit 12) in mip/hip is the OR of all active guest external
     * interrupts.  We cannot directly set it via hvip; instead we verify
     * the alias by reading both mip and hip and confirming they match
     * on bit 12.
     */
    uintptr_t mip, hip;
    asm volatile("csrr %0, mip" : "=r"(mip));
    asm volatile("csrr %0, 0x644" : "=r"(hip));
    TEST_ASSERT_EQ("mip.SGEIP should equal hip.SGEIP (alias)",
                   mip & VS_BIT_SGEI, hip & VS_BIT_SGEI);

    /*
     * Enable a guest external interrupt source via hgeie and verify
     * that mip.SGEIP and hip.SGEIP remain consistent.
     */
    asm volatile("csrw 0x607, %0" :: "r"(1UL) : "memory");
    asm volatile("csrr %0, mip" : "=r"(mip));
    asm volatile("csrr %0, 0x644" : "=r"(hip));
    TEST_ASSERT_EQ("mip.SGEIP should equal hip.SGEIP after hgeie write",
                   mip & VS_BIT_SGEI, hip & VS_BIT_SGEI);
    /* Restore hgeie. */
    asm volatile("csrw 0x607, zero" ::: "memory");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MIDLG-06: mie VS/SGE enable bits are hie aliases
 *
 * SPEC norm:mip_mie_alias: SGEIE, VSEIE, VSTIE, and VSSIE in mie
 * are aliases for the same bits in hie.
 * ------------------------------------------------------------------ */
TEST_REGISTER(mie_vSSIE_alias);
bool mie_vSSIE_alias(void)
{
    TEST_BEGIN("MIDLG-06: Verify mie VSSIE/VSTIE/VSEIE/SGEIE are hie aliases");

    /* Save original hie and clear it. */
    uintptr_t hie_orig;
    asm volatile("csrr %0, 0x604" : "=r"(hie_orig));
    asm volatile("csrw 0x604, zero" ::: "memory");

    uintptr_t hie, mie_val;

    /* ---- VSSIE (bit 2): set via hie, verify mie reflects it ---- */
    asm volatile("csrs 0x604, %0" :: "r"(VS_BIT_VSSI) : "memory");
    asm volatile("csrr %0, 0x604" : "=r"(hie));
    asm volatile("csrr %0, mie" : "=r"(mie_val));
    TEST_ASSERT_EQ("hie.VSSIE should be 1",
                   hie & VS_BIT_VSSI, VS_BIT_VSSI);
    TEST_ASSERT_EQ("mie.VSSIE should be 1 when hie.VSSIE set",
                   mie_val & VS_BIT_VSSI, VS_BIT_VSSI);
    asm volatile("csrc 0x604, %0" :: "r"(VS_BIT_VSSI) : "memory");
    asm volatile("csrr %0, mie" : "=r"(mie_val));
    TEST_ASSERT_EQ("mie.VSSIE should be 0 when hie.VSSIE cleared",
                   mie_val & VS_BIT_VSSI, (uintptr_t)0);

    /* ---- VSTIE (bit 6): set via hie, verify mie reflects it ---- */
    asm volatile("csrs 0x604, %0" :: "r"(VS_BIT_VSTI) : "memory");
    asm volatile("csrr %0, 0x604" : "=r"(hie));
    asm volatile("csrr %0, mie" : "=r"(mie_val));
    TEST_ASSERT_EQ("hie.VSTIE should be 1",
                   hie & VS_BIT_VSTI, VS_BIT_VSTI);
    TEST_ASSERT_EQ("mie.VSTIE should be 1 when hie.VSTIE set",
                   mie_val & VS_BIT_VSTI, VS_BIT_VSTI);
    asm volatile("csrc 0x604, %0" :: "r"(VS_BIT_VSTI) : "memory");
    asm volatile("csrr %0, mie" : "=r"(mie_val));
    TEST_ASSERT_EQ("mie.VSTIE should be 0 when hie.VSTIE cleared",
                   mie_val & VS_BIT_VSTI, (uintptr_t)0);

    /* ---- VSEIE (bit 10): set via hie, verify mie reflects it ---- */
    asm volatile("csrs 0x604, %0" :: "r"(VS_BIT_VSEI) : "memory");
    asm volatile("csrr %0, 0x604" : "=r"(hie));
    asm volatile("csrr %0, mie" : "=r"(mie_val));
    TEST_ASSERT_EQ("hie.VSEIE should be 1",
                   hie & VS_BIT_VSEI, VS_BIT_VSEI);
    TEST_ASSERT_EQ("mie.VSEIE should be 1 when hie.VSEIE set",
                   mie_val & VS_BIT_VSEI, VS_BIT_VSEI);
    asm volatile("csrc 0x604, %0" :: "r"(VS_BIT_VSEI) : "memory");
    asm volatile("csrr %0, mie" : "=r"(mie_val));
    TEST_ASSERT_EQ("mie.VSEIE should be 0 when hie.VSEIE cleared",
                   mie_val & VS_BIT_VSEI, (uintptr_t)0);

    /* ---- SGEIE (bit 12): set via hie, verify mie reflects it ---- */
    asm volatile("csrs 0x604, %0" :: "r"(VS_BIT_SGEI) : "memory");
    asm volatile("csrr %0, 0x604" : "=r"(hie));
    asm volatile("csrr %0, mie" : "=r"(mie_val));
    /*
     * SGEIE in hie is read-only 0 when GEILEN=0.  Check if it stuck.
     */
    if (hie & VS_BIT_SGEI)
    {
        TEST_ASSERT_EQ("mie.SGEIE should be 1 when hie.SGEIE set",
                       mie_val & VS_BIT_SGEI, VS_BIT_SGEI);
        asm volatile("csrc 0x604, %0" :: "r"(VS_BIT_SGEI) : "memory");
        asm volatile("csrr %0, mie" : "=r"(mie_val));
        TEST_ASSERT_EQ("mie.SGEIE should be 0 when hie.SGEIE cleared",
                       mie_val & VS_BIT_SGEI, (uintptr_t)0);
    }
    else
    {
        TEST_SKIP("hgeie=0 or GEILEN=0, SGEIE is read-only 0 in hie");
    }

    /* Restore original hie. */
    asm volatile("csrw 0x604, %0" :: "r"(hie_orig) : "memory");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * MIDLG-07: mip/mie VS interrupt bits are writable/active
 *
 * SPEC norm:mip_mie_vs: the hypervisor extension gives registers
 * mip and mie additional active bits for the hypervisor-added
 * interrupts.  We verify mie VS enable bits are writable and
 * read back correctly.
 * ------------------------------------------------------------------ */
TEST_REGISTER(mip_mie_vs_bits_visible);
bool mip_mie_vs_bits_visible(void)
{
    TEST_BEGIN("MIDLG-07: Verify mip/mie VS interrupt bits are active");

    /* Save original mie. */
    uintptr_t mie_orig;
    asm volatile("csrr %0, mie" : "=r"(mie_orig));

    /*
     * VS enable bits: VSSIE(2), VSTIE(6), VSEIE(10).
     * SGEIE(12) may be RO0 when GEILEN=0, so we test it separately.
     */
    uintptr_t vs_enable = VS_BIT_VSSI | VS_BIT_VSTI | VS_BIT_VSEI;

    /* Write all VS enable bits to mie. */
    asm volatile("csrs mie, %0" :: "r"(vs_enable));

    /* Read back and verify all three bits stuck. */
    uintptr_t mie_val;
    asm volatile("csrr %0, mie" : "=r"(mie_val));
    TEST_ASSERT_EQ("mie VS enable bits should be writable",
                   mie_val & vs_enable, vs_enable);

    /* Clear all VS enable bits. */
    asm volatile("csrc mie, %0" :: "r"(vs_enable));
    asm volatile("csrr %0, mie" : "=r"(mie_val));
    TEST_ASSERT_EQ("mie VS enable bits should be clearable",
                   mie_val & vs_enable, (uintptr_t)0);

    /*
     * Verify mip VS bits (2/6/10) are readable.  These reflect
     * current pending state so we just confirm the bit positions
     * are not hardwired to zero (i.e., the field exists).
     * We set hvip to inject VSSI and check mip bit 2.
     */
    hvip_set_vssi(true);
    uintptr_t mip;
    asm volatile("csrr %0, mip" : "=r"(mip));
    TEST_ASSERT_EQ("mip.VSSIP should be readable and reflect hvip",
                   mip & VS_BIT_VSSI, VS_BIT_VSSI);
    hvip_set_vssi(false);

    /* Restore original mie. */
    asm volatile("csrw mie, %0" :: "r"(mie_orig));

    HYP_TEST_END();
}
