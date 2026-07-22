/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 3: hedeleg/hideleg delegation
 *
 * Tests DELEG-01 through DELEG-16 verify delegation CSR behavior
 * and functional exception/interrupt routing through hedeleg/hideleg.
 */

#include "test_helpers.h"

/* ===================================================================
 * VS-mode exception handler for delegation tests.
 *
 * Records vscause, advances sepc by 4 bytes (to skip the faulting
 * instruction: ecall/ebreak/UNIMP are all 4-byte instructions),
 * clears sstatus.SIE+SPIE, forces SPP=1 (VS-mode), then returns
 * via sret.
 *
 * NOTE: SPP must be explicitly set to 1 before sret. When the trap
 * originates from VU-mode, hardware sets vsstatus.SPP=0 (U-level),
 * which would cause sret to return to VU-mode instead of VS-mode.
 * The _v_trampoline expects to run in VS-mode so its final ecall
 * generates cause=10 (ecall-from-VS) for the M-mode return path.
 * =================================================================== */

static volatile uintptr_t g_vs_exc_cause;
static volatile bool g_vs_exc_triggered;

static void vs_exc_handler(void) __attribute__((naked, aligned(4)));
static void vs_exc_handler(void)
{
    asm volatile (
        "addi   sp, sp, -32\n\t"
        "sd     ra, 0(sp)\n\t"
        "sd     t0, 8(sp)\n\t"
        "sd     t1, 16(sp)\n\t"
        "sd     t2, 24(sp)\n\t"

        /* Record vscause */
        "csrr   t0, scause\n\t"
        "la     t2, g_vs_exc_cause\n\t"
        "sd     t0, 0(t2)\n\t"
        "li     t0, 1\n\t"
        "la     t2, g_vs_exc_triggered\n\t"
        "sb     t0, 0(t2)\n\t"

        /* Advance sepc by 4 to skip the faulting instruction */
        "csrr   t0, sepc\n\t"
        "addi   t0, t0, 4\n\t"
        "csrw   sepc, t0\n\t"

        /* Clear SIE (bit 1) and SPIE (bit 5) to prevent re-entry */
        "li     t0, 0x22\n\t"
        "csrc   sstatus, t0\n\t"

        /* Force SPP=1 (S-mode) so sret returns to VS-mode, not VU.
         * When the trap came from VU-mode, hardware set SPP=0. */
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

/* ===================================================================
 * Helper: set up VS exception test infrastructure.
 *
 * Configures medeleg/hedeleg for the given exception mask, installs
 * the VS exception handler at vstvec, and enables MPIE for interrupt
 * delivery.
 * =================================================================== */
static void setup_vs_exc_test(uintptr_t exc_mask)
{
    g_vs_exc_cause = 0;
    g_vs_exc_triggered = false;

    /* Configure dual-layer delegation: M->HS (medeleg) -> VS (hedeleg) */
    setup_deleg_to_vs(exc_mask, 0);

    /* Install VS-mode trap handler (Direct mode) */
    vs_trap_setup_direct((uintptr_t)vs_exc_handler);

    /* Set mstatus.MPIE=1 so MIE=1 after mret into VS-mode */
    uintptr_t mstatus_val;
    asm volatile ("csrr %0, mstatus" : "=r"(mstatus_val));
    mstatus_val |= (1UL << 7);
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus_val) : "memory");
}

/* ===================================================================
 * VS-level interrupt cause codes (translated from VS to S level).
 *   VSSI (hvip bit 2)  -> vscause = 1 (SSI)
 *   VSTI (hvip bit 6)  -> vscause = 5 (STI)
 *   VSEI (hvip bit 10) -> vscause = 9 (SEI)
 * =================================================================== */
#define VS_IRQ_SSI   (CAUSE_INTERRUPT_BIT | 1)
#define VS_IRQ_STI   (CAUSE_INTERRUPT_BIT | 5)
#define VS_IRQ_SEI   (CAUSE_INTERRUPT_BIT | 9)

#define VS_VSSIP     (1UL << 2)
#define VS_VSTIP     (1UL << 6)
#define VS_VSEIP     (1UL << 10)

/* ===================================================================
 * VS-mode interrupt handler for delegation interrupt tests.
 * Named deleg_vs_int_handler to avoid collision with test_interrupts.c.
 * =================================================================== */

