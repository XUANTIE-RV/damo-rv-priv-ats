/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hypervisor x Zicbom: virtual-instruction stval tests (Group 6)
 *
 * Tests stval behavior when CBO management instructions trigger
 * virtual-instruction exceptions.
 *
 * Reference: Hypervisor_CMO_test_plan.md Group 6
 * Spec: virtual-instruction stval follows illegal-instruction rules:
 *       stval = faulting instruction encoding OR 0.
 *
 * Note: The CBO trampolines use a0 (x10) as rs1, so the actual
 * instruction encoding includes rs1=10 in bits [19:15].
 */

/*
 * stval validation for CBO virtual-instruction:
 * Per SPEC, stval = instruction encoding or 0.
 * We verify: if non-zero, bits [6:0]=0x0F, [14:12]=2, [31:20]=operation.
 */
static bool stval_is_cbo_insn(uintptr_t stval, uint32_t operation)
{
    if (stval == 0)
        return true;
    /* Check opcode=0x0F, funct3=2, operation matches */
    uint32_t expected = (operation << 20) | (2 << 12) | 0x0F;
    uint32_t mask = (0xFFF << 20) | (7 << 12) | 0x7F; /* op+funct3+opcode */
    return (stval & mask) == expected;
}

/* ===================================================================
 * HVINST-01: cbo.inval virtual-instruction stval
 * =================================================================== */
TEST_REGISTER(test_hvinst_01);
bool test_hvinst_01(void)
{
    TEST_BEGIN("HVINST-01: cbo.inval virtual-inst stval");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_DISABLE);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        /* stval: per illegal-instruction rules, either the instruction
         * encoding or 0 is acceptable */
        uintptr_t stval = trap_get_tval();
        TEST_ASSERT("stval=insn encoding or 0",
                    stval_is_cbo_insn(stval, CBO_OP_INVAL));
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HVINST-02: cbo.clean virtual-instruction stval
 * =================================================================== */
TEST_REGISTER(test_hvinst_02);
bool test_hvinst_02(void)
{
    TEST_BEGIN("HVINST-02: cbo.clean virtual-inst stval");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_clean, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        uintptr_t stval = trap_get_tval();
        TEST_ASSERT("stval=insn encoding or 0",
                    stval_is_cbo_insn(stval, CBO_OP_CLEAN));
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HVINST-03: cbo.flush virtual-instruction stval
 * =================================================================== */
TEST_REGISTER(test_hvinst_03);
bool test_hvinst_03(void)
{
    TEST_BEGIN("HVINST-03: cbo.flush virtual-inst stval");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbcfe(1);
    henvcfg_set_cbcfe(0);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, vs_cbo_flush, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        uintptr_t stval = trap_get_tval();
        TEST_ASSERT("stval=insn encoding or 0",
                    stval_is_cbo_insn(stval, CBO_OP_FLUSH));
    }

    ts2_finish(&ctx);
    TEST_END();
}

/* ===================================================================
 * HVINST-05: VU-mode cbo.inval virtual-instruction stval
 * =================================================================== */
TEST_REGISTER(test_hvinst_05);
bool test_hvinst_05(void)
{
    TEST_BEGIN("HVINST-05: VU-mode cbo.inval virtual-inst stval");

    H_REQUIRED_OR_SKIP();
    ZICBOM_REQUIRED_OR_SKIP();

    two_stage_ctx_t ctx;
    ts2_setup_full_u(&ctx, SATP_MODE_BARE, SUITE_HGATP_MODE);

    menvcfg_set_cbie(CBIE_INVAL);
    henvcfg_set_cbie(CBIE_DISABLE);

    uintptr_t addr = (uintptr_t)__cmo_test_data_start;
    trap_expect_begin();
    two_stage_run_in_vu(&ctx, vs_cbo_inval, addr);
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22",
                       trap_get_cause(),
                       (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        uintptr_t stval = trap_get_tval();
        TEST_ASSERT("stval=insn encoding or 0",
                    stval_is_cbo_insn(stval, CBO_OP_INVAL));
    }

    ts2_finish(&ctx);
    TEST_END();
}
