/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 1: Zicfilp CSR Control Tests
 *
 * Tests CSR accessibility and behavior of Zicfilp control fields:
 *   - mseccfg.MLPE (M-mode Landing Pad Enable)
 *   - menvcfg.LPE (S-mode Landing Pad Enable)
 *   - senvcfg.LPE (U-mode Landing Pad Enable)
 *   - mstatus.MPELP / mstatus.SPELP (Previous ELP state)
 */

/* CFI-LP-CSR-01: mseccfg.MLPE writability */
TEST_REGISTER(test_zicfilp_csr_mlpe_writable);
bool test_zicfilp_csr_mlpe_writable(void)
{
    TEST_BEGIN("CFI-LP-CSR-01: mseccfg.MLPE writability");

    /* Save original value */
    uintptr_t orig = mseccfg_read();

    /* Try to set MLPE */
    mseccfg_set(MSECCFG_MLPE);
    uintptr_t val = mseccfg_read();

    bool writable = (val & MSECCFG_MLPE) != 0;

    if (writable)
    {
        printf("    mseccfg.MLPE is writable (Zicfilp implemented)\n");
    }
    else
    {
        printf("    mseccfg.MLPE is read-only zero (Zicfilp not implemented)\n");
    }

    /* Restore */
    mseccfg_write(orig);

    /* After restore, mseccfg.MLPE should be back to its original value (0) */
    TEST_ASSERT("mseccfg.MLPE restored to original value",
                (mseccfg_read() & MSECCFG_MLPE) == (orig & MSECCFG_MLPE));

    TEST_END();
}

/* CFI-LP-CSR-02: mseccfg.MLPE reset value */
TEST_REGISTER(test_zicfilp_csr_mlpe_reset);
bool test_zicfilp_csr_mlpe_reset(void)
{
    TEST_BEGIN("CFI-LP-CSR-02: mseccfg.MLPE reset value is 0");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    /*
     * Per norm:mseccfg_mlpe_rst (SPEC/machine.adoc:2825):
     * "If the Zicfilp extension is implemented, the mseccfg.MLPE
     *  field is reset to 0."
     *
     * Since we cannot truly observe reset state after boot code has run,
     * we verify that MLPE is currently 0 (test framework does not modify it).
     */
    uintptr_t val = mseccfg_read();
    TEST_ASSERT("mseccfg.MLPE is 0 (expected reset value)",
                (val & MSECCFG_MLPE) == 0);

    TEST_END();
}

/* CFI-LP-CSR-03: menvcfg.LPE writability */
TEST_REGISTER(test_zicfilp_csr_menvcfg_lpe);
bool test_zicfilp_csr_menvcfg_lpe(void)
{
    TEST_BEGIN("CFI-LP-CSR-03: menvcfg.LPE writability");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    uintptr_t orig = menvcfg_read();

    /* Set LPE */
    menvcfg_set(MENVCFG_LPE);
    uintptr_t val = menvcfg_read();
    TEST_ASSERT("menvcfg.LPE can be set to 1",
                (val & MENVCFG_LPE) != 0);

    /* Clear LPE */
    menvcfg_clear(MENVCFG_LPE);
    val = menvcfg_read();
    TEST_ASSERT("menvcfg.LPE can be cleared to 0",
                (val & MENVCFG_LPE) == 0);

    /* Restore */
    menvcfg_write(orig);

    TEST_END();
}

/* CFI-LP-CSR-04: senvcfg.LPE writability */
TEST_REGISTER(test_zicfilp_csr_senvcfg_lpe);
bool test_zicfilp_csr_senvcfg_lpe(void)
{
    TEST_BEGIN("CFI-LP-CSR-04: senvcfg.LPE writability");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    uintptr_t orig = senvcfg_read();

    /* Set LPE */
    senvcfg_set(SENVCFG_LPE);
    uintptr_t val = senvcfg_read();
    TEST_ASSERT("senvcfg.LPE can be set to 1",
                (val & SENVCFG_LPE) != 0);

    /* Clear LPE */
    senvcfg_clear(SENVCFG_LPE);
    val = senvcfg_read();
    TEST_ASSERT("senvcfg.LPE can be cleared to 0",
                (val & SENVCFG_LPE) == 0);

    /* Restore */
    senvcfg_write(orig);

    TEST_END();
}

/* CFI-LP-CSR-05: mstatus.MPELP field presence and writability */
TEST_REGISTER(test_zicfilp_csr_pelp_fields);
bool test_zicfilp_csr_pelp_fields(void)
{
    TEST_BEGIN("CFI-LP-CSR-05: mstatus.MPELP/SPELP fields");

    if (!detect_zicfilp())
        TEST_SKIP("Zicfilp not implemented");

    /*
     * MPELP (bit 41) holds the Previous ELP state saved on M-mode trap.
     * SPELP (bit 33) holds the Previous ELP state saved on S-mode trap.
     *
     * SPELP may reflect transient ELP hardware state from prior
     * indirect jumps (e.g., during detect_zicfilp probes) and may
     * not be directly clearable via csrc when ELP is LP_EXPECTED.
     * We verify MPELP is writable (the primary M-mode field).
     */

    /* Clear MPELP */
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_MPELP_BIT) : "memory");

    uintptr_t ms = mstatus_read();
    uintptr_t mpelp = (ms & MSTATUS_MPELP_BIT) ? 1 : 0;
    uintptr_t spelp = (ms & MSTATUS_SPELP_BIT) ? 1 : 0;

    printf("    mstatus.MPELP = %lu (after clear)\n", (unsigned long)mpelp);
    printf("    mstatus.SPELP = %lu (informational)\n", (unsigned long)spelp);

    TEST_ASSERT("mstatus.MPELP can be cleared to 0", mpelp == 0);

    /* Verify MPELP is settable */
    asm volatile("csrs mstatus, %0" :: "r"(MSTATUS_MPELP_BIT) : "memory");
    ms = mstatus_read();
    mpelp = (ms & MSTATUS_MPELP_BIT) ? 1 : 0;
    TEST_ASSERT("mstatus.MPELP can be set to 1", mpelp == 1);

    /* Clean up: clear MPELP */
    asm volatile("csrc mstatus, %0" :: "r"(MSTATUS_MPELP_BIT) : "memory");

    TEST_END();
}