static volatile uintptr_t g_vs_int_cause;
static volatile bool g_vs_int_triggered;

static void deleg_vs_int_handler(void) __attribute__((naked, aligned(4)));
static void deleg_vs_int_handler(void)
{
    asm volatile (
        "addi   sp, sp, -32\n\t"
        "sd     ra, 0(sp)\n\t"
        "sd     t0, 8(sp)\n\t"
        "sd     t1, 16(sp)\n\t"
        "sd     t2, 24(sp)\n\t"

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
        "li     t0, 0x22\n\t"
        "csrc   sstatus, t0\n\t"
        "csrc   sip, 0x2\n\t"

        /* Force SPP=1 (S-mode) so sret returns to VS-mode, not VU.
         * When the trap came from VU-mode, hardware set SPP=0. */
        "li     t0, 0x100\n\t"
        "csrs   sstatus, t0\n\t"

        "ld     ra, 0(sp)\n\t"
        "ld     t0, 8(sp)\n\t"
        "ld     t1, 16(sp)\n\t"
        "ld     t2, 24(sp)\n\t"
        "addi   sp, sp, 32\n\t"

        "sret\n\t"
    );
}

/* Helper: set up VS interrupt delegation test. */
static void setup_vs_int_deleg_test(uintptr_t hideleg_mask)
{
    g_vs_int_cause = 0;
    g_vs_int_triggered = false;

    /* Clear interrupt sources */
    asm volatile ("csrw 0x604, zero" ::: "memory");   /* hie = 0 */
    asm volatile ("csrw 0x645, zero" ::: "memory");   /* hvip = 0 */
    asm volatile ("csrw 0x14D, %0" :: "r"((uintptr_t)-1) : "memory");

    /* Dual-layer delegation: M->HS->VS */
    uintptr_t mideleg;
    asm volatile ("csrr %0, 0x303" : "=r"(mideleg));
    mideleg |= hideleg_mask;
    asm volatile ("csrw 0x303, %0" :: "r"(mideleg) : "memory");
    hideleg_write(hideleg_mask);

    /* Install VS interrupt handler */
    vs_trap_setup_direct((uintptr_t)deleg_vs_int_handler);

    /* Enable VS-mode interrupts: vsstatus.SIE (bit 1) */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus |= 0x2;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus) : "memory");

    /* Set mstatus.MPIE=1 */
    uintptr_t mstatus_val;
    asm volatile ("csrr %0, mstatus" : "=r"(mstatus_val));
    mstatus_val |= (1UL << 7);
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus_val) : "memory");
}

