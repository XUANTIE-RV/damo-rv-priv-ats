/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_seed_basic.c - Group 1: seed CSR Basic Format and Address
 *
 * SEED-01 ~ SEED-04
 * Verifies seed CSR existence, address encoding, and basic format.
 */

/* ------------------------------------------------------------------ */
/* SEED-01: M-mode csrrw access to seed CSR                           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_seed01_csrrw_access);
bool test_seed01_csrrw_access(void)
{
    TEST_BEGIN("SEED-01: M-mode csrrw access to seed CSR");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented (seed CSR traps in M-mode)");
    }

    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read());
    printf("  [INFO] seed = 0x%lx\n", (unsigned long)val);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SEED-02: seed CSR address encoding verification                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_seed02_addr_encoding);
bool test_seed02_addr_encoding(void)
{
    TEST_BEGIN("SEED-02: seed CSR address 0x015 encoding");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* Verify we're accessing the seed CSR (not another CSR) by checking
     * its unique signature: csrrw succeeds but csrrs x0 traps. */
    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read());

    /* Cross-check: csrrs rd, seed, x0 must trap (unique to seed CSR) */
    M_EXPECT_TRAP(CAUSE_ILLEGAL_INST, {
        (void)seed_read_ro_csrrs();
    });

    /* Verify OPST field is one of the 4 defined states */
    uintptr_t opst = val & SEED_OPST_MASK;
    TEST_ASSERT("OPST is BIST/WAIT/ES16/DEAD",
                opst == SEED_OPST_BIST || opst == SEED_OPST_WAIT ||
                opst == SEED_OPST_ES16 || opst == SEED_OPST_DEAD);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SEED-03: seed CSR returns 32-bit value                             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_seed03_32bit_value);
bool test_seed03_32bit_value(void)
{
    TEST_BEGIN("SEED-03: seed CSR returns 32-bit value");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read());

#if __riscv_xlen == 64
    /* On RV64, upper 32 bits should be zero (CSR is 32-bit) */
    TEST_ASSERT_EQ("upper 32 bits are zero", val >> 32, 0UL);
#else
    /* On RV32, just verify we got a value */
    TEST_ASSERT("seed value read successfully", true);
#endif

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SEED-04: seed CSR OPST field encoding is legal                     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_seed04_opst_legal);
bool test_seed04_opst_legal(void)
{
    TEST_BEGIN("SEED-04: seed CSR OPST field encoding is legal");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* Read seed multiple times and verify OPST is always valid
     * and reserved bits [29:24] are always zero. */
    bool all_valid = true;
    for (int i = 0; i < 16; i++) {
        uintptr_t val = seed_read();
        uintptr_t opst = val & SEED_OPST_MASK;
        /* OPST must be one of the 4 defined states */
        if (opst != SEED_OPST_BIST && opst != SEED_OPST_WAIT &&
            opst != SEED_OPST_ES16 && opst != SEED_OPST_DEAD) {
            all_valid = false;
        }
        /* Reserved bits [29:24] must be zero */
        if ((val & SEED_RESERVED_MASK) != 0) {
            all_valid = false;
        }
    }
    TEST_ASSERT("OPST valid and reserved bits zero across 16 reads", all_valid);

    TEST_END();
}
