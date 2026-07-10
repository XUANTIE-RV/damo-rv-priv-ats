/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_hcross_sstc_vs.c - Groups 9.3-9.5: vstimecmp / VSTIP / VS-mode Timer
 *
 * HCROSS-SSTC-06 ~ HCROSS-SSTC-15
 * Verifies vstimecmp CSR read/write, VSTIP synthesis logic, and
 * henvcfg.STCE control over VS-mode timer. All tests require H ext.
 *
 * Migrated from sstc_test_plan.md Group 6 (SSTC-VS-01~10).
 */

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-06: M-mode vstimecmp read-write                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_06);
bool test_hcross_sstc_06(void)
{
    TEST_BEGIN("HCROSS-SSTC-06: M-mode vstimecmp read-write");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    henvcfg_set(HENVCFG_STCE);

    uintptr_t test_val = (uintptr_t)0x123456789ABCDEF0ULL;
    vstimecmp_write(test_val);
    uintptr_t val = vstimecmp_read();
    TEST_ASSERT_EQ("vstimecmp read-write", val, test_val);

    vstimecmp_write((uintptr_t)-1);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-07: vstimecmp all-ones / all-zeros                     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_07);
bool test_hcross_sstc_07(void)
{
    TEST_BEGIN("HCROSS-SSTC-07: vstimecmp all-ones / all-zeros");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    henvcfg_set(HENVCFG_STCE);

    vstimecmp_write((uintptr_t)-1);
    uintptr_t val = vstimecmp_read();
    TEST_ASSERT_EQ("vstimecmp all-ones", val, (uintptr_t)-1);

    vstimecmp_write(0);
    val = vstimecmp_read();
    TEST_ASSERT_EQ("vstimecmp all-zeros", val, 0);

    vstimecmp_write((uintptr_t)-1);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-08: HS-mode vstimecmp read-write                       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_08);
bool test_hcross_sstc_08(void)
{
    TEST_BEGIN("HCROSS-SSTC-08: HS-mode vstimecmp read-write");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);
    henvcfg_set(HENVCFG_STCE);

    uintptr_t test_val = (uintptr_t)0xFEDCBA9876543210ULL;

    /* Switch to HS-mode (S-mode with V=0) and write vstimecmp via 0x24D */
    goto_priv(PRIV_S);
    vstimecmp_write(test_val);
    goto_priv(PRIV_M);

    uintptr_t val = vstimecmp_read();
    TEST_ASSERT_EQ("HS-mode vstimecmp write visible", val, test_val);

    vstimecmp_write((uintptr_t)-1);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-09: vstimecmp triggers VSTIP                           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_09);
bool test_hcross_sstc_09(void)
{
    TEST_BEGIN("HCROSS-SSTC-09: vstimecmp triggers VSTIP");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    henvcfg_set(HENVCFG_STCE);

    /* Clear hvip.VSTIP to isolate vstimecmp signal */
    hvip_clear(HVIP_VSTIP);

    /* Clear: vstimecmp = MAX */
    vstimecmp_write((uintptr_t)-1);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    /* Read time + htimedelta */
    uintptr_t htd = htimedelta_read();
    uintptr_t now = time_read();
    uintptr_t vtime = now + htd;

    /* Set vstimecmp to a past value */
    vstimecmp_write(vtime > 0 ? vtime - 1 : 0);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    uintptr_t hip_val = hip_read();
    TEST_ASSERT("VSTIP is set", (hip_val & HIP_VSTIP) != 0);

    vstimecmp_write((uintptr_t)-1);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-10: vstimecmp clears VSTIP                             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_10);
