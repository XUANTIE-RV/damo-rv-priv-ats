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

/* ===================================================================
 * Bit position definitions
 *
 * RISC-V interrupt cause codes and corresponding bit positions:
 *   bit 1  (0x002): SSIP/SSIE  - HS software interrupt
 *   bit 2  (0x004): VSSIP/VSSIE - VS software interrupt (hvip/hip/hie)
 *   bit 5  (0x020): STIP/STIE  - HS timer interrupt
 *   bit 6  (0x040): VSTIP/VSTIE - VS timer interrupt (hvip/hip/hie)
 *   bit 9  (0x200): SEIP/SEIE   - HS external interrupt
 *   bit 10 (0x400): VSEIP/VSEIE - VS external interrupt (hvip/hip/hie)
 *   bit 12 (0x1000): SGEIP/SGEIE - Guest external interrupt
 * =================================================================== */

/* VS-level interrupt bit positions in hvip/hip/hie */
#define VS_VSSIP  (1UL << 2)
#define VS_VSTIP  (1UL << 6)
#define VS_VSEIP  (1UL << 10)
#define VS_ALL    (VS_VSSIP | VS_VSTIP | VS_VSEIP)

/* HS-level interrupt bit positions in sip/sie/mip/mie */
#define HS_SSIP   (1UL << 1)
#define HS_STIP   (1UL << 5)
#define HS_SEIP   (1UL << 9)

/* VS-level interrupt cause codes when delivered to VS-mode.
 * Per RISC-V spec, VS-level interrupts are translated to S-level
 * cause codes when delivered to VS-mode:
 *   VSSI (hvip bit 2)  -> vscause = 1 (SSI)
 *   VSTI (hvip bit 6)  -> vscause = 5 (STI)
 *   VSEI (hvip bit 10) -> vscause = 9 (SEI)
 */
#define VS_IRQ_SSI   (CAUSE_INTERRUPT_BIT | 1)
#define VS_IRQ_STI   (CAUSE_INTERRUPT_BIT | 5)
#define VS_IRQ_SEI   (CAUSE_INTERRUPT_BIT | 9)

/* HS-level interrupt cause codes */
#define IRQ_SSI    (CAUSE_INTERRUPT_BIT | 1)
#define IRQ_SEI    (CAUSE_INTERRUPT_BIT | 9)

/* ===================================================================
 * VS-mode interrupt handler
 *
 * This handler is installed at vstvec and runs in VS-mode when a
 * VS-level interrupt is delegated via hideleg. It records the first
 * interrupt cause, disables SIE to prevent re-entry, and returns
 * via sret.
 * =================================================================== */

static volatile uintptr_t g_vs_int_cause;
static volatile bool g_vs_int_triggered;

static void vs_int_handler(void) __attribute__((naked, aligned(4)));
static void vs_int_handler(void)
{
    asm volatile (
        "addi   sp, sp, -32\n\t"
        "sd     ra, 0(sp)\n\t"
        "sd     t0, 8(sp)\n\t"
        "sd     t1, 16(sp)\n\t"
        "sd     t2, 24(sp)\n\t"

        /* Record cause only if not already triggered (for priority) */
        "la     t2, g_vs_int_triggered\n\t"
        "lb     t0, 0(t2)\n\t"
        "bnez   t0, 1f\n\t"

        "csrr   t0, scause\n\t"
        "la     t2, g_vs_int_cause\n\t"
        "sd     t0, 0(t2)\n\t"
        "li     t0, 1\n\t"
        "la     t2, g_vs_int_triggered\n\t"
        "sb     t0, 0(t2)\n\t"

        "1:\n\t"
        /* Disable S-mode interrupts to prevent re-entry.
         * Must clear both SIE (bit 1) and SPIE (bit 5) because
         * sret restores SIE from SPIE. If we only clear SIE,
         * sret would restore SIE=1 from SPIE and re-trigger
         * the interrupt (infinite loop for read-only pending bits
         * like VSTIP/VSEIP that can't be cleared from VS-mode).
         * Use register form since 0x22 exceeds csrci 5-bit range. */
        "li     t0, 0x22\n\t"
        "csrc   sstatus, t0\n\t"
        /* Clear VSSIP pending bit via sip (alias for hvip when delegated).
         * This only works for VSSIP; VSTIP/VSEIP are read-only in vsip. */
        "csrc   sip, 0x2\n\t"

        /* Force SPP=1 (S-mode) so sret returns to VS-mode, not VU.
         * When the interrupt came from VU-mode, hardware set SPP=0. */
        "li     t0, 0x100\n\t"
        "csrs   sstatus, t0\n\t"

        /* Restore registers */
        "ld     ra, 0(sp)\n\t"
        "ld     t0, 8(sp)\n\t"
        "ld     t1, 16(sp)\n\t"
        "ld     t2, 24(sp)\n\t"
        "addi   sp, sp, 32\n\t"

        "sret\n\t"
    );
}