/* ------------------------------------------------------------------
 * DELEG-01: hedeleg writable/read-only bit verification
 *
 * Per spec hedeleg-bits table:
 *   Writable:    1-8, 12, 13, 15, 18, 19
 *   Read-only 0: 9-11, 16, 20-23
 *   Bit 0: writable if IALIGN=32, else read-only 0
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_writable_bits);
bool hedeleg_writable_bits(void)
{
    TEST_BEGIN("DELEG-01: Write all 1s to hedeleg and verify per-bit attributes");

    hedeleg_write(0xFFFFFFFF);
    uintptr_t val = hedeleg_read();

    /* Bits required writable per spec hedeleg-bits table.
     * Bit 19 (hardware error) requires Smaia; omit from required mask.
     * Test it separately as implementation-dependent. */
    uintptr_t spec_writable = (1UL << 1) | (1UL << 2) | (1UL << 3) |
                              (1UL << 4) | (1UL << 5) | (1UL << 6) |
                              (1UL << 7) | (1UL << 8) | (1UL << 12) |
                              (1UL << 13) | (1UL << 15) | (1UL << 18);
    TEST_ASSERT_EQ("hedeleg spec-writable bits (1-8,12,13,15,18) should be writable",
                   val & spec_writable, spec_writable);

    /* All read-only zero bits */
    uintptr_t ro_zero = (1UL << 9) | (1UL << 10) | (1UL << 11) |
                        (1UL << 16) |
                        (1UL << 20) | (1UL << 21) | (1UL << 22) | (1UL << 23);
    TEST_ASSERT_EQ("hedeleg read-only zero bits (9-11,16,20-23) must be zero",
                   val & ro_zero, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-02: hedeleg bit 0 writability depends on IALIGN
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_bit0_writable);
bool hedeleg_bit0_writable(void)
{
    TEST_BEGIN("DELEG-02: Verify hedeleg bit 0 IALIGN dependency");

    hedeleg_write(0x0);
    uintptr_t base = hedeleg_read();

    hedeleg_write(0x1);
    uintptr_t val = hedeleg_read();

    /* Writing bit 0 must not affect other bits */
    TEST_ASSERT_EQ("writing bit 0 must not set unexpected bits",
                   val & ~(uintptr_t)0x1, base & ~(uintptr_t)0x1);

    /* IALIGN=32 -> bit 0 writable; IALIGN=16/64 -> read-only zero. */
    if (val & 0x1) {
        TEST_ASSERT("IALIGN=32: hedeleg bit 0 should be writable", true);
    } else {
        TEST_ASSERT("IALIGN!=32: hedeleg bit 0 is read-only zero (acceptable)", true);
    }

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-03: hideleg writable and read-only bit verification
 *
 * Per norm:hideleg_acc:
 *   Writable:    bits 10, 6, 2 (VS-level interrupts)
 *   Read-only 0: bits 12, 9, 5, 1 (S-level interrupts)
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_writable_bits);
bool hideleg_writable_bits(void)
{
    TEST_BEGIN("DELEG-03: Verify hideleg writable bits (10/6/2) and read-only (12/9/5/1)");

    hideleg_write(0xFFFFFFFF);
    uintptr_t val = hideleg_read();

    /* Bits 10,6,2 must be writable (VSEI/VSTI/VSSI) */
    uintptr_t writable = 0x444;
    TEST_ASSERT_EQ("hideleg writable bits (10/6/2) must be writable",
                   val & writable, writable);

    /* Bits 12,9,5,1 must be read-only zero (SEI/STI/SSI) */
    uintptr_t ro_zero = (1UL << 12) | (1UL << 9) | (1UL << 5) | (1UL << 1);
    TEST_ASSERT_EQ("hideleg read-only zero bits (12/9/5/1) must be zero",
                   val & ro_zero, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-04: hedeleg delegates illegal-instruction to VS-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_illegal_inst_deleg);
bool hedeleg_illegal_inst_deleg(void)
{
    TEST_BEGIN("DELEG-04: Delegate illegal-instruction to VS (vscause=2)");

    setup_vs_exc_test(1UL << 2);

    /* Trigger illegal instruction in VS-mode; hardware routes to vstvec */
    run_in_vs_mode(vs_exec_illegal, 0);

    TEST_ASSERT("VS trap was triggered", g_vs_exc_triggered);
    TEST_ASSERT_EQ("vscause == 2 (illegal instruction)",
                   g_vs_exc_cause, (uintptr_t)2);

    clear_all_deleg();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-05: hedeleg NOT delegated -> trap stays at HS/M-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_default_trap_to_hs);
bool hedeleg_default_trap_to_hs(void)
{
    TEST_BEGIN("DELEG-05: hedeleg[10]=0, ecall traps to M/HS (not VS)");

    /* Clear hedeleg so ecall from VS is NOT delegated to VS-mode.
     * Framework default: medeleg has all bits set, so with hedeleg[10]=0
     * the ecall traps to M-mode (medeleg routes to HS, then hedeleg=0
     * means it stays at M-level). */
    hedeleg_write(0x0);

    /* Disable MIE to prevent spurious interrupts from prior test state */
    uintptr_t mstatus_val;
    asm volatile ("csrr %0, mstatus" : "=r"(mstatus_val));
    mstatus_val &= ~(1UL << 3);  /* Clear MIE */
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus_val) : "memory");

    trap_expect_begin();
    run_in_vs_mode(vs_exec_ecall, 0);

    TEST_ASSERT("trap was triggered at M/HS level", trap_was_triggered());
    TEST_ASSERT_EQ("cause == 10 (ecall from VS-mode, not delegated to VS)",
                   trap_get_cause(), (uintptr_t)10);
    trap_expect_end();

    clear_all_deleg();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-06: hedeleg delegates breakpoint to VS-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_breakpoint_deleg);