bool test_hcross_sstc_10(void)
{
    TEST_BEGIN("HCROSS-SSTC-10: vstimecmp MAX clears VSTIP");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    henvcfg_set(HENVCFG_STCE);
    hvip_clear(HVIP_VSTIP);

    /* Set vstimecmp to MAX */
    vstimecmp_write((uintptr_t)-1);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    uintptr_t hip_val = hip_read();
    TEST_ASSERT("VSTIP is clear (hvip=0, vstimecmp=MAX)",
                (hip_val & HIP_VSTIP) == 0);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-11: VSTIP = hvip.VSTIP OR vstimecmp signal             */
/*                                                                    */
/* Per spec (Sstc): when henvcfg.STCE=1, hip.VSTIP is read-only and   */
/* is the logical-OR of hvip.VSTIP and the vstimecmp signal.  In this */
/* mode hvip.VSTIP is also read-only zero (writes ignored), so the    */
/* OR reduces to: hip.VSTIP = vstimecmp_signal.                       */
/* We verify: (1) signal=0 -> VSTIP=0, (2) signal=1 -> VSTIP=1,      */
/*            (3) signal returns to 0 -> VSTIP=0.                     */
/* The hvip.VSTIP input to the OR gate is covered by HCROSS-SSTC-12.  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_11);
bool test_hcross_sstc_11(void)
{
    TEST_BEGIN("HCROSS-SSTC-11: VSTIP = hvip.VSTIP OR vstimecmp signal");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    henvcfg_set(HENVCFG_STCE);  /* STCE=1: vstimecmp signal active */
    hvip_clear(HVIP_VSTIP);

    /* ---- Sub-case A: vstimecmp=MAX -> signal=0 -> VSTIP=0 ---- */
    vstimecmp_write((uintptr_t)-1);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    uintptr_t hip_val = hip_read();
    TEST_ASSERT("VSTIP=0 when vstimecmp=MAX (no signal)",
                (hip_val & HIP_VSTIP) == 0);

    /* ---- Sub-case B: vstimecmp expired -> signal=1 -> VSTIP=1 ---- */
    /* Compute a past virtual time = time + htimedelta */
    uintptr_t htd = htimedelta_read();
    uintptr_t now = time_read();
    uintptr_t vtime = now + htd;
    vstimecmp_write(vtime > 0 ? vtime - 1 : 0);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    hip_val = hip_read();
    TEST_ASSERT("VSTIP=1 from vstimecmp signal (expired)",
                (hip_val & HIP_VSTIP) != 0);

    /* ---- Sub-case C: restore vstimecmp=MAX -> signal=0 -> VSTIP=0 ---- */
    vstimecmp_write((uintptr_t)-1);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    hip_val = hip_read();
    TEST_ASSERT("VSTIP=0 after vstimecmp returns to MAX",
                (hip_val & HIP_VSTIP) == 0);

    /* Clean up */
    hvip_clear(HVIP_VSTIP);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-12: henvcfg.STCE=0, VSTIP reverts to hvip-only         */
/*                                                                    */
/* Per spec: when henvcfg.STCE=0, the vstimecmp comparison signal is  */
/* disabled and hip.VSTIP is determined solely by the software-       */
/* writable hvip.VSTIP bit (old behaviour before Sstc).               */
/* We verify that a past vstimecmp value does NOT drive VSTIP=1.      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_12);
bool test_hcross_sstc_12(void)
{
    TEST_BEGIN("HCROSS-SSTC-12: henvcfg.STCE=0, VSTIP reverts to hvip-only");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    henvcfg_clear(HENVCFG_STCE);   /* STCE=0: vstimecmp signal disabled */
    hvip_clear(HVIP_VSTIP);

    /* Set vstimecmp to a past value; with STCE=0, this must NOT
     * generate a VSTIP signal. */
    vstimecmp_write(0);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    /* ---- Sub-case A: hvip.VSTIP=0, vstimecmp=past -> VSTIP=0 ---- */
    uintptr_t hip_val = hip_read();
    TEST_ASSERT("VSTIP=0 when STCE=0 despite vstimecmp=past",
                (hip_val & HIP_VSTIP) == 0);

    /* ---- Sub-case B: hvip.VSTIP=1 (manually set) -> VSTIP=1 ---- */
    /* Proves that VSTIP is now driven only by hvip.VSTIP. */
    hvip_set(HVIP_VSTIP);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    hip_val = hip_read();
    TEST_ASSERT("VSTIP=1 from hvip.VSTIP only when STCE=0",
                (hip_val & HIP_VSTIP) != 0);

    /* ---- Sub-case C: clear hvip.VSTIP -> VSTIP=0 again ---- */
    hvip_clear(HVIP_VSTIP);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    hip_val = hip_read();
    TEST_ASSERT("VSTIP=0 after clearing hvip.VSTIP (STCE=0)",
                (hip_val & HIP_VSTIP) == 0);

    /* Clean up */
    vstimecmp_write((uintptr_t)-1);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-13: VS-mode stimecmp -> vstimecmp transparent remap    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_13);
