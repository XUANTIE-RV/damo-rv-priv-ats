/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_illegal.c - Group 5: Illegal Instruction exceptions
 *
 * Per Sstvala (SPEC/sstvala.adoc:9-10):
 *   For illegal-instruction exception, stval must contain the faulting
 *   instruction encoding (32-bit or 16-bit, zero-extended to XLEN).
 *
 * Tests:
 *   TVAL-ILL-01: 32-bit custom-0 illegal instruction (0x0000000B)
 *   TVAL-ILL-02: 32-bit write to read-only CSR cycle (0xC0001073)
 *   TVAL-ILL-03: 32-bit access non-existent CSR 0x004 (0x004022F3)
 *   TVAL-ILL-04: 16-bit compressed illegal instruction (0x0000)
 *   TVAL-ILL-05: consecutive two different illegal instructions
 */

/*
 * Illegal instruction encodings:
 *
 * 0x0000000B: bits[1:0]=11 (32-bit), opcode[6:0]=0001011 (custom-0)
 * 0xC0001073: csrrw x0, cycle(0xC00), x0 -- write to read-only CSR
 * 0x004022F3: csrrs x5, 0x004, x0 -- access non-existent CSR
 * 0x0000:     16-bit all-zero (illegal compressed instruction)
 */

/* 32-bit illegal instructions, 4-byte aligned */
static uint32_t illegal_custom0    __attribute__((aligned(4))) = 0x0000000BU;
static uint32_t illegal_wr_cycle   __attribute__((aligned(4))) = 0xC0001073U;
static uint32_t illegal_csr_004    __attribute__((aligned(4))) = 0x004022F3U;


/* 16-bit illegal compressed instruction + c.nop fallthrough */
static uint16_t illegal_compressed[2] __attribute__((aligned(2))) = {
    0x0000,  /* illegal 16-bit instruction */
    0x0001,  /* c.nop (safe landing after trap handler advances PC) */
};

/* ===================================================================
 * TVAL-ILL-01: 32-bit custom-0 illegal instruction
 * =================================================================== */
TEST_REGISTER(test_sstvala_ill_01);
bool test_sstvala_ill_01(void) {
    TEST_BEGIN("TVAL-ILL-01: illegal custom-0 (0x0000000B) stval == encoding");

    uintptr_t addr = (uintptr_t)&illegal_custom0;

    ensure_smode_pmp();
    goto_priv(PRIV_S);
    PRIV_DO_EXEC_TRAP(addr);
    goto_priv(PRIV_M);

    CHECK_TRAP("illegal-inst trap", CAUSE_ILLEGAL_INST);
    TEST_ASSERT_EQ("stval == 0x0000000B",
                   trap_get_tval(), 0x0000000BUL);

    TEST_END();
}

/* ===================================================================
 * TVAL-ILL-02: write to read-only CSR cycle
 * =================================================================== */
TEST_REGISTER(test_sstvala_ill_02);
bool test_sstvala_ill_02(void) {
    TEST_BEGIN("TVAL-ILL-02: write read-only CSR (0xC0001073) stval == encoding");

    uintptr_t addr = (uintptr_t)&illegal_wr_cycle;

    ensure_smode_pmp();
    goto_priv(PRIV_S);
    PRIV_DO_EXEC_TRAP(addr);
    goto_priv(PRIV_M);

    CHECK_TRAP("illegal-inst trap", CAUSE_ILLEGAL_INST);
    TEST_ASSERT_EQ("stval == 0xC0001073",
                   trap_get_tval(), 0xC0001073UL);

    TEST_END();
}

/* ===================================================================
 * TVAL-ILL-03: access non-existent CSR 0x004
 * =================================================================== */
TEST_REGISTER(test_sstvala_ill_03);
bool test_sstvala_ill_03(void) {
    TEST_BEGIN("TVAL-ILL-03: non-existent CSR 0x004 (0x004022F3) stval == encoding");

    uintptr_t addr = (uintptr_t)&illegal_csr_004;

    ensure_smode_pmp();
    goto_priv(PRIV_S);
    PRIV_DO_EXEC_TRAP(addr);
    goto_priv(PRIV_M);

    CHECK_TRAP("illegal-inst trap", CAUSE_ILLEGAL_INST);
    TEST_ASSERT_EQ("stval == 0x004022F3",
                   trap_get_tval(), 0x004022F3UL);

    TEST_END();
}


/* ===================================================================
 * TVAL-ILL-04: 16-bit compressed illegal instruction
 * =================================================================== */
TEST_REGISTER(test_sstvala_ill_04);
bool test_sstvala_ill_04(void) {
    TEST_BEGIN("TVAL-ILL-04: compressed illegal (0x0000) stval == 0x0000");

    uintptr_t addr = (uintptr_t)&illegal_compressed[0];

    ensure_smode_pmp();
    goto_priv(PRIV_S);
    PRIV_DO_EXEC_TRAP(addr);
    goto_priv(PRIV_M);

    CHECK_TRAP("illegal-inst trap", CAUSE_ILLEGAL_INST);
    TEST_ASSERT_EQ("stval == 0x0000 (16-bit, zero-extended)",
                   trap_get_tval(), 0x0000UL);

    TEST_END();
}

/* ===================================================================
 * TVAL-ILL-05: consecutive two different illegal instructions
 * =================================================================== */
TEST_REGISTER(test_sstvala_ill_05);
bool test_sstvala_ill_05(void) {
    TEST_BEGIN("TVAL-ILL-05: two different illegal instructions, stval differs");

    /* First: custom-0 (0x0000000B) */
    uintptr_t addr1 = (uintptr_t)&illegal_custom0;
    ensure_smode_pmp();
    goto_priv(PRIV_S);
    PRIV_DO_EXEC_TRAP(addr1);
    goto_priv(PRIV_M);

    CHECK_TRAP("1st illegal-inst trap", CAUSE_ILLEGAL_INST);
    TEST_ASSERT_EQ("1st stval == 0x0000000B",
                   trap_get_tval(), 0x0000000BUL);

    /* Second: write read-only CSR (0xC0001073) */
    uintptr_t addr2 = (uintptr_t)&illegal_wr_cycle;
    ensure_smode_pmp();
    goto_priv(PRIV_S);
    PRIV_DO_EXEC_TRAP(addr2);
    goto_priv(PRIV_M);

    CHECK_TRAP("2nd illegal-inst trap", CAUSE_ILLEGAL_INST);
    TEST_ASSERT_EQ("2nd stval == 0xC0001073",
                   trap_get_tval(), 0xC0001073UL);

    TEST_END();
}
