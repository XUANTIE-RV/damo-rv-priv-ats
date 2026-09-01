/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.3: conditional case - misa.v writable (HVEC-13)
 *
 * Spec anchor (vector-common.adoc):
 *   norm:vsstatus_vs_exists
 *     - for implementations with a writable misa.v field, the
 *       vsstatus.vs field MAY exist even when misa.v is clear.
 *
 * Both outcomes (field present or absent) are legal, so this is a
 * record-only case; it also skips entirely when misa.v is not
 * writable at runtime (the common situation). Platform V support
 * itself is declared by V_SUPPORTED in rvtest_config.h.
 * =================================================================== */

TEST_REGISTER(test_hvec_13);
bool test_hvec_13(void)
{
    TEST_BEGIN("HVEC-13: (cond) vsstatus.vs existence with misa.v=0");
    H_REQUIRED_OR_SKIP();

    /* Probe misa.v writability: write 0 then try to restore/set. */
    uintptr_t misa_orig = CSRR(misa);
    uintptr_t v_bit = (1UL << ('V' - 'A'));

    CSRW(misa, misa_orig & ~v_bit);
    uintptr_t cleared = CSRR(misa);
    CSRW(misa, misa_orig);

    if ((cleared & v_bit) != 0 || !(misa_orig & v_bit)) {
        /* misa.v not writable (or V absent and sticky): the norm is
         * conditional on a writable misa.v, nothing to verify. */
        TEST_SKIP("misa.v not writable (norm:vsstatus_vs_exists is "
                  "conditional)");
    }

    /* misa.v is writable: clear it and probe vsstatus.vs. Both
     * presence (read/write succeeds) and absence (illegal-instruction)
     * are legal per the "may exist" wording - record the outcome. */
    CSRW(misa, misa_orig & ~v_bit);

    M_TRAP_EXPECT_BEGIN();
    uintptr_t val = hvec_vsstatus_read();
    bool read_trapped = trap_was_triggered();
    trap_expect_end();

    if (read_trapped) {
        printf("[I] misa.v=0: vsstatus.vs read trapped (cause=%lu) - "
               "field absent, legal per norm:vsstatus_vs_exists\n",
               (unsigned long)trap_get_cause());
    } else {
        printf("[I] misa.v=0: vsstatus.vs readable (vs=0x%lx) - "
               "field present, legal per norm:vsstatus_vs_exists\n",
               (unsigned long)((val >> HVEC_VS_SHIFT) & HVEC_CTX_MASK));

        /* If present, a write must also be accepted (read the field
         * back; the value itself is unconstrained here). */
        M_TRAP_EXPECT_BEGIN();
        hvec_vsstatus_set_field(HVEC_VS_SHIFT, CTX_INITIAL);
        (void)hvec_vsstatus_field(HVEC_VS_SHIFT);
        bool write_trapped = trap_was_triggered();
        trap_expect_end();
        TEST_ASSERT("present vsstatus.vs accepts writes", !write_trapped);
    }

    CSRW(misa, misa_orig);

    HYP_TEST_END();
}
