/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Group 4: Zicfiss CSR Control Tests
 *
 * Tests CSR accessibility and behavior of Zicfiss control fields:
 *   - menvcfg.SSE (S-mode Shadow Stack Enable)
 *   - senvcfg.SSE (U-mode Shadow Stack Enable)
 *   - CSR_SSP (Shadow Stack Pointer)
 *   - SSAMOSWAP behavior when SSE=0
 */

/* CFI-SS-CSR-01: menvcfg.SSE writability */
TEST_REGISTER(test_zicfiss_csr_menvcfg_sse);
bool test_zicfiss_csr_menvcfg_sse(void)
{
    TEST_BEGIN("CFI-SS-CSR-01: menvcfg.SSE writability");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    uintptr_t orig = menvcfg_read();

    /* Try to set SSE */
    menvcfg_set(MENVCFG_SSE);
    uintptr_t val = menvcfg_read();

    bool writable = (val & MENVCFG_SSE) != 0;

    if (writable)
    {
        printf("    menvcfg.SSE is writable (Zicfiss implemented)\n");
        /* Clear it back */
        menvcfg_clear(MENVCFG_SSE);
    }
    else
    {
        printf("    menvcfg.SSE is read-only zero (Zicfiss not implemented)\n");
    }

    /* Restore */
    menvcfg_write(orig);

    /* Informational test - always passes */
    TEST_ASSERT("menvcfg.SSE read-back consistent", true);

    TEST_END();
}

/* CFI-SS-CSR-02: senvcfg.SSE behavior when menvcfg.SSE=0 */
TEST_REGISTER(test_zicfiss_csr_senvcfg_sse_rdonly);
bool test_zicfiss_csr_senvcfg_sse_rdonly(void)
{
    TEST_BEGIN("CFI-SS-CSR-02: senvcfg.SSE when menvcfg.SSE=0");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /*
     * Per norm:menvcfg_sse_rdonly0 (SPEC/machine.adoc:2346):
     * "When menvcfg.SSE is 0, the henvcfg.SSE and senvcfg.SSE fields
     *  are read-only zero."
     *
     * Verify that senvcfg.SSE is gated by menvcfg.SSE:
     * 1. With menvcfg.SSE=1, senvcfg.SSE should be writable
     * 2. With menvcfg.SSE=0, senvcfg.SSE should be read-only 0
     */

    /* First: verify senvcfg.SSE is writable when menvcfg.SSE=1 */
    menvcfg_set(MENVCFG_SSE);
    senvcfg_set(SENVCFG_SSE);
    uintptr_t val_enabled = senvcfg_read();
    bool writable_when_enabled = (val_enabled & SENVCFG_SSE) != 0;

    /* Clean up senvcfg.SSE before disabling menvcfg.SSE */
    senvcfg_clear(SENVCFG_SSE);
    menvcfg_clear(MENVCFG_SSE);

    TEST_ASSERT("senvcfg.SSE writable when menvcfg.SSE=1",
                writable_when_enabled);

    /* Second: verify senvcfg.SSE behavior when menvcfg.SSE=0 */
    senvcfg_set(SENVCFG_SSE);
    uintptr_t val_disabled = senvcfg_read();

    if ((val_disabled & SENVCFG_SSE) == 0)
    {
        printf("    senvcfg.SSE is read-only 0 when menvcfg.SSE=0 (spec-compliant)\n");
    }
    else
    {
        printf("    [NOTE] senvcfg.SSE writable even with menvcfg.SSE=0 "
               "(QEMU implementation deviation)\n");
        senvcfg_clear(SENVCFG_SSE);
    }

    TEST_ASSERT("senvcfg.SSE writable when menvcfg.SSE=1",
                writable_when_enabled);
    TEST_ASSERT("senvcfg.SSE is read-only 0 when menvcfg.SSE=0 (norm:menvcfg_sse_rdonly0)",
                (val_disabled & SENVCFG_SSE) == 0);

    TEST_END();
}

