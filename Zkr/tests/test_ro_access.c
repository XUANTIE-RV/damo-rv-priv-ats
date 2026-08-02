/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_ro_access.c - Group 3: seed CSR Read-Only Access Exception
 *
 * ROACC-01 ~ ROACC-09
 * Verifies that read-only CSR access instructions trigger
 * illegal-instruction exception, and write values are ignored.
 */

/* ------------------------------------------------------------------ */
/* ROACC-01: csrrs rd, seed, x0 triggers exception                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_roacc01_csrrs_x0);
bool test_roacc01_csrrs_x0(void)
{
    TEST_BEGIN("ROACC-01: csrrs rd, seed, x0 triggers illegal-instruction");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    M_EXPECT_TRAP(CAUSE_ILLEGAL_INST, {
        (void)seed_read_ro_csrrs();
    });

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* ROACC-02: csrrc rd, seed, x0 triggers exception                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_roacc02_csrrc_x0);
bool test_roacc02_csrrc_x0(void)
{
    TEST_BEGIN("ROACC-02: csrrc rd, seed, x0 triggers illegal-instruction");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    M_EXPECT_TRAP(CAUSE_ILLEGAL_INST, {
        (void)seed_read_ro_csrrc();
    });

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* ROACC-03: csrrsi rd, seed, 0 triggers exception                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_roacc03_csrrsi_0);
bool test_roacc03_csrrsi_0(void)
{
    TEST_BEGIN("ROACC-03: csrrsi rd, seed, 0 triggers illegal-instruction");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    M_EXPECT_TRAP(CAUSE_ILLEGAL_INST, {
        (void)seed_read_ro_csrrsi();
    });

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* ROACC-04: csrrci rd, seed, 0 triggers exception                    */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_roacc04_csrrci_0);
bool test_roacc04_csrrci_0(void)
{
    TEST_BEGIN("ROACC-04: csrrci rd, seed, 0 triggers illegal-instruction");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    M_EXPECT_TRAP(CAUSE_ILLEGAL_INST, {
        (void)seed_read_ro_csrrci();
    });

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* ROACC-05: csrrw rd, seed, x0 normal access                         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_roacc05_csrrw_ok);
bool test_roacc05_csrrw_ok(void)
{
    TEST_BEGIN("ROACC-05: csrrw rd, seed, x0 normal access");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read());
    printf("  [INFO] seed = 0x%lx\n", (unsigned long)val);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* ROACC-06: csrrs rd, seed, rs1 (rs1!=x0) normal access              */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_roacc06_csrrs_nonzero);
bool test_roacc06_csrrs_nonzero(void)
{
    TEST_BEGIN("ROACC-06: csrrs rd, seed, rs1 (rs1!=x0) normal");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t val = 0;
    uintptr_t dummy = 1;
    M_EXPECT_NO_TRAP(val = seed_read_csrrs(dummy));
    printf("  [INFO] seed via csrrs = 0x%lx\n", (unsigned long)val);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* ROACC-07: csrrsi rd, seed, uimm (uimm!=0) normal access            */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_roacc07_csrrsi_nonzero);
bool test_roacc07_csrrsi_nonzero(void)
{
    TEST_BEGIN("ROACC-07: csrrsi rd, seed, 1 normal access");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read_csrrsi());
    printf("  [INFO] seed via csrrsi = 0x%lx\n", (unsigned long)val);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* ROACC-08: write value is ignored                                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_roacc08_write_ignored);
bool test_roacc08_write_ignored(void)
{
    TEST_BEGIN("ROACC-08: write value is ignored by implementation");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    /* Write a distinctive pattern */
    M_EXPECT_NO_TRAP({
        (void)seed_write(0xDEADBEEFUL);
    });

    /* Read twice and verify write value is ignored */
    uintptr_t val1 = 0, val2 = 0;
    M_EXPECT_NO_TRAP(val1 = seed_read());
    M_EXPECT_NO_TRAP(val2 = seed_read());

    printf("  [INFO] write=0xDEADBEEF, read1=0x%lx, read2=0x%lx\n",
           (unsigned long)val1, (unsigned long)val2);

    /* Both reads must have valid OPST */
    uintptr_t opst1 = val1 & SEED_OPST_MASK;
    uintptr_t opst2 = val2 & SEED_OPST_MASK;
    bool valid_opst = (opst1 == SEED_OPST_BIST || opst1 == SEED_OPST_WAIT ||
                       opst1 == SEED_OPST_ES16 || opst1 == SEED_OPST_DEAD) &&
                      (opst2 == SEED_OPST_BIST || opst2 == SEED_OPST_WAIT ||
                       opst2 == SEED_OPST_ES16 || opst2 == SEED_OPST_DEAD);
    TEST_ASSERT("both reads have valid OPST", valid_opst);

    /* Written value (0xDEADBEEF) must not leak into entropy field */
    uintptr_t entropy1 = val1 & SEED_ENTROPY_MASK;
    uintptr_t entropy2 = val2 & SEED_ENTROPY_MASK;
    TEST_ASSERT("write value does not leak into entropy (read1)",
                entropy1 != 0xBEEF);
    TEST_ASSERT("write value does not leak into entropy (read2)",
                entropy2 != 0xBEEF);

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* ROACC-09: csrrw with non-zero rs1 does not affect entropy          */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_roacc09_csrrw_nonzero_rs1);
bool test_roacc09_csrrw_nonzero_rs1(void)
{
    TEST_BEGIN("ROACC-09: csrrw write 0xFFFFFFFF does not affect entropy");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_write(0xFFFFFFFFUL));

    uintptr_t opst = val & SEED_OPST_MASK;
    bool valid_opst = (opst == SEED_OPST_BIST || opst == SEED_OPST_WAIT ||
                       opst == SEED_OPST_ES16 || opst == SEED_OPST_DEAD);
    TEST_ASSERT("returned old value has valid OPST", valid_opst);

    /* Written value 0xFFFFFFFF must not leak into entropy field */
    uintptr_t entropy = val & SEED_ENTROPY_MASK;
    TEST_ASSERT("write 0xFFFFFFFF does not leak into entropy",
                entropy != 0xFFFF);

    printf("  [INFO] old seed = 0x%lx\n", (unsigned long)val);

    TEST_END();
}
