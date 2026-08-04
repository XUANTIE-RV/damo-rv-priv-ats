/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 4: hideleg interrupt delegation and cause translation
 *
 * Tests DELEG-08 through DELEG-14 verify interrupt delegation to
 * VS-mode and VS interrupt number translation.
 * Split from Hypervisor/tests/test_delegation.c.
 * See DOCS/testplan/Hypervisor_Interrupts_test_plan.md.
 */

#include "hyp_test_helpers.h"
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
