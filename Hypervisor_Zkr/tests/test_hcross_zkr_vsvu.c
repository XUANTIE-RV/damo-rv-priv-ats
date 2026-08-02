/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 17.2: VS/VU-mode Access Control (SSEED + H extension)
 *
 * ZKR-HYP-03 ~ ZKR-HYP-11, ZKR-HYP-14 ~ ZKR-HYP-18
 *
 * Spec anchors:
 *   norm:mseccfg_sseed_VSorVU-mode_op
 *     — With H ext, HS-qualified read-write access to seed in VS/VU
 *       raises virtual-instruction (cause=22); other accesses raise
 *       illegal-instruction (cause=2).
 *   norm:mseccfg_sseed_useed_op_tbl
 *     — VS/VU + SSEED=0: any access -> illegal-instruction.
 *       VS/VU + SSEED=1: read-write access -> virtual-instruction.
 *   norm:seed_ro_illegal
 *     — Read-only access (csrrs/csrrc rs1=x0, csrrsi/csrrci uimm=0)
 *       raises illegal-instruction regardless of mode/SSEED.
 *
 * VS/VU-mode code is executed via run_in_vs_mode / run_in_vu_mode.
 * The trap is captured by the M-mode handler (trap_expect_begin).
 * =================================================================== */

/* Helper: run a seed-access callback in VS-mode and check the cause. */
static void _vs_expect_cause(uintptr_t (*fn)(uintptr_t),
                             bool expect_trap, uintptr_t cause)
{
    trap_expect_begin();
    (void)run_in_vs_mode(fn, 0);
    if (expect_trap) {
        TEST_ASSERT("VS-mode trap triggered", trap_was_triggered());
        if (trap_was_triggered())
            TEST_ASSERT_EQ("VS-mode cause", trap_get_cause(), cause);
    } else {
        TEST_ASSERT("VS-mode no trap", !trap_was_triggered());
    }
    trap_expect_end();
}

/* Helper: run a seed-access callback in VU-mode and check the cause. */
static void _vu_expect_cause(uintptr_t (*fn)(uintptr_t),
                             bool expect_trap, uintptr_t cause)
{
    trap_expect_begin();
    (void)run_in_vu_mode(fn, 0);
    if (expect_trap) {
        TEST_ASSERT("VU-mode trap triggered", trap_was_triggered());
        if (trap_was_triggered())
            TEST_ASSERT_EQ("VU-mode cause", trap_get_cause(), cause);
    } else {
        TEST_ASSERT("VU-mode no trap", !trap_was_triggered());
    }
    trap_expect_end();
}

/* ---- ZKR-HYP-03: SSEED=0 VS-mode csrrw -> illegal ---- */

