/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 17.3: Exception Priority and Combined Scenarios
 *
 * ZKR-HYP-12
 *
 * Spec anchors:
 *   norm:seed_ro_illegal
 *     — Read-only access raises illegal-instruction regardless of mode.
 *   norm:mseccfg_sseed_VSorVU-mode_op
 *     — HS-qualified read-write in VS/VU raises virtual-instruction.
 *
 * Verifies that the read-only access condition (illegal-instruction)
 * takes priority over the HS-qualified read-write condition
 * (virtual-instruction) when both could apply in VU-mode.
 * =================================================================== */

/* ---- ZKR-HYP-12: VU-mode RO access priority over virtual-inst ---- */

TEST_REGISTER(test_zkr_hyp_12);
bool test_zkr_hyp_12(void)
{
    TEST_BEGIN("ZKR-HYP-12: VU-mode RO access -> illegal (not virtual)");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    /*
     * SSEED=1 + VU-mode + csrrs rd, seed, x0:
     *   - HS-qualified read-write would give virtual-instruction (22)
     *   - but csrrs with rs1=x0 is read-only -> illegal-instruction (2)
     * The read-only condition must win: expect cause=2.
     */
    trap_expect_begin();
    (void)run_in_vu_mode(_vs_seed_csrrs_ro, 0);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        uintptr_t cause = trap_get_cause();
        TEST_ASSERT_EQ("cause=2 (illegal), not 22 (virtual)",
                       cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    }
    trap_expect_end();

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}