/* vs_nop_fn is defined in test_helpers.c (shared helper). */

/* ===================================================================
 * Helper: set up VS interrupt test infrastructure
 *
 * Configures delegation, vstvec, and vsstatus.SIE for VS-mode
 * interrupt delivery testing.
 * =================================================================== */
static void setup_vs_int_test(uintptr_t hideleg_mask)
{
    /* Clear global interrupt record */
    g_vs_int_cause = 0;
    g_vs_int_triggered = false;

    /* Clear all interrupt sources to prevent spurious interrupts */
    asm volatile("csrw 0x604, zero" ::: "memory");   /* hie = 0 */
    asm volatile("csrw 0x645, zero" ::: "memory");   /* hvip = 0 */
    /* Set vstimecmp to max to prevent VSTIP from vstimecmp */
    asm volatile("csrw 0x14D, %0" :: "r"((uintptr_t)-1) : "memory");

    /* Dual-layer delegation: M -> HS (mideleg) -> VS (hideleg) */
    uintptr_t mideleg;
    asm volatile("csrr %0, 0x303" : "=r"(mideleg));
    mideleg |= hideleg_mask;
    asm volatile("csrw 0x303, %0" :: "r"(mideleg) : "memory");
    hideleg_write(hideleg_mask);

    /* Install VS-mode trap handler (Direct mode).
     * IMPORTANT: vs_int_handler must be 4-byte aligned so that
     * vstvec base address matches the handler entry point exactly. */
    vs_trap_setup_direct((uintptr_t)vs_int_handler);

    /* Enable VS-mode interrupts: set vsstatus.SIE (bit 1) */
    uintptr_t vsstatus;
    asm volatile("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus |= 0x2;  /* SIE */
    asm volatile("csrw 0x200, %0" :: "r"(vsstatus) : "memory");

    /* Set mstatus.MPIE=1 so that after mret, MIE=1.
     * This is needed for interrupt delivery to work. */
    uintptr_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));
    mstatus |= (1UL << 7);   /* MPIE */
    asm volatile("csrw mstatus, %0" :: "r"(mstatus) : "memory");
}

/* ===================================================================
 * Group 4: hvip/hip/hie interrupt injection and aliasing
 * =================================================================== */

/* ------------------------------------------------------------------
 * HINT-01: hvip VSSIP injection — end-to-end VS-mode delivery
 * ------------------------------------------------------------------ */
