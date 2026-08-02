/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_smode.c - Group 6: S-mode Access Control (SSEED)
 *
 * SACC-01 ~ SACC-09
 * Verifies SSEED field control over S-mode access to seed CSR.
 * Note: HS-mode and VS/VU-mode tests are in Hypervisor_cross_test_plan.md.
 */

/* ------------------------------------------------------------------ */
/* SACC-01: SSEED=0 S-mode csrrw triggers exception                   */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sacc01_sseed0_traps);
bool test_sacc01_sseed0_traps(void)
{
    TEST_BEGIN("SACC-01: SSEED=0 S-mode csrrw triggers illegal-instruction");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!sseed_writable()) {
        TEST_SKIP("SSEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED);

    goto_priv(PRIV_S);
    PRIV_DO(SMODE_SEED_CSRRW());
    goto_priv(PRIV_M);
    CHECK_TRAP("S-mode csrrw with SSEED=0", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SACC-02: SSEED=1 S-mode csrrw normal access                        */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sacc02_sseed1_ok);
bool test_sacc02_sseed1_ok(void)
{
    TEST_BEGIN("SACC-02: SSEED=1 S-mode csrrw normal access");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!sseed_writable()) {
        TEST_SKIP("SSEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    goto_priv(PRIV_S);
    PRIV_DO(SMODE_SEED_CSRRW());
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("S-mode csrrw with SSEED=1");

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SACC-03: SSEED=1 S-mode read-only access still triggers exception  */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sacc03_sseed1_ro_traps);
bool test_sacc03_sseed1_ro_traps(void)
{
    TEST_BEGIN("SACC-03: SSEED=1 S-mode csrrs x0 still triggers exception");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!sseed_writable()) {
        TEST_SKIP("SSEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    goto_priv(PRIV_S);
    PRIV_DO(SMODE_SEED_CSRRS_RO());
    goto_priv(PRIV_M);
    CHECK_TRAP("S-mode csrrs x0 with SSEED=1", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SACC-06: SSEED field writability probe                             */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sacc06_sseed_writable);
bool test_sacc06_sseed_writable(void)
{
    TEST_BEGIN("SACC-06: SSEED field writability probe");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);
    uintptr_t val = mseccfg_read_zkr();
    mseccfg_write_zkr(orig);

    if (val & MSECCFG_SSEED) {
        printf("  [INFO] SSEED is writable (Zkr + S-mode implemented)\n");
        TEST_ASSERT("SSEED writable", true);
    } else {
        printf("  [INFO] SSEED is read-only zero\n");
        TEST_ASSERT("SSEED read-only zero (acceptable)", true);
    }

    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SACC-07: SSEED=1 S-mode csrrci uimm=0 triggers exception           */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sacc07_sseed1_csrrci0_traps);
bool test_sacc07_sseed1_csrrci0_traps(void)
{
    TEST_BEGIN("SACC-07: SSEED=1 S-mode csrrci uimm=0 triggers exception");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!sseed_writable()) {
        TEST_SKIP("SSEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_set_bits(MSECCFG_SSEED);

    goto_priv(PRIV_S);
    PRIV_DO(SMODE_SEED_CSRRCI_RO());
    goto_priv(PRIV_M);
    CHECK_TRAP("S-mode csrrci 0 with SSEED=1", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SACC-08: SSEED=0 S-mode any CSR instruction triggers exception     */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sacc08_sseed0_all_trap);
bool test_sacc08_sseed0_all_trap(void)
{
    TEST_BEGIN("SACC-08: SSEED=0 S-mode all CSR instructions trap");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }
    if (!sseed_writable()) {
        TEST_SKIP("SSEED is read-only zero");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED);

    /* csrrw should trap */
    goto_priv(PRIV_S);
    PRIV_DO(SMODE_SEED_CSRRW());
    goto_priv(PRIV_M);
    CHECK_TRAP("S-mode csrrw with SSEED=0", CAUSE_ILLEGAL_INST);

    /* csrrs (rs1!=x0) should also trap */
    goto_priv(PRIV_S);
    PRIV_DO(SMODE_SEED_CSRRS_RW());
    goto_priv(PRIV_M);
    CHECK_TRAP("S-mode csrrs rs1!=x0 with SSEED=0", CAUSE_ILLEGAL_INST);

    /* csrrs (rs1=x0) should also trap */
    goto_priv(PRIV_S);
    PRIV_DO(SMODE_SEED_CSRRS_RO());
    goto_priv(PRIV_M);
    CHECK_TRAP("S-mode csrrs x0 with SSEED=0", CAUSE_ILLEGAL_INST);

    /* csrrsi (uimm=0) should also trap */
    goto_priv(PRIV_S);
    PRIV_DO(SMODE_SEED_CSRRSI_RO());
    goto_priv(PRIV_M);
    CHECK_TRAP("S-mode csrrsi 0 with SSEED=0", CAUSE_ILLEGAL_INST);

    /* csrrci (uimm=0) should also trap */
    goto_priv(PRIV_S);
    PRIV_DO(SMODE_SEED_CSRRCI_RO());
    goto_priv(PRIV_M);
    CHECK_TRAP("S-mode csrrci 0 with SSEED=0", CAUSE_ILLEGAL_INST);

    mseccfg_write_zkr(orig);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* SACC-09: SSEED does not affect M-mode access                       */
/* ------------------------------------------------------------------ */
TEST_REGISTER(test_sacc09_sseed_no_mmode_effect);
bool test_sacc09_sseed_no_mmode_effect(void)
{
    TEST_BEGIN("SACC-09: SSEED does not affect M-mode access");

    if (!check_zkr()) {
        TEST_SKIP("Zkr not implemented");
    }

    uintptr_t orig = mseccfg_read_zkr();
    mseccfg_clear_bits(MSECCFG_SSEED);

    uintptr_t val = 0;
    M_EXPECT_NO_TRAP(val = seed_read());
    printf("  [INFO] M-mode seed = 0x%lx (SSEED=0)\n", (unsigned long)val);

    mseccfg_write_zkr(orig);
    TEST_END();
}