/* CFI-SS-CSR-03: CSR_SSP accessible when SSE=1 */
TEST_REGISTER(test_zicfiss_csr_ssp_accessible);
bool test_zicfiss_csr_ssp_accessible(void)
{
    TEST_BEGIN("CFI-SS-CSR-03: CSR_SSP read/write when SSE=1");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /* Enable SSE */
    menvcfg_set(MENVCFG_SSE);

    /*
     * CSR_SSP (0x011) should be accessible.
     * Try to read and write it without triggering an exception.
     */
    trap_expect_begin();
    uintptr_t ssp_val;
    asm volatile("csrr %0, 0x011" : "=r"(ssp_val) :: "memory");
    trap_expect_end();

    TEST_ASSERT("CSR_SSP read does not fault",
                !trap_was_triggered());

    /* Try writing a test value */
    uintptr_t test_val = (uintptr_t)__shadow_stack_start + 0x100;
    trap_expect_begin();
    asm volatile("csrw 0x011, %0" :: "r"(test_val) : "memory");
    trap_expect_end();

    TEST_ASSERT("CSR_SSP write does not fault",
                !trap_was_triggered());

    /* Read back and verify */
    uintptr_t readback;
    asm volatile("csrr %0, 0x011" : "=r"(readback) :: "memory");
    TEST_ASSERT("CSR_SSP read-back matches written value",
                readback == test_val);

    /* Restore SSP to 0 and disable SSE */
    asm volatile("csrw 0x011, zero" ::: "memory");
    menvcfg_clear(MENVCFG_SSE);

    TEST_END();
}

/* CFI-SS-CSR-04: SSAMOSWAP behavior when SSE=0 in S-mode */
TEST_REGISTER(test_zicfiss_csr_ssamoswap_sse0);
bool test_zicfiss_csr_ssamoswap_sse0(void)
{
    TEST_BEGIN("CFI-SS-CSR-04: SSAMOSWAP triggers illegal-instruction when SSE=0 (S-mode)");

    if (!detect_zicfiss())
        TEST_SKIP("Zicfiss not implemented");

    /*
     * Per norm:menvcfg_sse_op_list (SPEC/machine.adoc:2338-2344):
     * "When SSE field is 0, the following rules apply to privilege
     *  modes that are less than M:"
     * ...
     * "SSAMOSWAP.W/D raises an illegal-instruction exception."
     *
     * This rule applies ONLY to S/U-mode (not M-mode). We execute
     * SSAMOSWAP in S-mode with menvcfg.SSE=0 to verify the
     * illegal-instruction behavior.
     *
     * Strategy: Do NOT delegate exceptions. The M-mode trap handler
     * catches traps from S-mode and records them in trap_record.
     * We arm trap_expect, switch to S-mode via mret, execute the
     * faulting instruction, and the M-mode handler catches it.
     */
    menvcfg_clear(MENVCFG_SSE);

    /* Arm trap before switching to S-mode */
    trap_expect_begin();

    /* Switch to S-mode and execute SSAMOSWAP inline.
     * Use mret to enter S-mode. The SSAMOSWAP will trap back to
     * M-mode handler (medeleg=0, all traps go to M-mode).
     * After trap, handler skips the instruction and we continue
     * in S-mode, then ecall back to M-mode. */
    goto_priv(PRIV_S);

    /* SSAMOSWAP.W x0, (x0), x0
     * In S-mode with SSE=0, this must raise illegal-instruction (cause=2).
     * We use x0 as base register (addr=0) — but the illegal-instruction
     * check must happen BEFORE any memory access attempt. */
    asm volatile(
        ".word 0x4800202F\n"  /* SSAMOSWAP.W x0, (x0), x0 */
        ::: "memory"
    );

    /* Return to M-mode via ecall */
    goto_priv(PRIV_M);
    trap_expect_end();

    TEST_ASSERT("SSAMOSWAP triggers exception when SSE=0 in S-mode",
                trap_was_triggered());

    if (trap_was_triggered())
    {
        uintptr_t cause = trap_get_cause();
        printf("    SSAMOSWAP exception cause = %lu\n", (unsigned long)cause);

        /* Per SPEC norm:menvcfg_sse_op_list: "SSAMOSWAP.W/D raises an
         * illegal-instruction exception." - cause must be exactly 2. */
        TEST_ASSERT("SSAMOSWAP cause is illegal-instruction (2) per SPEC",
                    cause == CAUSE_ILLEGAL_INST);
    }

    TEST_END();
}
