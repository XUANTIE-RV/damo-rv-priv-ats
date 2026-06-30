/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_stip.c - Group 5: STIP Read-Only Behavior
 *
 * SSTC-STIP-01 ~ SSTC-STIP-05
 * Verifies that when Sstc is implemented (STCE=1), the STIP bit in
 * mip/sip becomes read-only — reflecting only the stimecmp vs time
 * comparison result — and cannot be software-toggled.
 */

/* ------------------------------------------------------------------ */
/* SSTC-STIP-01: mip.STIP write-1 ignored when STCE=1 (STIP=0)      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stip_readonly_write1);
bool test_sstc_stip_readonly_write1(void) {
    TEST_BEGIN("SSTC-STIP-01: mip.STIP write-1 ignored (STIP=0)");

    menvcfg_set(MENVCFG_STCE);

    /* Ensure STIP=0: stimecmp = MAX */
    sstc_clear_timer();
    DELAY_LOOP(SSTC_DELAY);

    uintptr_t mip_val = mip_read();
    TEST_ASSERT("STIP initially 0", (mip_val & MIP_STIP) == 0);

    /* Try to set STIP via csrs mip */
    CSRS(mip, MIP_STIP);

    mip_val = mip_read();
    TEST_ASSERT("STIP still 0 after write attempt", (mip_val & MIP_STIP) == 0);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-STIP-02: mip.STIP write-0 ignored when STCE=1 (STIP=1)      */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stip_readonly_write0);
bool test_sstc_stip_readonly_write0(void) {
    TEST_BEGIN("SSTC-STIP-02: mip.STIP write-0 ignored (STIP=1)");

    menvcfg_set(MENVCFG_STCE);

    /* Trigger STIP=1 */
    sstc_trigger_stip();
    DELAY_LOOP(SSTC_DELAY);

    uintptr_t mip_val = mip_read();
    TEST_ASSERT("STIP initially 1", (mip_val & MIP_STIP) != 0);

    /* Try to clear STIP via csrc mip */
    CSRC(mip, MIP_STIP);

    mip_val = mip_read();
    TEST_ASSERT("STIP still 1 after clear attempt", (mip_val & MIP_STIP) != 0);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-STIP-03: sip.STIP read-only                                  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stip_sip_readonly);
bool test_sstc_stip_sip_readonly(void) {
    TEST_BEGIN("SSTC-STIP-03: sip.STIP read-only in S-mode");

    sstc_enable_smode_access();
    sstc_clear_timer();
    DELAY_LOOP(SSTC_DELAY);

    /* In S-mode, try to set sip.STIP */
    goto_priv(PRIV_S);
    CSRS(sip, SIP_STIP);
    goto_priv(PRIV_M);

    /* STIP should still be 0 (stimecmp = MAX) */
    uintptr_t mip_val = mip_read();
    TEST_ASSERT("STIP still 0 after S-mode write", (mip_val & MIP_STIP) == 0);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-STIP-04: STIP follows stimecmp vs time comparison             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stip_follows_stimecmp);
bool test_sstc_stip_follows_stimecmp(void) {
    TEST_BEGIN("SSTC-STIP-04: STIP follows stimecmp vs time");

    menvcfg_set(MENVCFG_STCE);

    /* Start clear */
    sstc_clear_timer();
    DELAY_LOOP(SSTC_DELAY);
    uintptr_t mip_val = mip_read();
    TEST_ASSERT("STIP=0 with stimecmp=MAX", (mip_val & MIP_STIP) == 0);

    /* Trigger */
    sstc_trigger_stip();
    DELAY_LOOP(SSTC_DELAY);
    mip_val = mip_read();
    TEST_ASSERT("STIP=1 after trigger", (mip_val & MIP_STIP) != 0);

    /* Clear again */
    sstc_clear_timer();
    DELAY_LOOP(SSTC_DELAY);
    mip_val = mip_read();
    TEST_ASSERT("STIP=0 after clear", (mip_val & MIP_STIP) == 0);

    /* Trigger again */
    sstc_trigger_stip();
    DELAY_LOOP(SSTC_DELAY);
    mip_val = mip_read();
    TEST_ASSERT("STIP=1 after re-trigger", (mip_val & MIP_STIP) != 0);

    sstc_clear_timer();
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SSTC-STIP-05: STCE=0 restores mip.STIP writability                */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sstc_stip_stce0_writable);
bool test_sstc_stip_stce0_writable(void) {
    TEST_BEGIN("SSTC-STIP-05: STCE=0 restores mip.STIP writability");

    /* First enable STCE and clear timer to ensure clean state */
    menvcfg_set(MENVCFG_STCE);
    sstc_clear_timer();
    DELAY_LOOP(SSTC_DELAY);

    /* Now disable STCE — STIP should become software-writable again */
    menvcfg_clear(MENVCFG_STCE);

    /* Clear any residual STIP */
    CSRC(mip, MIP_STIP);

    /* Try to set STIP via csrs mip */
    CSRS(mip, MIP_STIP);
    uintptr_t mip_val = mip_read();
    TEST_ASSERT("STIP writable when STCE=0 (set to 1)",
                (mip_val & MIP_STIP) != 0);

    /* Clean up: clear STIP */
    CSRC(mip, MIP_STIP);

    TEST_END();
}
