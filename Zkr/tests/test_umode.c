/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_umode.c - Group 5: U-mode Access Control (USEED)
 *
 * UACC-01 ~ UACC-07
 * Verifies USEED field control over U-mode access to seed CSR.
 */

/* ------------------------------------------------------------------ */
/* UACC-01: USEED=0 U-mode csrrw triggers exception                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_uacc01_useed0_traps);
bool test_uacc01_useed0_traps(void)
{
    TEST_BEGIN("UACC-01: USEED=0 U-mode csrrw triggers illegal-instruction");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!useed_writable()) {
        TEST_SKIP("USEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_USEED);

    goto_priv(PRIV_U);
    PRIV_DO(UMODE_SEED_CSRRW());
    goto_priv(PRIV_M);
    CHECK_TRAP("U-mode csrrw with USEED=0", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* UACC-02: USEED=1 U-mode csrrw normal access                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_uacc02_useed1_ok);
bool test_uacc02_useed1_ok(void)
{
    TEST_BEGIN("UACC-02: USEED=1 U-mode csrrw normal access");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!useed_writable()) {
        TEST_SKIP("USEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_USEED);

    goto_priv(PRIV_U);
    PRIV_DO(UMODE_SEED_CSRRW());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("U-mode csrrw with USEED=1");

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* UACC-03: USEED=1 U-mode read-only access still triggers exception  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_uacc03_useed1_ro_traps);
bool test_uacc03_useed1_ro_traps(void)
{
    TEST_BEGIN("UACC-03: USEED=1 U-mode csrrs x0 still triggers exception");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!useed_writable()) {
        TEST_SKIP("USEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_USEED);

    goto_priv(PRIV_U);
    PRIV_DO(UMODE_SEED_CSRRS_RO());
    goto_priv(PRIV_M);
    CHECK_TRAP("U-mode csrrs x0 with USEED=1", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* UACC-04: USEED=1 U-mode csrrsi uimm=0 triggers exception           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_uacc04_useed1_csrrsi0_traps);
bool test_uacc04_useed1_csrrsi0_traps(void)
{
    TEST_BEGIN("UACC-04: USEED=1 U-mode csrrsi uimm=0 triggers exception");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!useed_writable()) {
        TEST_SKIP("USEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_USEED);

    goto_priv(PRIV_U);
    PRIV_DO(UMODE_SEED_CSRRSI_RO());
    goto_priv(PRIV_M);
    CHECK_TRAP("U-mode csrrsi 0 with USEED=1", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* UACC-05: USEED field writability probe                             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_uacc05_useed_writable);
bool test_uacc05_useed_writable(void)
{
    TEST_BEGIN("UACC-05: USEED field writability probe");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_USEED);
    uintptr_t val = mseccfg_read_zkr();
    mseccfg_write_zkr(orig);

    if (val & MSECCFG_USEED) {
        printf("  [INFO] USEED is writable (Zkr + U-mode implemented)\n");
        TEST_ASSERT("USEED writable", true);
    } else {
        printf("  [INFO] USEED is read-only zero\n");
        TEST_ASSERT("USEED read-only zero (acceptable)", true);
    }

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* UACC-06: USEED=0 U-mode csrrs (rs1!=x0) triggers exception         */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_uacc06_useed0_csrrs_nonzero);
bool test_uacc06_useed0_csrrs_nonzero(void)
{
    TEST_BEGIN("UACC-06: USEED=0 U-mode csrrs (rs1!=x0) triggers exception");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!useed_writable()) {
        TEST_SKIP("USEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_USEED);

    goto_priv(PRIV_U);
    PRIV_DO(UMODE_SEED_CSRRS_RW());
    goto_priv(PRIV_M);
    CHECK_TRAP("U-mode csrrs rs1!=x0 with USEED=0", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* UACC-07: USEED does not affect M-mode access                       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_uacc07_useed_no_mmode_effect);
bool test_uacc07_useed_no_mmode_effect(void)
{
    TEST_BEGIN("UACC-07: USEED does not affect M-mode access");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_USEED);

    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read());
    printf("  [INFO] M-mode seed = 0x%lx (USEED=0)\n", (unsigned long)val);

    mseccfg_write_zkr(orig);
    TEST_END();
}
