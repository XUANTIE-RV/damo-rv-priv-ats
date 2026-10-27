/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 17.1: HS-mode Access Control (SSEED)
 *
 * ZKR-HYP-01 ~ ZKR-HYP-02, ZKR-HYP-13
 *
 * Spec anchors:
 *   norm:mseccfg_sseed_SorHS-mode_op
 *     — SSEED=0: S/HS-mode access to seed raises illegal-instruction.
 *       SSEED=1: read-write access allowed; other accesses raise
 *       illegal-instruction.
 *
 * =================================================================== */

/* ---- ZKR-HYP-01: SSEED=1 HS-mode csrrw access OK ---- */

TEST_REGISTER(test_zkr_hyp_01);
bool test_zkr_hyp_01(void)
{
    TEST_BEGIN("ZKR-HYP-01: SSEED=1 HS-mode csrrw access seed OK");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZKR_AVAILABLE) TEST_SKIP("Zkr not implemented");
    REQUIRE_SSEED();

    goto_priv(PRIV_S);

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    PRIV_DO(HS_SEED_CSRRW_CAPTURE());
    CHECK_NO_TRAP("HS-mode csrrw with SSEED=1");

    /* Validate the returned value has valid OPST */
    uintptr_t opst = g_hs_seed_val & SEED_OPST_MASK;
    TEST_ASSERT("HS-mode returned value has valid OPST",
                opst == SEED_OPST_BIST || opst == SEED_OPST_WAIT ||
                opst == SEED_OPST_ES16 || opst == SEED_OPST_DEAD);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-13: SSEED=1 HS-mode csrrs x0 (RO) -> illegal ---- */

TEST_REGISTER(test_zkr_hyp_13);
bool test_zkr_hyp_13(void)
{
    TEST_BEGIN("ZKR-HYP-13: SSEED=1 HS-mode csrrs x0 (RO) -> illegal");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZKR_AVAILABLE) TEST_SKIP("Zkr not implemented");
    REQUIRE_SSEED();

    goto_priv(PRIV_S);

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    /* Even with SSEED=1, read-only access traps in HS-mode */
    PRIV_DO(HS_SEED_CSRRS_RO());
    CHECK_TRAP("HS-mode csrrs x0 with SSEED=1", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

TEST_REGISTER(test_zkr_hyp_02);
bool test_zkr_hyp_02(void)
{
    TEST_BEGIN("ZKR-HYP-02: SSEED=0 HS-mode csrrw triggers illegal");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZKR_AVAILABLE) TEST_SKIP("Zkr not implemented");
    REQUIRE_SSEED();

    goto_priv(PRIV_S);

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED);

    PRIV_DO(HS_SEED_CSRRW());
    CHECK_TRAP("HS-mode csrrw with SSEED=0", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}