bool test_hcross_sstc_13(void)
{
    TEST_BEGIN("HCROSS-SSTC-13: VS-mode stimecmp -> vstimecmp remap");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);
    henvcfg_set(HENVCFG_STCE);
    hcounteren_set(HCOUNTEREN_TM);

    uintptr_t test_val = (uintptr_t)0xABCD1234ABCD0000ULL;

    /* VS-mode writes to stimecmp (CSR 0x14D) which hardware
     * transparently remaps to vstimecmp (CSR 0x24D). */
    run_in_vs_mode(_vs_stimecmp_write, test_val);

    /* M-mode reads vstimecmp and expects the same value */
    uintptr_t val = vstimecmp_read();
    TEST_ASSERT_EQ("VS stimecmp write -> vstimecmp", val, test_val);

    vstimecmp_write((uintptr_t)-1);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-14: htimedelta affects vstimecmp comparison             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_14);
bool test_hcross_sstc_14(void)
{
    TEST_BEGIN("HCROSS-SSTC-14: htimedelta affects vstimecmp comparison");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    henvcfg_set(HENVCFG_STCE);
    hvip_clear(HVIP_VSTIP);

    /* Save and set a large positive htimedelta */
    uintptr_t orig_htd = htimedelta_read();
    uintptr_t big_delta = 0x100000000ULL; /* 4G ticks ahead */
    htimedelta_write(big_delta);

    /* Clear first */
    vstimecmp_write((uintptr_t)-1);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    /* Now set vstimecmp so that (time + htimedelta) >= vstimecmp.
     * virtual_time = real_time + big_delta, so if we set vstimecmp
     * to real_time (which is < virtual_time), it should trigger. */
    uintptr_t now = time_read();
    vstimecmp_write(now);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    uintptr_t hip_val = hip_read();
    TEST_ASSERT("VSTIP=1 with htimedelta making vtime >= vstimecmp",
                (hip_val & HIP_VSTIP) != 0);

    /* Restore htimedelta */
    htimedelta_write(orig_htd);
    vstimecmp_write((uintptr_t)-1);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* HCROSS-SSTC-15: VS-mode timer interrupt capture                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_hcross_sstc_15);
bool test_hcross_sstc_15(void)
{
    TEST_BEGIN("HCROSS-SSTC-15: VS-mode timer interrupt capture");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);
    henvcfg_set(HENVCFG_STCE);
    hcounteren_set(HCOUNTEREN_TM);
    hvip_clear(HVIP_VSTIP);
    vstimecmp_write((uintptr_t)-1);

    /* Delegate VSTIP to VS-mode:
     * hideleg bit 6 = VSTIP, so VS-mode can handle it */
    CSRS(0x603, HIP_VSTIP);  /* hideleg */

    /* Enable VSTIE in vsie (bit 5 in vsie = STIE from VS perspective) */
    CSRS(0x204, SIE_STIE);  /* vsie */

    /* Install VS-mode trap handler.
     * We reuse sstc_trap_entry -- in VS-mode it writes to stimecmp
     * which maps to vstimecmp, clearing the VS timer interrupt. */
    g_sstc_trap_cause = 0;
    CSRW(0x240, (uintptr_t)sstc_trap_scratch); /* vsscratch */
    CSRW(0x205, (uintptr_t)sstc_trap_entry);   /* vstvec */

    /* Execute VS-mode timer interrupt test via run_in_vs_mode.
     * The _vs_timer_interrupt_test trampoline enables SIE, triggers
     * the timer, waits for the interrupt, then disables SIE. */
    run_in_vs_mode(_vs_timer_interrupt_test, 0);

    /* Check VS trap handler recorded the cause */
    uintptr_t expected = CAUSE_INTERRUPT_BIT | CAUSE_S_TIMER_INTERRUPT;
    TEST_ASSERT_EQ("VS-mode caught timer interrupt",
                   g_sstc_trap_cause, expected);

    /* Cleanup */
    vstimecmp_write((uintptr_t)-1);
    CSRC(0x603, HIP_VSTIP);  /* hideleg */
    CSRC(0x204, SIE_STIE);   /* vsie */
    TEST_END();
}
