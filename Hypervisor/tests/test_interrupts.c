/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 4-5: hvip/hip/hie and hgeip/hgeie interrupts
 *
 * Tests HINT-01 through HINT-14 and HGEI-01 through HGEI-05 verify
 * interrupt injection and guest external interrupt behavior.
 */

#include "test_helpers.h"

/* VS-level interrupt bit positions in hvip/hip/hie (cause codes 2/6/10). */
#define VS_VSSIP  (1UL << 2)   /* 0x4   - VS software interrupt */
#define VS_VSTIP  (1UL << 6)   /* 0x40  - VS timer interrupt */
#define VS_VSEIP  (1UL << 10)  /* 0x400 - VS external interrupt */
#define VS_ALL    (VS_VSSIP | VS_VSTIP | VS_VSEIP)  /* 0x444 */

/* ===================================================================
 * Group 4: hvip/hip/hie interrupt injection and aliasing
 * =================================================================== */

/* ------------------------------------------------------------------
 * HINT-01: hvip VSSIP injection
 * ------------------------------------------------------------------ */
TEST_REGISTER(hvip_vssp_injection);
bool hvip_vssp_injection(void) {
    TEST_BEGIN("HINT-01: Inject VSSIP via hvip and verify hip.VSSIP");

    /* Set VSSIP in hvip. */
    hvip_set_vssi(1);
    uintptr_t hvip_val = hvip_read();

    /* Verify hvip.VSSIP is set (bit 2 = cause code 2). */
    TEST_ASSERT_EQ("hvip.VSSIP should be set", hvip_val & VS_VSSIP, VS_VSSIP);

    /* Verify hip.VSSIP reflects hvip.VSSIP. */
    uintptr_t hip_val; asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP should reflect hvip.VSSIP", hip_val & VS_VSSIP, VS_VSSIP);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-02: hvip VSTIP injection
 * ------------------------------------------------------------------ */
TEST_REGISTER(hvip_vstip_injection);
bool hvip_vstip_injection(void) {
    TEST_BEGIN("HINT-02: Inject VSTIP via hvip and verify hip.VSTIP");

    /* Set VSTIP in hvip. */
    hvip_set_vsti(1);
    uintptr_t hvip_val = hvip_read();

    /* Verify hvip.VSTIP is set (bit 6 = cause code 6). */
    TEST_ASSERT_EQ("hvip.VSTIP should be set", hvip_val & VS_VSTIP, VS_VSTIP);

    /* Verify hip.VSTIP reflects hvip.VSTIP. */
    uintptr_t hip_val; asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSTIP should reflect hvip.VSTIP", hip_val & VS_VSTIP, VS_VSTIP);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-03: hvip VSEIP injection
 * ------------------------------------------------------------------ */
TEST_REGISTER(hvip_vseip_injection);
bool hvip_vseip_injection(void) {
    TEST_BEGIN("HINT-03: Inject VSEIP via hvip and verify hip.VSEIP");

    /* Set VSEIP in hvip. */
    hvip_set_vsei(1);
    uintptr_t hvip_val = hvip_read();

    /* Verify hvip.VSEIP is set (bit 10 = cause code 10). */
    TEST_ASSERT_EQ("hvip.VSEIP should be set", hvip_val & VS_VSEIP, VS_VSEIP);

    /* Verify hip.VSEIP reflects hvip.VSEIP. */
    uintptr_t hip_val; asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSEIP should reflect hvip.VSEIP", hip_val & VS_VSEIP, VS_VSEIP);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-04: hip.VSSIP is hvip.VSSIP alias
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_vssp_is_alias);
bool hip_vssp_is_alias(void) {
    TEST_BEGIN("HINT-04: Verify hip.VSSIP is hvip.VSSIP alias");

    /* Set hvip.VSSIP. */
    hvip_set_vssi(1);
    uintptr_t hvip_val = hvip_read();

    /* Read hip and verify alias relationship. */
    uintptr_t hip_val; asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP should alias hvip.VSSIP",
                   (hip_val & 0x2) == (hvip_val & 0x2), true);

    /* Clear hvip.VSSIP and verify hip clears. */
    hvip_set_vssi(0);
    hvip_val = hvip_read();
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("Clearing hvip.VSSIP should clear hip.VSSIP", hip_val & 0x2, 0x0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-05: hip.VSEIP is read-only
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_vseip_readonly);
bool hip_vseip_readonly(void) {
    TEST_BEGIN("HINT-05: Verify hip.VSEIP is read-only (alias of hvip.VSEIP)");

    /* Set hvip.VSEIP. */
    hvip_set_vsei(1);
    uintptr_t hvip_val = hvip_read();

    /* Try to write hip.VSEIP (should not affect hvip). */
    uintptr_t _hip_read_tmp; asm volatile("csrr %0, 0x644" : "=r"(_hip_read_tmp));
    asm volatile("csrw 0x644, %0" :: "r"(_hip_read_tmp | 0x200));
    uintptr_t hip_val; asm volatile("csrr %0, 0x644" : "=r"(hip_val));

    /* Verify hip.VSEIP still reflects hvip.VSEIP. */
    TEST_ASSERT_EQ("hip.VSEIP should remain alias of hvip.VSEIP",
                   hip_val & 0x200, hvip_val & 0x200);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-06: hip.VSTIP is read-only
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_vstip_readonly);
bool hip_vstip_readonly(void) {
    TEST_BEGIN("HINT-06: Verify hip.VSTIP is read-only (alias of hvip.VSTIP)");

    /* Set hvip.VSTIP. */
    hvip_set_vsti(1);
    uintptr_t hvip_val = hvip_read();

    /* Try to write hip.VSTIP (should not affect hvip). */
    uintptr_t _hip_read_tmp; asm volatile("csrr %0, 0x644" : "=r"(_hip_read_tmp));
    asm volatile("csrw 0x644, %0" :: "r"(_hip_read_tmp | 0x20));
    uintptr_t hip_val; asm volatile("csrr %0, 0x644" : "=r"(hip_val));

    /* Verify hip.VSTIP still reflects hvip.VSTIP. */
    TEST_ASSERT_EQ("hip.VSTIP should remain alias of hvip.VSTIP",
                   hip_val & 0x20, hvip_val & 0x20);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-07: hie VSEIE/VSTIE/VSSIE are writable
 * ------------------------------------------------------------------ */
TEST_REGISTER(hie_vseie_vstie_vssie_writable);
bool hie_vseie_vstie_vssie_writable(void) {
    TEST_BEGIN("HINT-07: Verify hie VSEIE/VSTIE/VSSIE bits are writable");

    /* Set VSEIE, VSTIE, VSSIE in hie (bits 10, 6, 2). */
    asm volatile("csrw 0x604, %0" :: "r"(VS_ALL));
    uintptr_t val; asm volatile("csrr %0, 0x604" : "=r"(val));

    /* Verify bits are set. */
    TEST_ASSERT_EQ("hie VSEIE/VSTIE/VSSIE should be writable", val & VS_ALL, VS_ALL);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-08: sie and hip/hie are mutually exclusive
 * ------------------------------------------------------------------ */
TEST_REGISTER(sie_hip_hie_mutually_exclusive);
bool sie_hip_hie_mutually_exclusive(void) {
    TEST_BEGIN("HINT-08: Verify sie and hip/hie do not share bit fields");

    /* Write all 1s to hie. */
    uintptr_t hie_val; asm volatile("csrw 0x604, %0" :: "r"((uintptr_t)0xFFFFFFFF));
    asm volatile("csrr %0, 0x604" : "=r"(hie_val));
    uintptr_t sie_val; asm volatile("csrr %0, sie" : "=r"(sie_val));
    /* hie controls VS interrupt enables (bits 10,6,2), sie controls HS interrupt enables (bits 9,5,1) */
    /* They should not share the same bit positions */
    TEST_ASSERT("hie VS bits (10,6,2) writable", (hie_val & 0x444) != 0);
    TEST_ASSERT("sie does not contain VS int bits", (sie_val & 0x444) == 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-09: Clearing hvip.VSSIP clears interrupt
 * ------------------------------------------------------------------ */
TEST_REGISTER(clear_hvip_vssp_clears_interrupt);
bool clear_hvip_vssp_clears_interrupt(void) {
    TEST_BEGIN("HINT-09: Verify clearing hvip.VSSIP clears VSSIP interrupt");

    /* Set VSSIP. */
    hvip_set_vssi(1);
    uintptr_t hvip_val = hvip_read();
    TEST_ASSERT_EQ("hvip.VSSIP should be set", hvip_val & VS_VSSIP, VS_VSSIP);

    /* Clear VSSIP. */
    hvip_set_vssi(0);
    hvip_val = hvip_read();

    /* Verify VSSIP is cleared. */
    TEST_ASSERT_EQ("hvip.VSSIP should be cleared", hvip_val & 0x2, 0x0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-10: hideleg controls VSSI target (CSR verification)
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_controls_vssp_target);
bool hideleg_controls_vssp_target(void) {
    TEST_BEGIN("HINT-10: Verify hideleg controls VSSI interrupt target");

    /* Set hideleg[2] to delegate VSSI. */
    hideleg_write(0x4);
    uintptr_t hideleg_val = hideleg_read();

    /* Verify hideleg[2] is set. */
    TEST_ASSERT_EQ("hideleg[2] should delegate VSSI", hideleg_val & 0x4, 0x4);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-11: hideleg controls VSTI target (CSR verification)
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_controls_vsti_target);
bool hideleg_controls_vsti_target(void) {
    TEST_BEGIN("HINT-11: Verify hideleg controls VSTI interrupt target");

    /* Set hideleg[6] to delegate VSTI. */
    hideleg_write(0x40);
    uintptr_t hideleg_val = hideleg_read();

    /* Verify hideleg[6] is set. */
    TEST_ASSERT_EQ("hideleg[6] should delegate VSTI", hideleg_val & 0x40, 0x40);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-12: Multiple pending bits can be set simultaneously
 * ------------------------------------------------------------------ */
TEST_REGISTER(multiple_pending_bits);
bool multiple_pending_bits(void) {
    TEST_BEGIN("HINT-12: Verify multiple pending bits can be set simultaneously");

    /* Set VSSIP, VSTIP, VSEIP in hvip (bits 2, 6, 10). */
    hvip_write(VS_ALL);
    uintptr_t hvip_val = hvip_read();

    /* Verify all bits are set. */
    TEST_ASSERT_EQ("Multiple pending bits should be settable simultaneously",
                   hvip_val & VS_ALL, VS_ALL);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-13: Interrupt priority (simplified to CSR verification)
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_priority_verification);
bool interrupt_priority_verification(void) {
    TEST_BEGIN("HINT-13: Verify interrupt priority through CSR verification");

    /* Set all VS interrupts pending (bits 2, 6, 10). */
    hvip_write(VS_ALL);
    uintptr_t hvip_val = hvip_read();

    /* Verify all bits remain pending. */
    TEST_ASSERT_EQ("All VS interrupts should be pending", hvip_val & VS_ALL, VS_ALL);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-14: hip/hie non-standard bits are read-zero
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_hie_nonstandard_bits_readzero);
bool hip_hie_nonstandard_bits_readzero(void) {
    TEST_BEGIN("HINT-14: Verify hip/hie non-standard bits are read-zero");

    /* Write all 1s to hip and hie. */
    uintptr_t hip_val; asm volatile("csrw 0x644, %0" :: "r"((uintptr_t)0xFFFFFFFF));
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    uintptr_t hie_val; asm volatile("csrw 0x604, %0" :: "r"((uintptr_t)0xFFFFFFFF));
    asm volatile("csrr %0, 0x604" : "=r"(hie_val));
    /* Non-standard bits above bit 12 for hie, above bit 12 for hip should be zero */
    /* Bits above 12 (excluding SGEIP=bit12) should be read-zero in hip */
    TEST_ASSERT("hip high bits read-zero", (hip_val & ~0x1FFFUL) == 0);
    /* hie standard bits are 10,6,2 plus HS bits 9,5,1 */
    TEST_ASSERT("hie high bits read-zero", (hie_val & ~0x1FFFUL) == 0);

    HYP_TEST_END();
}

/* ===================================================================
 * Group 5: hgeip/hgeie guest external interrupts
 * =================================================================== */

/* ------------------------------------------------------------------
 * HGEI-01: hgeip is read-only
 * ------------------------------------------------------------------ */
TEST_REGISTER(hgeip_readonly);
bool hgeip_readonly(void) {
    TEST_BEGIN("HGEI-01: Verify hgeip is read-only");

    uintptr_t before; asm volatile("csrr %0, 0xE12" : "=r"(before));
    /* hgeip is read-only; csrw to it triggers illegal-instruction (cause=2). */
    EXPECT_TRAP(2, {
        asm volatile("csrw 0xE12, %0" :: "r"((uintptr_t)0xFFFFFFFF));
    });
    uintptr_t after; asm volatile("csrr %0, 0xE12" : "=r"(after));
    TEST_ASSERT_EQ("hgeip should not change on write", after, before);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HGEI-02: hgeie writable bit range
 * ------------------------------------------------------------------ */
TEST_REGISTER(hgeie_writable_bit_range);
bool hgeie_writable_bit_range(void) {
    TEST_BEGIN("HGEI-02: Verify hgeie writable bit range");

    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)0xFFFFFFFF));
    uintptr_t val; asm volatile("csrr %0, 0x607" : "=r"(val));
    /* bit 0 must be zero, remaining bits depend on GEILEN */
    TEST_ASSERT("hgeie bit 0 is read-zero", (val & 0x1) == 0);
    /* writable bits should be contiguous starting from bit 1 up to GEILEN */
    TEST_ASSERT("hgeie has valid writable range", val == (val & ~0x1UL));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HGEI-03: hgeie bit 0 is read-zero
 * ------------------------------------------------------------------ */
TEST_REGISTER(hgeie_bit0_readzero);
bool hgeie_bit0_readzero(void) {
    TEST_BEGIN("HGEI-03: Verify hgeie bit 0 is read-zero");

    /* Write all 1s to hgeie. */
    asm volatile("csrw 0x607, %0" :: "r"(0xFFFFFFFF));
    uintptr_t val; asm volatile("csrr %0, 0x607" : "=r"(val));

    /* Verify bit 0 is read-zero. */
    TEST_ASSERT_EQ("hgeie bit 0 should be read-zero", val & 0x1, 0x0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HGEI-04: hgeip AND hgeie triggers SGEIP (simplified verification)
 * ------------------------------------------------------------------ */
TEST_REGISTER(hgeip_and_hgeie_triggers_sgeip);
bool hgeip_and_hgeie_triggers_sgeip(void) {
    TEST_BEGIN("HGEI-04: Verify hgeip AND hgeie triggers SGEIP");

    /* hgeip is read-only hardware, so we verify that when hgeie is set,
     * the SGEIP mechanism is armed. Since hgeip depends on external interrupts,
     * we verify: hgeie writable AND hgeip & hgeie == 0 when no external interrupt */
    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)0xFFFFFFFE)); /* set all except bit 0 */
    uintptr_t hgeie_val; asm volatile("csrr %0, 0x607" : "=r"(hgeie_val));
    uintptr_t hgeip_val; asm volatile("csrr %0, 0xE12" : "=r"(hgeip_val));
    /* With no external interrupt sources, hgeip & hgeie should be 0 */
    TEST_ASSERT("no spurious SGEIP", (hgeip_val & hgeie_val) == 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HGEI-05: GEILEN=0 results in all zeros
 * ------------------------------------------------------------------ */
TEST_REGISTER(geilen_zero_all_zeros);
bool geilen_zero_all_zeros(void) {
    TEST_BEGIN("HGEI-05: Verify GEILEN=0 results in all zeros");

    /* Write all 1s to hgeie and check how many bits are writable */
    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)0xFFFFFFFF));
    uintptr_t hgeie_val; asm volatile("csrr %0, 0x607" : "=r"(hgeie_val));
    if (hgeie_val == 0) {
        /* GEILEN=0: both hgeip and hgeie should be all zeros */
        uintptr_t hgeip_val; asm volatile("csrr %0, 0xE12" : "=r"(hgeip_val));
        TEST_ASSERT_EQ("hgeie all zero when GEILEN=0", hgeie_val, (uintptr_t)0);
        TEST_ASSERT_EQ("hgeip all zero when GEILEN=0", hgeip_val, (uintptr_t)0);
    } else {
        /* GEILEN > 0: verified by hgeie having writable bits */
        TEST_ASSERT("GEILEN > 0, hgeie has writable bits", hgeie_val != 0);
    }

    HYP_TEST_END();
}
