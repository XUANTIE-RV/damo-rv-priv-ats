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
 * HS-mode interrupt handler
 *
 * This handler is installed at stvec for tests that need to verify
 * interrupts actually delivered to HS-mode (HINT-10, HINT-12).
 * It records the first interrupt cause, clears pending sources to
 * prevent re-entry, disables SIE+SPIE, and returns via sret.
 * =================================================================== */

static volatile uintptr_t g_hs_int_cause;
static volatile bool g_hs_int_triggered;

static void hs_int_handler(void) __attribute__((naked, aligned(4)));
static void hs_int_handler(void)
{
    asm volatile (
        "csrr   t0, scause\n\t"
        "la     t1, g_hs_int_cause\n\t"
        "sd     t0, 0(t1)\n\t"
        "li     t0, 1\n\t"
        "la     t1, g_hs_int_triggered\n\t"
        "sb     t0, 0(t1)\n\t"
        "li     t0, 0x22\n\t"
        "csrc   sstatus, t0\n\t"
        "csrw   sie, zero\n\t"
        "sret\n\t"
    );
}

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
 * HINT-04: hip.VSSIP is bidirectional writable alias of hvip.VSSIP
 *
 * Per norm:hip_vssip_vssie_op: "VSSIP in hip is an alias (writable)
 * of the same bit in hvip." This means:
 *   - Read direction: hvip.VSSIP=1 -> hip.VSSIP reads 1
 *   - Write direction: writing hip.VSSIP=1 -> hvip.VSSIP becomes 1
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_vssp_is_alias);
bool hip_vssp_is_alias(void)
{
    TEST_BEGIN("HINT-04: Verify hip.VSSIP is bidirectional writable alias");

    /* --- Part A: Read direction (hvip -> hip) --- */
    hvip_set_vssi(1);
    uintptr_t hip_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP=1 when hvip.VSSIP=1 (read direction)",
                   hip_val & VS_VSSIP, VS_VSSIP);

    hvip_set_vssi(0);
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP=0 when hvip.VSSIP=0 (read direction)",
                   hip_val & VS_VSSIP, (uintptr_t)0);

    /* --- Part B: Write direction (hip -> hvip) --- */
    /* Ensure hvip.VSSIP starts at 0 */
    hvip_set_vssi(0);
    uintptr_t hvip_val = hvip_read();
    TEST_ASSERT_EQ("hvip.VSSIP initially 0", hvip_val & VS_VSSIP, (uintptr_t)0);

    /* Write hip.VSSIP=1 directly (bit 2 = 0x4) */
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    asm volatile("csrw 0x644, %0" :: "r"(hip_val | VS_VSSIP) : "memory");

    /* Verify hvip.VSSIP became 1 (writable alias) */
    hvip_val = hvip_read();
    TEST_ASSERT_EQ("hvip.VSSIP=1 after writing hip.VSSIP=1 (write direction)",
                   hvip_val & VS_VSSIP, VS_VSSIP);

    /* Write hip.VSSIP=0 directly */
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    asm volatile("csrw 0x644, %0" :: "r"(hip_val & ~VS_VSSIP) : "memory");

    /* Verify hvip.VSSIP became 0 */
    hvip_val = hvip_read();
    TEST_ASSERT_EQ("hvip.VSSIP=0 after writing hip.VSSIP=0 (write direction)",
                   hvip_val & VS_VSSIP, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-05: hip.VSEIP is read-only (multi-source OR)
 *
 * Per norm:hip_vseip_vseie_op: "VSEIP is read-only in hip, and is
 * the logical-OR of: hvip.VSEIP, hgeip[VGEIN], platform signal."
 * Verify with two combinations:
 *   Combo 1: hvip.VSEIP=1, attempt to clear hip.VSEIP -> stays 1
 *   Combo 2: hvip.VSEIP=0, attempt to set hip.VSEIP -> stays 0
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_vseip_readonly);
bool hip_vseip_readonly(void)
{
    TEST_BEGIN("HINT-05: Verify hip.VSEIP is read-only (two combos)");

    /* --- Combo 1: hvip.VSEIP=1, try to write hip.VSEIP=0 --- */
    hvip_set_vsei(1);
    uintptr_t hip_read;
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));
    TEST_ASSERT_EQ("hip.VSEIP=1 when hvip.VSEIP=1",
                   hip_read & VS_VSEIP, VS_VSEIP);

    /* Attempt to clear hip.VSEIP by writing 0 to bit 10 */
    asm volatile("csrw 0x644, %0" :: "r"(hip_read & ~VS_VSEIP) : "memory");
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));
    TEST_ASSERT_EQ("hip.VSEIP stays 1 after write-0 (read-only)",
                   hip_read & VS_VSEIP, VS_VSEIP);

    /* --- Combo 2: hvip.VSEIP=0, try to write hip.VSEIP=1 --- */
    hvip_set_vsei(0);
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));
    TEST_ASSERT_EQ("hip.VSEIP=0 when hvip.VSEIP=0",
                   hip_read & VS_VSEIP, (uintptr_t)0);

    /* Attempt to set hip.VSEIP by writing 1 to bit 10 */
    asm volatile("csrw 0x644, %0" :: "r"(hip_read | VS_VSEIP) : "memory");
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));
    TEST_ASSERT_EQ("hip.VSEIP stays 0 after write-1 (read-only)",
                   hip_read & VS_VSEIP, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-06: hip.VSTIP is read-only
 *
 * Per norm:hip_vstip_vstie_acc_op: "VSTIP is read-only in hip, and
 * is the logical-OR of hvip.VSTIP and vstimecmp trigger."
 * Verify with two combinations:
 *   Combo 1: hvip.VSTIP=1, attempt to clear hip.VSTIP -> stays 1
 *   Combo 2: hvip.VSTIP=0, attempt to set hip.VSTIP -> stays 0
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_vstip_readonly);
bool hip_vstip_readonly(void)
{
    TEST_BEGIN("HINT-06: Verify hip.VSTIP is read-only (two combos)");

    /* Prevent vstimecmp from interfering: set to max.
     * vstimecmp is CSR 0x24D (not stimecmp 0x14D). */
    asm volatile("csrw 0x24D, %0" :: "r"((uintptr_t)-1) : "memory");

    /* --- Combo 1: hvip.VSTIP=1, try to write hip.VSTIP=0 --- */
    hvip_set_vsti(1);
    uintptr_t hip_read;
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));
    TEST_ASSERT_EQ("hip.VSTIP=1 when hvip.VSTIP=1",
                   hip_read & VS_VSTIP, VS_VSTIP);

    /* Attempt to clear hip.VSTIP by writing 0 to bit 6 */
    asm volatile("csrw 0x644, %0" :: "r"(hip_read & ~VS_VSTIP) : "memory");
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));
    TEST_ASSERT_EQ("hip.VSTIP stays 1 after write-0 (read-only)",
                   hip_read & VS_VSTIP, VS_VSTIP);

    /* --- Combo 2: hvip.VSTIP=0, try to write hip.VSTIP=1 --- */
    hvip_set_vsti(0);
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));
    TEST_ASSERT_EQ("hip.VSTIP=0 when hvip.VSTIP=0",
                   hip_read & VS_VSTIP, (uintptr_t)0);

    /* Attempt to set hip.VSTIP by writing 1 to bit 6 */
    asm volatile("csrw 0x644, %0" :: "r"(hip_read | VS_VSTIP) : "memory");
    asm volatile("csrr %0, 0x644" : "=r"(hip_read));
    TEST_ASSERT_EQ("hip.VSTIP stays 0 after write-1 (read-only)",
                   hip_read & VS_VSTIP, (uintptr_t)0);

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
 * HINT-10: hideleg=0 — interrupt traps to HS-mode
 *
 * When hideleg[2]=0, VSSIP is pending in hip and hie.VSSIE=1.
 * Per norm:hideleg_hs, since current mode is HS and sstatus.SIE=1,
 * and bit 2 is set in both hip and hie, and bit 2 is NOT set in
 * hideleg, the interrupt traps to HS-mode (stvec).
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_0_trap_to_hs);
bool hideleg_0_trap_to_hs(void)
{
    TEST_BEGIN("HINT-10: hideleg[2]=0, VSSIP traps to HS-mode");

    /* Reset HS interrupt record */
    g_hs_int_cause = 0;
    g_hs_int_triggered = false;

    /* Clear all interrupt sources */
    asm volatile("csrw 0x604, zero" ::: "memory");   /* hie = 0 */
    asm volatile("csrw 0x645, zero" ::: "memory");   /* hvip = 0 */
    asm volatile("csrw mip, zero" ::: "memory");

    /* Do NOT delegate to VS (hideleg[2]=0) */
    hideleg_write(0);

    /* Delegate S-level interrupts to HS-mode via mideleg.
     * This ensures the interrupt routes to stvec (not mtvec). */
    uintptr_t mideleg_val;
    asm volatile("csrr %0, 0x303" : "=r"(mideleg_val));
    mideleg_val |= VS_VSSIP;  /* bit 2 */
    asm volatile("csrw 0x303, %0" :: "r"(mideleg_val) : "memory");

    /* Install custom HS-mode interrupt handler at stvec */
    uintptr_t saved_stvec;
    asm volatile("csrr %0, stvec" : "=r"(saved_stvec));
    asm volatile("csrw stvec, %0" :: "r"((uintptr_t)hs_int_handler) : "memory");

    /* Enable VSSIE in hie (bit 2) */
    asm volatile("csrs 0x604, %0" :: "r"(VS_VSSIP) : "memory");

    /* Inject VSSIP via hvip */
    hvip_set_vssi(1);

    /* Verify VSSIP is pending in hip (HS-visible) */
    uintptr_t hip_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP should be pending",
                   hip_val & VS_VSSIP, VS_VSSIP);

    /* Verify hie.VSSIE is enabled */
    uintptr_t hie_val;
    asm volatile("csrr %0, 0x604" : "=r"(hie_val));
    TEST_ASSERT_EQ("hie.VSSIE should be enabled",
                   hie_val & VS_VSSIP, VS_VSSIP);

    /* Verify hideleg[2]=0 (not delegated to VS) */
    uintptr_t hideleg_val = hideleg_read();
    TEST_ASSERT_EQ("hideleg[2] should be 0",
                   hideleg_val & VS_VSSIP, (uintptr_t)0);

    /* Per norm:hideleg_hs, with hip.VSSIP=1, hie.VSSIE=1, hideleg[2]=0,
     * and sstatus.SIE=1 in HS-mode, the interrupt traps to HS-mode.
     * Enter HS-mode then enable SIE to trigger the interrupt. */
    goto_priv(PRIV_S);
    /* Now in HS-mode (V=0) with SIE=0. Enable SIE to trigger interrupt. */
    asm volatile("csrsi sstatus, 0x2" ::: "memory");  /* SIE=1 */
    /* Interrupt should fire here; handler disables SIE and returns */
    goto_priv(PRIV_M);

    /* Verify HS-mode received the interrupt */
    TEST_ASSERT("HS-mode interrupt was triggered", g_hs_int_triggered);
    TEST_ASSERT_EQ("HS interrupt cause = VSSI (cause 2)",
                   g_hs_int_cause, (uintptr_t)(CAUSE_INTERRUPT_BIT | 2));

    /* Cleanup */
    hvip_set_vssi(0);
    asm volatile("csrw 0x604, zero" ::: "memory");
    mideleg_val &= ~VS_VSSIP;
    asm volatile("csrw 0x303, %0" :: "r"(mideleg_val) : "memory");
    asm volatile("csrw stvec, %0" :: "r"(saved_stvec) : "memory");

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
 * This test sets both SEI and SSI pending and enabled simultaneously,
 * then enters HS-mode with SIE=1. The first interrupt delivered must
 * be SEI (cause 9), proving higher priority than SSI (cause 1).
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_priority_sei_ssi);
bool interrupt_priority_sei_ssi(void)
{
    TEST_BEGIN("HINT-12: Verify SEI > SSI priority (actual delivery)");

    /* Reset HS interrupt record */
    g_hs_int_cause = 0;
    g_hs_int_triggered = false;

    /* Clear all interrupt sources */
    asm volatile("csrw mip, zero" ::: "memory");
    asm volatile("csrw sie, zero" ::: "memory");
    asm volatile("csrw 0x604, zero" ::: "memory");   /* hie = 0 */
    asm volatile("csrw 0x645, zero" ::: "memory");   /* hvip = 0 */

    /* Delegate S-level interrupts to HS-mode (mideleg bits 1,9) */
    uintptr_t mideleg_val;
    asm volatile("csrr %0, 0x303" : "=r"(mideleg_val));
    mideleg_val |= (HS_SSIP | HS_SEIP);
    asm volatile("csrw 0x303, %0" :: "r"(mideleg_val) : "memory");

    /* Install custom HS-mode interrupt handler at stvec */
    uintptr_t saved_stvec;
    asm volatile("csrr %0, stvec" : "=r"(saved_stvec));
    asm volatile("csrw stvec, %0" :: "r"((uintptr_t)hs_int_handler) : "memory");

    /* Enable both SSIE and SEIE in sie */
    asm volatile("csrs 0x104, %0" :: "r"(HS_SSIP | HS_SEIP) : "memory");

    /* Set both SSIP and SEIP pending via mip (M-mode writable) */
    asm volatile("csrs mip, %0" :: "r"(HS_SSIP | HS_SEIP) : "memory");

    /* Verify both are pending simultaneously */
    uintptr_t mip_val;
    asm volatile("csrr %0, mip" : "=r"(mip_val));
    TEST_ASSERT_EQ("mip.SSIP should be pending", mip_val & HS_SSIP, HS_SSIP);
    TEST_ASSERT_EQ("mip.SEIP should be pending", mip_val & HS_SEIP, HS_SEIP);

    /* Verify both are enabled in sie */
    uintptr_t sie_val;
    asm volatile("csrr %0, 0x104" : "=r"(sie_val));
    TEST_ASSERT_EQ("sie.SSIE should be enabled", sie_val & HS_SSIP, HS_SSIP);
    TEST_ASSERT_EQ("sie.SEIE should be enabled", sie_val & HS_SEIP, HS_SEIP);

    /* Per norm:hsint_priority, when both SEI and SSI are pending and
     * enabled, SEI (cause 9) must be delivered first. Enter HS-mode
     * then enable SIE to trigger the interrupt. */
    goto_priv(PRIV_S);
    /* Now in HS-mode with SIE=0. Enable SIE to trigger interrupt. */
    asm volatile("csrsi sstatus, 0x2" ::: "memory");  /* SIE=1 */
    /* SEI fires first (higher priority); handler disables SIE */
    goto_priv(PRIV_M);

    /* Verify HS-mode received the interrupt */
    TEST_ASSERT("HS-mode interrupt was triggered", g_hs_int_triggered);
    TEST_ASSERT_EQ("First HS interrupt should be SEI (highest priority)",
                   g_hs_int_cause,
                   (uintptr_t)(CAUSE_INTERRUPT_BIT | IRQ_S_EXTERNAL));

    /* Cleanup */
    asm volatile("csrw mip, zero" ::: "memory");
    asm volatile("csrw 0x104, zero" ::: "memory");
    asm volatile("csrw stvec, %0" :: "r"(saved_stvec) : "memory");
    mideleg_val &= ~(HS_SSIP | HS_SEIP);
    asm volatile("csrw 0x303, %0" :: "r"(mideleg_val) : "memory");

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

/* ------------------------------------------------------------------
 * HINT-15: sstatus.SIE=0 masks interrupt in HS-mode
 *
 * Per norm:hideleg_hs condition (a): interrupt traps to HS-mode only
 * when "the current operating mode is HS-mode and the SIE bit in the
 * sstatus register is set". With SIE=0, the interrupt must NOT be
 * delivered even if pending and enabled.
 * ------------------------------------------------------------------ */
TEST_REGISTER(sie_zero_masks_interrupt);
bool sie_zero_masks_interrupt(void)
{
    TEST_BEGIN("HINT-15: SIE=0 masks interrupt in HS-mode");

    /* Reset HS interrupt record */
    g_hs_int_cause = 0;
    g_hs_int_triggered = false;

    /* Clear all interrupt sources */
    asm volatile("csrw 0x604, zero" ::: "memory");   /* hie = 0 */
    asm volatile("csrw 0x645, zero" ::: "memory");   /* hvip = 0 */

    /* Setup: hideleg[2]=0, inject VSSIP, enable hie.VSSIE */
    hideleg_write(0);
    uintptr_t mideleg_val;
    asm volatile("csrr %0, 0x303" : "=r"(mideleg_val));
    mideleg_val |= VS_VSSIP;
    asm volatile("csrw 0x303, %0" :: "r"(mideleg_val) : "memory");

    uintptr_t saved_stvec;
    asm volatile("csrr %0, stvec" : "=r"(saved_stvec));
    asm volatile("csrw stvec, %0" :: "r"((uintptr_t)hs_int_handler) : "memory");

    asm volatile("csrs 0x604, %0" :: "r"(VS_VSSIP) : "memory");
    hvip_set_vssi(1);

    /* --- Part A: Enter HS-mode with SIE=0 --- */
    goto_priv(PRIV_S);
    /* SIE=0 (default after goto_priv). Interrupt should NOT fire. */
    asm volatile("nop" ::: "memory");
    asm volatile("nop" ::: "memory");
    goto_priv(PRIV_M);

    TEST_ASSERT("SIE=0: interrupt NOT triggered", !g_hs_int_triggered);

    /* --- Part B: Enter HS-mode, then set SIE=1 --- */
    goto_priv(PRIV_S);
    asm volatile("csrsi sstatus, 0x2" ::: "memory");  /* SIE=1 */
    /* Interrupt fires immediately; handler records cause */
    goto_priv(PRIV_M);

    TEST_ASSERT("SIE=1: interrupt triggered", g_hs_int_triggered);
    TEST_ASSERT_EQ("cause = VSSI (2)",
                   g_hs_int_cause, (uintptr_t)(CAUSE_INTERRUPT_BIT | 2));

    /* Cleanup */
    hvip_set_vssi(0);
    asm volatile("csrw 0x604, zero" ::: "memory");
    asm volatile("csrw stvec, %0" :: "r"(saved_stvec) : "memory");
    mideleg_val &= ~VS_VSSIP;
    asm volatile("csrw 0x303, %0" :: "r"(mideleg_val) : "memory");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-16: hvip non-writable bits are read-only zeros
 *
 * Per norm:hvip_sz_op: "Bits of hvip that are not writable are
 * read-only zeros." Per norm:hvip_acc: VSEIP (bit 10), VSTIP (bit 6),
 * VSSIP (bit 2) are writable. With AIA, additional bits (13+) may
 * be writable for virtual interrupts.
 * ------------------------------------------------------------------ */
TEST_REGISTER(hvip_nonwritable_readzero);
bool hvip_nonwritable_readzero(void)
{
    TEST_BEGIN("HINT-16: Verify hvip non-writable bits are read-only zero");

    /* Write all 1s to hvip */
    hvip_write((uintptr_t)-1);
    uintptr_t val = hvip_read();

    /* Standard writable bits 2, 6, 10 must be set */
    uintptr_t std_bits = VS_VSSIP | VS_VSTIP | VS_VSEIP;  /* 0x444 */
    TEST_ASSERT_EQ("hvip standard bits 2/6/10 writable",
                   val & std_bits, std_bits);

    /* Bits 0, 1, 3, 4, 5, 7, 8, 9, 11, 12 have no standard interrupt
     * and must be read-only zero (below AIA range). */
    uintptr_t non_std_low = 0x1FFF & ~std_bits;  /* bits 12:0 except 2,6,10 */
    TEST_ASSERT_EQ("hvip bits 12:0 non-standard are read-zero",
                   val & non_std_low, (uintptr_t)0);

    /* Bits above 12 depend on AIA extension (implementation-specific).
     * Report for informational purposes. */
    uintptr_t high_bits = val & ~0x1FFFUL;
    if (high_bits != 0) {
        printf("  [INFO] hvip has AIA/impl-specific writable bits: "
               "0x%lx\n", (unsigned long)high_bits);
    }

    /* Cleanup */
    hvip_write(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-17: hip.VSTIP remains defined while V=0
 *
 * Per norm:hip_vstip_vstie_acc_op: "The hip[vstip] bit remains
 * defined while V=0 as well as V=1." This means hip.VSTIP must
 * correctly reflect hvip.VSTIP when accessed from HS-mode (V=0).
 * ------------------------------------------------------------------ */
TEST_REGISTER(hip_vstip_defined_v0);
bool hip_vstip_defined_v0(void)
{
    TEST_BEGIN("HINT-17: Verify hip.VSTIP defined at V=0");

    /* Prevent vstimecmp from interfering (CSR 0x24D) */
    asm volatile("csrw 0x24D, %0" :: "r"((uintptr_t)-1) : "memory");

    /* Confirm we are in M-mode (V=0 context for hip access) */

    /* Set hvip.VSTIP=1, verify hip.VSTIP=1 at V=0 */
    hvip_set_vsti(1);
    uintptr_t hip_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSTIP=1 at V=0 when hvip.VSTIP=1",
                   hip_val & VS_VSTIP, VS_VSTIP);

    /* Clear hvip.VSTIP=0, verify hip.VSTIP=0 at V=0 */
    hvip_set_vsti(0);
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSTIP=0 at V=0 when hvip.VSTIP=0",
                   hip_val & VS_VSTIP, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HINT-18: HS-mode interrupt priority SSI > STI
 *
 * Per norm:hsint_priority: SEI > SSI > STI > SGEI > VSEI > VSSI >
 * VSTI > LCOFI. This test verifies SSI has higher priority than STI
 * by setting both pending simultaneously and checking SSI is
 * delivered first.
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_priority_ssi_sti);
bool interrupt_priority_ssi_sti(void)
{
    TEST_BEGIN("HINT-18: Verify SSI > STI priority (actual delivery)");

    /* Reset HS interrupt record */
    g_hs_int_cause = 0;
    g_hs_int_triggered = false;

    /* Clear all interrupt sources */
    asm volatile("csrw mip, zero" ::: "memory");
    asm volatile("csrw 0x104, zero" ::: "memory");  /* sie = 0 */
    asm volatile("csrw 0x604, zero" ::: "memory");  /* hie = 0 */
    asm volatile("csrw 0x645, zero" ::: "memory");  /* hvip = 0 */

    /* Delegate S-level interrupts to HS-mode */
    uintptr_t mideleg_val;
    asm volatile("csrr %0, 0x303" : "=r"(mideleg_val));
    mideleg_val |= (HS_SSIP | HS_STIP);
    asm volatile("csrw 0x303, %0" :: "r"(mideleg_val) : "memory");

    /* Install HS-mode interrupt handler */
    uintptr_t saved_stvec;
    asm volatile("csrr %0, stvec" : "=r"(saved_stvec));
    asm volatile("csrw stvec, %0" :: "r"((uintptr_t)hs_int_handler) : "memory");

    /* Enable SSIE and STIE in sie */
    asm volatile("csrs 0x104, %0" :: "r"(HS_SSIP | HS_STIP) : "memory");

    /* Set SSIP pending (writable in mip) */
    asm volatile("csrs mip, %0" :: "r"(HS_SSIP) : "memory");

    /* Trigger STIP via stimecmp: set stimecmp=0 so time >= stimecmp.
     * stimecmp is CSR 0x14D. Requires menvcfg.STCE=1 (Sstc). */
    uintptr_t saved_stimecmp;
    asm volatile("csrr %0, 0x14D" : "=r"(saved_stimecmp));
    /* Enable STCE in menvcfg (bit 63) */
    uintptr_t menvcfg_val;
    asm volatile("csrr %0, 0x30A" : "=r"(menvcfg_val));
    menvcfg_val |= (1UL << 63);
    asm volatile("csrw 0x30A, %0" :: "r"(menvcfg_val) : "memory");
    asm volatile("csrw 0x14D, zero" ::: "memory");

    /* Verify both are pending */
    uintptr_t mip_val;
    asm volatile("csrr %0, mip" : "=r"(mip_val));
    TEST_ASSERT("mip.SSIP pending", (mip_val & HS_SSIP) != 0);
    TEST_ASSERT("mip.STIP pending (via stimecmp)", (mip_val & HS_STIP) != 0);

    /* Enter HS-mode and enable SIE */
    goto_priv(PRIV_S);
    asm volatile("csrsi sstatus, 0x2" ::: "memory");  /* SIE=1 */
    /* SSI fires first (higher priority than STI) */
    goto_priv(PRIV_M);

    /* Verify */
    TEST_ASSERT("HS-mode interrupt was triggered", g_hs_int_triggered);
    TEST_ASSERT_EQ("First interrupt = SSI (cause 1, priority > STI)",
                   g_hs_int_cause,
                   (uintptr_t)(CAUSE_INTERRUPT_BIT | IRQ_S_SOFTWARE));

    /* Cleanup */
    asm volatile("csrw mip, zero" ::: "memory");
    asm volatile("csrw 0x104, zero" ::: "memory");
    asm volatile("csrw 0x14D, %0" :: "r"(saved_stimecmp) : "memory");
    menvcfg_val &= ~(1UL << 63);
    asm volatile("csrw 0x30A, %0" :: "r"(menvcfg_val) : "memory");
    asm volatile("csrw stvec, %0" :: "r"(saved_stvec) : "memory");
    mideleg_val &= ~(HS_SSIP | HS_STIP);
    asm volatile("csrw 0x303, %0" :: "r"(mideleg_val) : "memory");

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
 *
 * Per norm:geilen, if GEILEN is nonzero, bits GEILEN:1 shall be
 * writable in hgeie, and all other bit positions shall be read-only
 * zeros. This means the read-back value must be a contiguous mask
 * of the form (1<<N)-2 (bits N:1 all ones, bit 0 and bits above N
 * all zeros).
 * ------------------------------------------------------------------ */
TEST_REGISTER(hgeie_writable_bit_range);
bool hgeie_writable_bit_range(void)
{
    TEST_BEGIN("HGEI-02: Verify hgeie writable bit range");

    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)-1) : "memory");
    uintptr_t val;
    asm volatile("csrr %0, 0x607" : "=r"(val));

    /* bit 0 must be zero */
    TEST_ASSERT("hgeie bit 0 is read-zero", (val & 0x1) == 0);

    /* Verify writable bits are contiguous from bit 1 to GEILEN.
     * A valid mask has the form: bits[N:1] all 1, everything else 0.
     * Equivalently: val | 1 == (1 << (N+1)) - 1 for some N >= 0,
     * i.e., (val | 1) must be of the form (2^k - 1) for some k >= 1.
     * Check: (val | 1) & ((val | 1) + 1) == 0 (power-of-2 minus 1). */
    if (val != 0) {
        uintptr_t mask_with_bit0 = val | 1;
        TEST_ASSERT("hgeie writable bits are contiguous from bit 1",
                    (mask_with_bit0 & (mask_with_bit0 + 1)) == 0);
    } else {
        /* GEILEN=0: no writable bits, valid */
        TEST_ASSERT("hgeie all zero (GEILEN=0)", true);
    }

    /* Cleanup */
    asm volatile("csrw 0x607, zero" ::: "memory");

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
 * HGEI-04: hgeip AND hgeie triggers SGEIP
 *
 * Per norm:hip_sgeip_sgeie_acc_op, SGEIP in hip is 1 if and only if
 * the bitwise logical-AND of hgeip and hgeie is nonzero in any bit.
 *
 * hgeip is read-only and depends on external interrupt controller
 * (IMSIC guest interrupt files). This test verifies:
 *   1. When hgeip & hgeie == 0, hip.SGEIP must be 0.
 *   2. When hgeip & hgeie != 0 (if external source active),
 *      hip.SGEIP must be 1.
 * ------------------------------------------------------------------ */
TEST_REGISTER(hgeip_and_hgeie_triggers_sgeip);
bool hgeip_and_hgeie_triggers_sgeip(void)
{
    TEST_BEGIN("HGEI-04: Verify hgeip AND hgeie triggers SGEIP");

    /* Enable all possible guest external interrupt bits in hgeie */
    asm volatile("csrw 0x607, %0" :: "r"((uintptr_t)-1) : "memory");
    uintptr_t hgeie_val;
    asm volatile("csrr %0, 0x607" : "=r"(hgeie_val));

    if (hgeie_val == 0) {
        TEST_SKIP("GEILEN=0, guest external interrupts not implemented");
    }

    uintptr_t hgeip_val;
    asm volatile("csrr %0, 0xE12" : "=r"(hgeip_val));

    /* Read hip.SGEIP (bit 12) */
    uintptr_t hip_val;
    asm volatile("csrr %0, 0x644" : "=r"(hip_val));
    uintptr_t sgeip = (hip_val >> 12) & 1;

    if (hgeip_val & hgeie_val) {
        /* External interrupt source is active: SGEIP must be 1 */
        TEST_ASSERT_EQ("hip.SGEIP=1 when hgeip & hgeie != 0",
                       sgeip, (uintptr_t)1);
    } else {
        /* No external interrupt source active: SGEIP must be 0 */
        TEST_ASSERT_EQ("hip.SGEIP=0 when hgeip & hgeie == 0",
                       sgeip, (uintptr_t)0);

        /* Note: verifying SGEIP=1 requires an active external interrupt
         * source (e.g., IMSIC guest interrupt file). On platforms without
         * such a source, we can only verify the negative case. */
        printf("  [INFO] hgeip=0x%lx: no external source, "
               "SGEIP=1 path not testable\n",
               (unsigned long)hgeip_val);
    }

    /* Cleanup */
    asm volatile("csrw 0x607, zero" ::: "memory");

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