TEST_REGISTER(test_zkr_hyp_03);
bool test_zkr_hyp_03(void)
{
    TEST_BEGIN("ZKR-HYP-03: SSEED=0 VS-mode csrrw -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED);

    _vs_expect_cause(_vs_seed_csrrw, true, CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-04: SSEED=1 VS-mode csrrw -> virtual-instruction ---- */

TEST_REGISTER(test_zkr_hyp_04);
bool test_zkr_hyp_04(void)
{
    TEST_BEGIN("ZKR-HYP-04: SSEED=1 VS-mode csrrw -> virtual-instruction");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    _vs_expect_cause(_vs_seed_csrrw, true, CAUSE_VIRTUAL_INSTRUCTION);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-05: SSEED=0 VU-mode csrrw -> illegal ---- */

TEST_REGISTER(test_zkr_hyp_05);
bool test_zkr_hyp_05(void)
{
    TEST_BEGIN("ZKR-HYP-05: SSEED=0 VU-mode csrrw -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED);

    _vu_expect_cause(_vs_seed_csrrw, true, CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-06: SSEED=1 VU-mode csrrw -> virtual-instruction ---- */

TEST_REGISTER(test_zkr_hyp_06);
bool test_zkr_hyp_06(void)
{
    TEST_BEGIN("ZKR-HYP-06: SSEED=1 VU-mode csrrw -> virtual-instruction");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    _vu_expect_cause(_vs_seed_csrrw, true, CAUSE_VIRTUAL_INSTRUCTION);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-07: SSEED=1 VS-mode csrrs x0 (RO) -> illegal ---- */

TEST_REGISTER(test_zkr_hyp_07);
bool test_zkr_hyp_07(void)
{
    TEST_BEGIN("ZKR-HYP-07: SSEED=1 VS-mode csrrs x0 (RO) -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    /* Read-only access condition takes priority: illegal, not virtual */
    _vs_expect_cause(_vs_seed_csrrs_ro, true, CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-08: SSEED=1 VS-mode csrrsi 0 (RO) -> illegal ---- */

TEST_REGISTER(test_zkr_hyp_08);
bool test_zkr_hyp_08(void)
{
    TEST_BEGIN("ZKR-HYP-08: SSEED=1 VS-mode csrrsi 0 (RO) -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    _vs_expect_cause(_vs_seed_csrrsi_ro, true, CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-09: SSEED=1 VS-mode csrrs rs1!=x0 -> virtual-inst ---- */

TEST_REGISTER(test_zkr_hyp_09);
bool test_zkr_hyp_09(void)
{
    TEST_BEGIN("ZKR-HYP-09: SSEED=1 VS-mode csrrs rs1!=x0 -> virtual-inst");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    /* csrrs with rs1!=x0 is HS-qualified read-write -> virtual-inst */
    _vs_expect_cause(_vs_seed_csrrs_rw, true, CAUSE_VIRTUAL_INSTRUCTION);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-10: SSEED=0 VS-mode csrrs rs1!=x0 -> illegal ---- */

TEST_REGISTER(test_zkr_hyp_10);
bool test_zkr_hyp_10(void)
{
    TEST_BEGIN("ZKR-HYP-10: SSEED=0 VS-mode csrrs rs1!=x0 -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED);

    _vs_expect_cause(_vs_seed_csrrs_rw, true, CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-14: SSEED=1 VU-mode csrrs x0 (RO) -> illegal ---- */

TEST_REGISTER(test_zkr_hyp_14);
bool test_zkr_hyp_14(void)
{
    TEST_BEGIN("ZKR-HYP-14: SSEED=1 VU-mode csrrs x0 (RO) -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    _vu_expect_cause(_vs_seed_csrrs_ro, true, CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-15: SSEED=1 VU-mode csrrsi 0 (RO) -> illegal ---- */

TEST_REGISTER(test_zkr_hyp_15);
bool test_zkr_hyp_15(void)
{
    TEST_BEGIN("ZKR-HYP-15: SSEED=1 VU-mode csrrsi 0 (RO) -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    _vu_expect_cause(_vs_seed_csrrsi_ro, true, CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-16: SSEED=1 VU-mode csrrs rs1!=x0 -> virtual-inst ---- */

TEST_REGISTER(test_zkr_hyp_16);
bool test_zkr_hyp_16(void)
{
    TEST_BEGIN("ZKR-HYP-16: SSEED=1 VU-mode csrrs rs1!=x0 -> virtual-inst");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    _vu_expect_cause(_vs_seed_csrrs_rw, true, CAUSE_VIRTUAL_INSTRUCTION);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-17: SSEED=0 VU-mode csrrs rs1!=x0 -> illegal ---- */

TEST_REGISTER(test_zkr_hyp_17);
bool test_zkr_hyp_17(void)
{
    TEST_BEGIN("ZKR-HYP-17: SSEED=0 VU-mode csrrs rs1!=x0 -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED);

    _vu_expect_cause(_vs_seed_csrrs_rw, true, CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

/* ---- ZKR-HYP-18: SSEED=1 VS-mode csrrci 0 (RO) -> illegal ---- */

TEST_REGISTER(test_zkr_hyp_18);
bool test_zkr_hyp_18(void)
{
    TEST_BEGIN("ZKR-HYP-18: SSEED=1 VS-mode csrrci 0 (RO) -> illegal");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();
    REQUIRE_SSEED();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    _vs_expect_cause(_vs_seed_csrrci_ro, true, CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}

TEST_REGISTER(test_zkr_hyp_11);
bool test_zkr_hyp_11(void)
{
    TEST_BEGIN("ZKR-HYP-11: SSEED=0 does not affect M-mode access");
    REQUIRE_H_EXT();
    REQUIRE_ZKR();

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED);

    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read_m());
    (void)val;

    mseccfg_write_zkr(orig);
    HYP_TEST_END();
}