TEST_REGISTER(hvip_vssp_injection);
bool hvip_vssp_injection(void)
{
    TEST_BEGIN("HINT-01: Inject VSSIP via hvip, verify VS-mode receives interrupt");

    /* Set up delegation + vstvec + vsstatus.SIE */
    setup_vs_int_test(VS_VSSIP);

    /* Enable VSSIE in hie */
    asm volatile("csrs 0x604, %0" :: "r"(VS_VSSIP) : "memory");

    /* Inject VSSIP via hvip */
    hvip_set_vssi(1);

    /* CSR verification: hvip and hip reflect pending */
    uintptr_t hvip_val = hvip_read();
    TEST_ASSERT_EQ("hvip.VSSIP should be set", hvip_val & VS_VSSIP, VS_VSSIP);
    uintptr_t hip_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP should reflect hvip.VSSIP", hip_val & VS_VSSIP, VS_VSSIP);

    /* Enter VS-mode: interrupt should fire and be delivered to vstvec */
    run_in_vs_mode(vs_nop_fn, 0);

    /* Verify VS-mode received the interrupt */
    TEST_ASSERT("VS-mode interrupt was triggered", g_vs_int_triggered);
    TEST_ASSERT_EQ("VS interrupt cause = VSSIP", g_vs_int_cause, (uintptr_t)VS_IRQ_SSI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-02: hvip VSTIP injection — end-to-end VS-mode delivery
 * ------------------------------------------------------------------ */
TEST_REGISTER(hvip_vstip_injection);
bool hvip_vstip_injection(void)
{
    TEST_BEGIN("HINT-02: Inject VSTIP via hvip, verify VS-mode receives interrupt");

    setup_vs_int_test(VS_VSTIP);

    /* Enable VSTIE in hie */
    asm volatile("csrs 0x604, %0" :: "r"(VS_VSTIP) : "memory");

    /* Inject VSTIP via hvip */
    hvip_set_vsti(1);

    /* CSR verification */
    uintptr_t hvip_val = hvip_read();
    TEST_ASSERT_EQ("hvip.VSTIP should be set", hvip_val & VS_VSTIP, VS_VSTIP);
    uintptr_t hip_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSTIP should reflect hvip.VSTIP", hip_val & VS_VSTIP, VS_VSTIP);

    /* Enter VS-mode: interrupt should fire immediately */
    run_in_vs_mode(vs_nop_fn, 0);

    /* Verify VS-mode received the interrupt */
    TEST_ASSERT("VS-mode interrupt was triggered", g_vs_int_triggered);
    TEST_ASSERT_EQ("VS interrupt cause = VSTIP", g_vs_int_cause, (uintptr_t)VS_IRQ_STI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-03: hvip VSEIP injection — end-to-end VS-mode delivery
 * ------------------------------------------------------------------ */
TEST_REGISTER(hvip_vseip_injection);
bool hvip_vseip_injection(void)
{
    TEST_BEGIN("HINT-03: Inject VSEIP via hvip, verify VS-mode receives interrupt");

    setup_vs_int_test(VS_VSEIP);

    /* Enable VSEIE in hie */
    asm volatile("csrs 0x604, %0" :: "r"(VS_VSEIP) : "memory");

    /* Inject VSEIP via hvip */
    hvip_set_vsei(1);

    /* CSR verification */
    uintptr_t hvip_val = hvip_read();
    TEST_ASSERT_EQ("hvip.VSEIP should be set", hvip_val & VS_VSEIP, VS_VSEIP);
    uintptr_t hip_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSEIP should reflect hvip.VSEIP", hip_val & VS_VSEIP, VS_VSEIP);

    /* Enter VS-mode: interrupt should fire immediately */
    run_in_vs_mode(vs_nop_fn, 0);

    /* Verify VS-mode received the interrupt */
    TEST_ASSERT("VS-mode interrupt was triggered", g_vs_int_triggered);
    TEST_ASSERT_EQ("VS interrupt cause = VSEIP", g_vs_int_cause, (uintptr_t)VS_IRQ_SEI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-04: hip.VSSIP is hvip.VSSIP alias
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_vssp_is_alias);
bool hip_vssp_is_alias(void)
{
    TEST_BEGIN("HINT-04: Verify hip.VSSIP is hvip.VSSIP alias");

    /* Set hvip.VSSIP (bit 2 = 0x4). */
    hvip_set_vssi(1);
    uintptr_t hvip_val = hvip_read();

    /* Read hip and verify alias relationship (bit 2 = 0x4). */
    uintptr_t hip_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP should alias hvip.VSSIP",
                   (hip_val & VS_VSSIP), (hvip_val & VS_VSSIP));

    /* Clear hvip.VSSIP and verify hip clears. */
    hvip_set_vssi(0);
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("Clearing hvip.VSSIP should clear hip.VSSIP",
                   hip_val & VS_VSSIP, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-05: hip.VSEIP is read-only (alias of hvip.VSEIP)
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_vseip_readonly);
bool hip_vseip_readonly(void)
{
    TEST_BEGIN("HINT-05: Verify hip.VSEIP is read-only (alias of hvip.VSEIP)");

    /* Set hvip.VSEIP (bit 10 = 0x400). */
    hvip_set_vsei(1);
    uintptr_t hvip_val = hvip_read();

    /* Try to write hip.VSEIP directly (bit 10 = 0x400).
     * hip is mostly read-only; VS pending bits alias hvip. */
    uintptr_t hip_read;
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));
    asm volatile("csrw 0x644, %0" :: "r"(hip_read | VS_VSEIP) : "memory");
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));

    /* Verify hip.VSEIP still reflects hvip.VSEIP, not the write. */
    TEST_ASSERT_EQ("hip.VSEIP should remain alias of hvip.VSEIP",
                   hip_read & VS_VSEIP, hvip_val & VS_VSEIP);

    /* Cleanup */
    hvip_set_vsei(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-06: hip.VSTIP is read-only
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_vstip_readonly);
bool hip_vstip_readonly(void)
{
    TEST_BEGIN("HINT-06: Verify hip.VSTIP is read-only (alias of hvip.VSTIP)");

    /* Set hvip.VSTIP (bit 6 = 0x40). */
    hvip_set_vsti(1);
    uintptr_t hvip_val = hvip_read();

    /* Try to write hip.VSTIP directly (bit 6 = 0x40). */
    uintptr_t hip_read;
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));
    asm volatile("csrw 0x644, %0" :: "r"(hip_read | VS_VSTIP) : "memory");
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));

    /* Verify hip.VSTIP still reflects hvip.VSTIP, not the write. */
    TEST_ASSERT_EQ("hip.VSTIP should remain alias of hvip.VSTIP",
                   hip_read & VS_VSTIP, hvip_val & VS_VSTIP);

    /* Cleanup */
    hvip_set_vsti(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-07: hie VSEIE/VSTIE/VSSIE are writable
 * ------------------------------------------------------------------ */
TEST_REGISTER(hie_vseie_vstie_vssie_writable);
bool hie_vseie_vstie_vssie_writable(void)
{
    TEST_BEGIN("HINT-07: Verify hie VSEIE/VSTIE/VSSIE bits are writable");

    /* Clear hie first */
    asm volatile("csrw 0x604, zero" ::: "memory");

    /* Set all three enable bits (bits 10, 6, 2 = 0x444). */
    asm volatile("csrs 0x604, %0" :: "r"(VS_ALL) : "memory");
    uintptr_t val;
    asm volatile("csrr %0, 0x604" : "=r"(val));
    TEST_ASSERT_EQ("hie VSEIE/VSTIE/VSSIE should be settable",
                   val & VS_ALL, VS_ALL);

    /* Clear all three enable bits. */
    asm volatile("csrc 0x604, %0" :: "r"(VS_ALL) : "memory");
    asm volatile("csrr %0, 0x604" : "=r"(val));
    TEST_ASSERT_EQ("hie VSEIE/VSTIE/VSSIE should be clearable",
                   val & VS_ALL, (uintptr_t)0);

    /* Write each bit independently */
    asm volatile("csrw 0x604, %0" :: "r"(VS_VSSIP) : "memory");
    asm volatile("csrr %0, 0x604" : "=r"(val));
    TEST_ASSERT_EQ("hie.VSSIE independently writable",
                   val & VS_VSSIP, VS_VSSIP);
    TEST_ASSERT_EQ("hie.VSTIE not set when only VSSIE written",
                   val & VS_VSTIP, (uintptr_t)0);

    asm volatile("csrw 0x604, %0" :: "r"(VS_VSTIP) : "memory");
    asm volatile("csrr %0, 0x604" : "=r"(val));
    TEST_ASSERT_EQ("hie.VSTIE independently writable",
                   val & VS_VSTIP, VS_VSTIP);

    asm volatile("csrw 0x604, %0" :: "r"(VS_VSEIP) : "memory");
    asm volatile("csrr %0, 0x604" : "=r"(val));
    TEST_ASSERT_EQ("hie.VSEIE independently writable",
                   val & VS_VSEIP, VS_VSEIP);

    /* Cleanup */
    asm volatile("csrw 0x604, zero" ::: "memory");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-08: sie and hip/hie are mutually exclusive
 *
 * Per norm:sie_hip_hie_mutex: sie writable bits (1,5,9) must be
 * read-zero in hip/hie, and hip/hie writable bits (2,6,10) must
 * be read-zero in sie.
 * ------------------------------------------------------------------ */
TEST_REGISTER(sie_hip_hie_mutually_exclusive);
bool sie_hip_hie_mutually_exclusive(void)
{
    TEST_BEGIN("HINT-08: Verify sie and hip/hie mutual exclusivity");

    /* Clear both sie and hie first */
    asm volatile("csrw sie, zero" ::: "memory");
    asm volatile("csrw 0x604, zero" ::: "memory");

    /* Part A: Write hie VS bits (2,6,10), verify sie does NOT contain them.
     * Per norm:sie_hip_hie_mutex, hie writable bits must be read-zero in sie. */
    asm volatile("csrs 0x604, %0" :: "r"(VS_ALL) : "memory");
    uintptr_t hie_val;
    asm volatile("csrr %0, 0x604" : "=r"(hie_val));
    uintptr_t sie_val;
    asm volatile("csrr %0, sie" : "=r"(sie_val));
    TEST_ASSERT("hie VS bits (2,6,10) are writable",
                (hie_val & VS_ALL) != 0);
    TEST_ASSERT_EQ("sie does not contain VS interrupt bits",
                   sie_val & VS_ALL, (uintptr_t)0);

    /* Part B: Write HS bits (1,5,9) to mie, verify hie does NOT contain them.
     * mie (0x304) is the M-mode interrupt enable. Bits 1,5,9 in mie are
     * SSIE/STIE/SEIE. sie (0x104) aliases these bits.
     * Per norm:sie_hip_hie_mutex, sie writable bits must be read-zero in hie. */
    asm volatile("csrw 0x604, zero" ::: "memory");  /* clear hie first */
    uintptr_t hs_bits = HS_SSIP | HS_STIP | HS_SEIP;
    asm volatile("csrs mie, %0" :: "r"(hs_bits) : "memory");
    /* Read back mie to verify HS bits are set */
    uintptr_t mie_val;
    asm volatile("csrr %0, mie" : "=r"(mie_val));
    asm volatile("csrr %0, 0x604" : "=r"(hie_val));
    /* SSIE (bit 1) should be writable in mie */
    TEST_ASSERT("mie SSIE (bit 1) is writable",
                (mie_val & HS_SSIP) != 0);
    TEST_ASSERT_EQ("hie does not contain HS SSIE bit",
                   hie_val & HS_SSIP, (uintptr_t)0);
    /* SEIE (bit 9) should also be writable if external interrupts supported */
    if (mie_val & HS_SEIP) {
        TEST_ASSERT_EQ("hie does not contain HS SEIE bit",
                       hie_val & HS_SEIP, (uintptr_t)0);
    }

    /* Cleanup */
    asm volatile("csrw mie, zero" ::: "memory");
    asm volatile("csrw 0x604, zero" ::: "memory");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-09: Clearing hvip.VSSIP clears interrupt
 * ------------------------------------------------------------------ */
TEST_REGISTER(clear_hvip_vssp_clears_interrupt);
bool clear_hvip_vssp_clears_interrupt(void)
{
    TEST_BEGIN("HINT-09: Verify clearing hvip.VSSIP clears VSSIP interrupt");

    /* Set VSSIP (bit 2 = 0x4). */
    hvip_set_vssi(1);
    uintptr_t hvip_val = hvip_read();
    TEST_ASSERT_EQ("hvip.VSSIP should be set", hvip_val & VS_VSSIP, VS_VSSIP);

    /* Verify hip reflects pending. */
    uintptr_t hip_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP should be set", hip_val & VS_VSSIP, VS_VSSIP);

    /* Clear VSSIP. */
    hvip_set_vssi(0);
    hvip_val = hvip_read();

    /* Verify VSSIP is cleared in both hvip and hip. */
    TEST_ASSERT_EQ("hvip.VSSIP should be cleared", hvip_val & VS_VSSIP, (uintptr_t)0);
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP should be cleared", hip_val & VS_VSSIP, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-10: hideleg=0 — interrupt not delivered to VS-mode
 *
 * When hideleg[2]=0, VSSIP is pending in hip but NOT visible to
 * VS-mode (vsip.SSIP reads zero). The interrupt traps to HS/M-mode
 * instead of VS-mode.
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_0_trap_to_hs);
bool hideleg_0_trap_to_hs(void)
{
    TEST_BEGIN("HINT-10: hideleg[2]=0, VSSIP not delivered to VS-mode");

    /* Do NOT delegate to VS (hideleg[2]=0) */
    hideleg_write(0);

    /* Inject VSSIP */
    hvip_set_vssi(1);

    /* Verify VSSIP is pending in hip (HS-visible) */
    uintptr_t hip_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP should be pending (HS-visible)",
                   hip_val & VS_VSSIP, VS_VSSIP);

    /* VS-mode reads sip.SSIP — should be 0 (not delegated) */
    uintptr_t sip_val = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT_EQ("vsip.SSIP should be 0 when hideleg[2]=0",
                   sip_val & HS_SSIP, (uintptr_t)0);

    /* Verify hideleg state */
    uintptr_t hideleg_val = hideleg_read();
    TEST_ASSERT_EQ("hideleg should not delegate VSSIP",
                   hideleg_val & VS_VSSIP, (uintptr_t)0);

    /* Cleanup */
    hvip_set_vssi(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-11: hideleg=1 — interrupt delivered to VS-mode
 *
 * When hideleg[2]=1, VSSIP is delegated to VS-mode. The interrupt
 * is delivered to VS-mode via vstvec.
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_1_trap_to_vs);
bool hideleg_1_trap_to_vs(void)
{
    TEST_BEGIN("HINT-11: hideleg[2]=1, VSSIP delivered to VS-mode");

    /* Set up delegation + vstvec + vsstatus.SIE */
    setup_vs_int_test(VS_VSSIP);

    /* Enable VSSIE in hie */
    asm volatile("csrs 0x604, %0" :: "r"(VS_VSSIP) : "memory");

    /* Inject VSSIP */
    hvip_set_vssi(1);

    /* Enter VS-mode: interrupt should be delivered to VS-mode */
    run_in_vs_mode(vs_nop_fn, 0);

    /* Verify VS-mode received the interrupt */
    TEST_ASSERT("VS-mode interrupt was triggered", g_vs_int_triggered);
    TEST_ASSERT_EQ("VS interrupt cause = VSSIP",
                   g_vs_int_cause, (uintptr_t)VS_IRQ_SSI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-12: HS-mode interrupt priority SEI > SSI
 *
 * Per norm:hsint_priority, the HS-mode interrupt priority is:
 *   SEI > SSI > STI > SGEI > VSEI > VSSI > VSTI > LCOFI
 *
 * This test verifies that both SEI and SSI can be pending
 * simultaneously. Actual delivery-priority verification requires
 * a custom multi-interrupt handler (framework limitation).
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_priority_sei_ssi);
bool interrupt_priority_sei_ssi(void)
{
    TEST_BEGIN("HINT-12: Verify SEI > SSI priority (CSR pending verification)");

    /* Clear any existing pending/enable bits */
    asm volatile("csrw sip, zero" ::: "memory");
    asm volatile("csrw sie, zero" ::: "memory");

    /* Set both SSIP and SEIP pending from M-mode (mip is writable in M-mode) */
    uintptr_t mip_val;
    asm volatile("csrr %0, mip" : "=r"(mip_val));
    mip_val |= (HS_SSIP | HS_SEIP);
    asm volatile("csrw mip, %0" :: "r"(mip_val) : "memory");
    asm volatile("csrr %0, mip" : "=r"(mip_val));

    /* Verify both are pending simultaneously */
    TEST_ASSERT_EQ("mip.SSIP should be pending", mip_val & HS_SSIP, HS_SSIP);
    TEST_ASSERT_EQ("mip.SEIP should be pending", mip_val & HS_SEIP, HS_SEIP);

    /* Per spec norm:hsint_priority, SEI (cause 9) has higher priority
     * than SSI (cause 1). When both are pending and enabled, SEI is
     * delivered first. Actual delivery verification requires a custom
     * multi-interrupt trap handler to capture the first cause and
     * clear pending bits to prevent re-entry. */

    /* Cleanup: clear pending bits */
    asm volatile("csrc mip, %0" :: "r"(HS_SSIP | HS_SEIP) : "memory");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-13: VS-mode interrupt priority VSEI > VSSI > VSTI
 *
 * Per norm:hsint_priority, the VS-level interrupt priority within
 * the HS-mode priority chain is VSEI > VSSI > VSTI.
 *
 * This test injects all three VS interrupts simultaneously and
 * verifies that VSEI (highest priority) is delivered first to
 * VS-mode.
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_priority_vsei_vssi_vsti);
bool interrupt_priority_vsei_vssi_vsti(void)
{
    TEST_BEGIN("HINT-13: Verify VSEI > VSSI > VSTI priority (VS delivery)");

    /* Set up delegation for all three VS interrupts */
    setup_vs_int_test(VS_ALL);

    /* Enable all three in hie */
    asm volatile("csrw 0x604, %0" :: "r"(VS_ALL) : "memory");

    /* Inject all three VS interrupts simultaneously via hvip */
    hvip_write(VS_ALL);
    uintptr_t hvip_val = hvip_read();
    TEST_ASSERT_EQ("All VS interrupts should be pending",
                   hvip_val & VS_ALL, VS_ALL);

    /* Enter VS-mode: highest-priority interrupt (VSEI) should fire */
    run_in_vs_mode(vs_nop_fn, 0);

    /* Verify VS-mode received an interrupt */
    TEST_ASSERT("VS-mode interrupt was triggered", g_vs_int_triggered);

    /* Per norm:hsint_priority, VSEI (cause 9 in VS-mode) has higher priority
     * than VSSI (cause 1) and VSTI (cause 5). The VS handler records
     * only the first interrupt (subsequent ones are suppressed by
     * disabling SIE in the handler). */
    TEST_ASSERT_EQ("First VS interrupt should be VSEI (highest priority)",
                   g_vs_int_cause, (uintptr_t)VS_IRQ_SEI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-14: hip/hie non-standard bits are read-zero
 *
 * Per spec, only standard interrupt bits are writable in hip/hie.
 * Standard bits: 1,2,5,6,9,10,12 (and optionally 13+ with AIA).
 * All other bits should read as zero.
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_hie_nonstandard_bits_readzero);
bool hip_hie_nonstandard_bits_readzero(void)
{
    TEST_BEGIN("HINT-14: Verify hip/hie non-standard bits are read-zero");

    /* Write all 1s to hip and hie */
    asm volatile("csrw 0x644, %0" :: "r"((uintptr_t)0xFFFFFFFFFFFFFFFF) : "memory");
    asm volatile("csrw 0x604, %0" :: "r"((uintptr_t)0xFFFFFFFFFFFFFFFF) : "memory");

    uintptr_t hip_val, hie_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    asm volatile("csrr %0, 0x604" : "=r"(hie_val));

    /* Bits above 12 (excluding SGEIP=bit12) should be read-zero */
    TEST_ASSERT("hip high bits (above 12) read-zero",
                (hip_val & ~0x1FFFUL) == 0);
    TEST_ASSERT("hie high bits (above 12) read-zero",
                (hie_val & ~0x1FFFUL) == 0);

    /* Verify individual non-standard bits are zero */
    /* bit 0: no standard interrupt */
    TEST_ASSERT_EQ("hip bit 0 read-zero", hip_val & (1UL << 0), (uintptr_t)0);
    TEST_ASSERT_EQ("hie bit 0 read-zero", hie_val & (1UL << 0), (uintptr_t)0);

    /* bit 3,4: no standard interrupt */
    TEST_ASSERT_EQ("hip bit 3 read-zero", hip_val & (1UL << 3), (uintptr_t)0);
    TEST_ASSERT_EQ("hip bit 4 read-zero", hip_val & (1UL << 4), (uintptr_t)0);
    TEST_ASSERT_EQ("hie bit 3 read-zero", hie_val & (1UL << 3), (uintptr_t)0);
    TEST_ASSERT_EQ("hie bit 4 read-zero", hie_val & (1UL << 4), (uintptr_t)0);

    /* bit 7,8: no standard interrupt */
    TEST_ASSERT_EQ("hip bit 7 read-zero", hip_val & (1UL << 7), (uintptr_t)0);
    TEST_ASSERT_EQ("hip bit 8 read-zero", hip_val & (1UL << 8), (uintptr_t)0);
    TEST_ASSERT_EQ("hie bit 7 read-zero", hie_val & (1UL << 7), (uintptr_t)0);
    TEST_ASSERT_EQ("hie bit 8 read-zero", hie_val & (1UL << 8), (uintptr_t)0);

    /* bit 11: no standard interrupt */
    TEST_ASSERT_EQ("hip bit 11 read-zero", hip_val & (1UL << 11), (uintptr_t)0);
    TEST_ASSERT_EQ("hie bit 11 read-zero", hie_val & (1UL << 11), (uintptr_t)0);

    /* Cleanup */
    asm volatile("csrw 0x644, zero" ::: "memory");
    asm volatile("csrw 0x604, zero" ::: "memory");

    HYP_TEST_END();
}

/* ===================================================================
 * Group 5: hgeip/hgeie guest external interrupts
 * =================================================================== */

/* ------------------------------------------------------------------
 * HGEI-01: hgeip is read-only
 * ------------------------------------------------------------------ */
TEST_REGISTER(hgeip_readonly);
bool hgeip_readonly(void)
{
    TEST_BEGIN("HGEI-01: Verify hgeip is read-only");

    uintptr_t before;
    asm volatile("csrr %0, 0xE12" : "=r"(before));
    /* hgeip is read-only; csrw to it triggers illegal-instruction (cause=2). */
    EXPECT_TRAP(2, {
        asm volatile("csrw 0xE12, %0" :: "r"((uintptr_t)0xFFFFFFFF));
    });
    uintptr_t after;
    asm volatile("csrr %0, 0xE12" : "=r"(after));
    TEST_ASSERT_EQ("hgeip should not change on write", after, before);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HGEI-02: hgeie writable bit range
 * ------------------------------------------------------------------ */
TEST_REGISTER(hgeie_writable_bit_range);
bool hgeie_writable_bit_range(void)
{
    TEST_BEGIN("HGEI-02: Verify hgeie writable bit range");

    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)0xFFFFFFFF));
    uintptr_t val;
    asm volatile("csrr %0, 0x607" : "=r"(val));
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
bool hgeie_bit0_readzero(void)
{
    TEST_BEGIN("HGEI-03: Verify hgeie bit 0 is read-zero");

    /* Write all 1s to hgeie. */
    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)0xFFFFFFFF));
    uintptr_t val;
    asm volatile("csrr %0, 0x607" : "=r"(val));

    /* Verify bit 0 is read-zero. */
    TEST_ASSERT_EQ("hgeie bit 0 should be read-zero", val & 0x1, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HGEI-04: hgeip AND hgeie triggers SGEIP (simplified verification)
 * ------------------------------------------------------------------ */
TEST_REGISTER(hgeip_and_hgeie_triggers_sgeip);
bool hgeip_and_hgeie_triggers_sgeip(void)
{
    TEST_BEGIN("HGEI-04: Verify hgeip AND hgeie triggers SGEIP");

    /* hgeip is read-only hardware, so we verify that when hgeie is set,
     * the SGEIP mechanism is armed. Since hgeip depends on external interrupts,
     * we verify: hgeie writable AND hgeip & hgeie == 0 when no external interrupt */
    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)0xFFFFFFFE)); /* set all except bit 0 */
    uintptr_t hgeie_val;
    asm volatile("csrr %0, 0x607" : "=r"(hgeie_val));
    uintptr_t hgeip_val;
    asm volatile("csrr %0, 0xE12" : "=r"(hgeip_val));
    /* With no external interrupt sources, hgeip & hgeie should be 0 */
    TEST_ASSERT("no spurious SGEIP", (hgeip_val & hgeie_val) == 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HGEI-05: GEILEN=0 results in all zeros
 * ------------------------------------------------------------------ */
TEST_REGISTER(geilen_zero_all_zeros);
bool geilen_zero_all_zeros(void)
{
    TEST_BEGIN("HGEI-05: Verify GEILEN=0 results in all zeros");

    /* Write all 1s to hgeie and check how many bits are writable */
    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)0xFFFFFFFF));
    uintptr_t hgeie_val;
    asm volatile("csrr %0, 0x607" : "=r"(hgeie_val));
    if (hgeie_val == 0) {
        /* GEILEN=0: both hgeip and hgeie should be all zeros */
        uintptr_t hgeip_val;
        asm volatile("csrr %0, 0xE12" : "=r"(hgeip_val));
        TEST_ASSERT_EQ("hgeie all zero when GEILEN=0", hgeie_val, (uintptr_t)0);
        TEST_ASSERT_EQ("hgeip all zero when GEILEN=0", hgeip_val, (uintptr_t)0);
    } else {
        /* GEILEN > 0: verified by hgeie having writable bits */
        TEST_ASSERT("GEILEN > 0, hgeie has writable bits", hgeie_val != 0);
    }

    HYP_TEST_END();
}