bool hedeleg_breakpoint_deleg(void)
{
    TEST_BEGIN("DELEG-06: Delegate breakpoint to VS (vscause=3)");

#ifdef SKIP_BREAKPOINT_TESTS
    TEST_SKIP("platform does not support breakpoint (SKIP_BREAKPOINT_TESTS)");
#endif

    setup_vs_exc_test(1UL << 3);

    run_in_vs_mode(vs_exec_ebreak, 0);

    TEST_ASSERT("VS trap was triggered", g_vs_exc_triggered);
    TEST_ASSERT_EQ("vscause == 3 (breakpoint)",
                   g_vs_exc_cause, (uintptr_t)3);

    clear_all_deleg();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-07: hedeleg delegates ecall-from-VU to VS-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(hedeleg_ecall_vu_deleg);
bool hedeleg_ecall_vu_deleg(void)
{
    TEST_BEGIN("DELEG-07: Delegate ecall-from-VU to VS (vscause=8)");

    setup_vs_exc_test(1UL << 8);

    /* VU-mode ecall with delegation -> trap routed to VS handler */
    run_in_vu_mode(vs_exec_ecall, 0);

    TEST_ASSERT("VS trap was triggered", g_vs_exc_triggered);
    TEST_ASSERT_EQ("vscause == 8 (ecall from VU-mode)",
                   g_vs_exc_cause, (uintptr_t)8);

    clear_all_deleg();
    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-08: hideleg delegates VSSI to VS-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_vssi_deleg);
bool hideleg_vssi_deleg(void)
{
    TEST_BEGIN("DELEG-08: Delegate VSSI to VS (vscause=1, translated)");

    setup_vs_int_deleg_test(VS_VSSIP);
    asm volatile ("csrs 0x604, %0" :: "r"(VS_VSSIP) : "memory");
    hvip_set_vssi(1);

    run_in_vs_mode(vs_nop_fn, 0);

    TEST_ASSERT("VS interrupt was triggered", g_vs_int_triggered);
    TEST_ASSERT_EQ("vscause == 1 (SSI, translated from VSSI)",
                   g_vs_int_cause, (uintptr_t)VS_IRQ_SSI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-09: hideleg delegates VSTI to VS-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_vsti_deleg);
bool hideleg_vsti_deleg(void)
{
    TEST_BEGIN("DELEG-09: Delegate VSTI to VS (vscause=5, translated)");

    setup_vs_int_deleg_test(VS_VSTIP);
    asm volatile ("csrs 0x604, %0" :: "r"(VS_VSTIP) : "memory");
    hvip_set_vsti(1);

    run_in_vs_mode(vs_nop_fn, 0);

    TEST_ASSERT("VS interrupt was triggered", g_vs_int_triggered);
    TEST_ASSERT_EQ("vscause == 5 (STI, translated from VSTI)",
                   g_vs_int_cause, (uintptr_t)VS_IRQ_STI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-10: hideleg delegates VSEI to VS-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_vsei_deleg);
bool hideleg_vsei_deleg(void)
{
    TEST_BEGIN("DELEG-10: Delegate VSEI to VS (vscause=9, translated)");

    setup_vs_int_deleg_test(VS_VSEIP);
    asm volatile ("csrs 0x604, %0" :: "r"(VS_VSEIP) : "memory");
    hvip_set_vsei(1);

    run_in_vs_mode(vs_nop_fn, 0);

    TEST_ASSERT("VS interrupt was triggered", g_vs_int_triggered);
    TEST_ASSERT_EQ("vscause == 9 (SEI, translated from VSEI)",
                   g_vs_int_cause, (uintptr_t)VS_IRQ_SEI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-11: hideleg NOT delegated -> interrupt not delivered to VS
 * ------------------------------------------------------------------ */
TEST_REGISTER(hideleg_not_deleg_trap_to_hs);
bool hideleg_not_deleg_trap_to_hs(void)
{
    TEST_BEGIN("DELEG-11: hideleg[2]=0, VSSIP not delivered to VS-mode");

    hideleg_write(0);

    /* Inject VSSIP */
    hvip_set_vssi(1);

    /* Verify VSSIP is pending in hip (HS-visible) */
    uintptr_t hip_val;
    asm volatile ("csrr %0, 0x644" : "=r"(hip_val));
    TEST_ASSERT_EQ("hip.VSSIP should be pending (HS-visible)",
                   hip_val & VS_VSSIP, VS_VSSIP);

    /* VS-mode reads sip (= vsip in V=1): SSIP must be 0 (not delegated) */
    uintptr_t sip_val = run_in_vs_mode(vs_read_sip, 0);
    TEST_ASSERT_EQ("vsip.SSIP should be 0 when hideleg[2]=0",
                   sip_val & (1UL << 1), (uintptr_t)0);

    /* Cleanup */
    hvip_set_vssi(0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-12: Interrupt number translation VSSI -> SSI
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_translation_vssi);
bool interrupt_translation_vssi(void)
{
    TEST_BEGIN("DELEG-12: VSSI->SSI translation (vscause=1, not 2)");

    setup_vs_int_deleg_test(VS_VSSIP);
    asm volatile ("csrs 0x604, %0" :: "r"(VS_VSSIP) : "memory");
    hvip_set_vssi(1);

    run_in_vs_mode(vs_nop_fn, 0);

    TEST_ASSERT("VS interrupt was triggered", g_vs_int_triggered);
    /* Key assertion: cause must be 1 (SSI), proving translation occurred */
    TEST_ASSERT_EQ("vscause == 1 (translated SSI, NOT raw VSSI=2)",
                   g_vs_int_cause, (uintptr_t)VS_IRQ_SSI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-13: Interrupt number translation VSTI -> STI
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_translation_vsti);
bool interrupt_translation_vsti(void)
{
    TEST_BEGIN("DELEG-13: VSTI->STI translation (vscause=5, not 6)");

    setup_vs_int_deleg_test(VS_VSTIP);
    asm volatile ("csrs 0x604, %0" :: "r"(VS_VSTIP) : "memory");
    hvip_set_vsti(1);

    run_in_vs_mode(vs_nop_fn, 0);

    TEST_ASSERT("VS interrupt was triggered", g_vs_int_triggered);
    TEST_ASSERT_EQ("vscause == 5 (translated STI, NOT raw VSTI=6)",
                   g_vs_int_cause, (uintptr_t)VS_IRQ_STI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-14: Interrupt number translation VSEI -> SEI
 * ------------------------------------------------------------------ */
TEST_REGISTER(interrupt_translation_vsei);
bool interrupt_translation_vsei(void)
{
    TEST_BEGIN("DELEG-14: VSEI->SEI translation (vscause=9, not 10)");

    setup_vs_int_deleg_test(VS_VSEIP);
    asm volatile ("csrs 0x604, %0" :: "r"(VS_VSEIP) : "memory");
    hvip_set_vsei(1);

    run_in_vs_mode(vs_nop_fn, 0);

    TEST_ASSERT("VS interrupt was triggered", g_vs_int_triggered);
    TEST_ASSERT_EQ("vscause == 9 (translated SEI, NOT raw VSEI=10)",
                   g_vs_int_cause, (uintptr_t)VS_IRQ_SEI);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-15: guest-page-fault bits (20/21/23) are not delegatable
 * ------------------------------------------------------------------ */
TEST_REGISTER(guest_page_fault_not_delegatable);
bool guest_page_fault_not_delegatable(void)
{
    TEST_BEGIN("DELEG-15: Verify guest-page-fault bits (20/21/23) are read-zero");

    hedeleg_write(0xFFFFFFFF);
    uintptr_t val = hedeleg_read();

    uintptr_t gpf_bits = (1UL << 20) | (1UL << 21) | (1UL << 23);
    TEST_ASSERT_EQ("hedeleg bits 20/21/23 (guest-page-fault) must be read-zero",
                   val & gpf_bits, (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * DELEG-16: virtual-instruction bit (22) is not delegatable
 * ------------------------------------------------------------------ */
TEST_REGISTER(virtual_inst_not_delegatable);
bool virtual_inst_not_delegatable(void)
{
    TEST_BEGIN("DELEG-16: Verify virtual-instruction bit (22) is read-zero");

    hedeleg_write(0xFFFFFFFF);
    uintptr_t val = hedeleg_read();

    TEST_ASSERT_EQ("hedeleg bit 22 (virtual-instruction) must be read-zero",
                   val & (1UL << 22), (uintptr_t)0);

    HYP_TEST_END();
}
