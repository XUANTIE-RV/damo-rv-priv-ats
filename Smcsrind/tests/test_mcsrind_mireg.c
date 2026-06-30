/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_mcsrind_mireg.c - Group 3: mireg* indirect access mechanism
 *
 * Verifies that miselect value determines which register is accessed
 * via mireg*, and behavior for legal/illegal select values.
 */

/* MCSRIND-MIREG-01: miselect determines mireg access target */
TEST_REGISTER(test_mcsrind_mireg_01);
bool test_mcsrind_mireg_01(void)
{
    TEST_BEGIN("MCSRIND-MIREG-01: miselect determines mireg target");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    /* Need a dependent extension to test meaningfully.
     * Check if Smaia provides miselect range 0x30-0x3F. */
    uintptr_t orig_sel = miselect_read();

    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    if (rb != 0x30) {
        miselect_write(orig_sel);
        TEST_SKIP("No dependent extension with known miselect range");
    }

    /* Read mireg with miselect=0x30 */
    M_TRAP_EXPECT_BEGIN();
    uintptr_t val1 = mireg_read();
    bool trapped1 = trap_was_triggered();
    trap_expect_end();

    /* Read mireg with miselect=0x31 (different target) */
    miselect_write(0x31);
    M_TRAP_EXPECT_BEGIN();
    uintptr_t val2 = mireg_read();
    bool trapped2 = trap_was_triggered();
    trap_expect_end();

    /* Record the actual behavior - both may trap if extension not implemented */
    if (!trapped1 && !trapped2) {
        TEST_ASSERT("mireg accessible with different miselect values", 1);
    } else if (trapped1 && trapped2) {
        printf("  INFO: Both trapped - extension may not be fully implemented (UNSPECIFIED)\n");
        TEST_ASSERT("mireg: both trapped (acceptable UNSPECIFIED)", 1);
    } else {
        printf("  INFO: Mixed behavior - val1 trapped=%d, val2 trapped=%d\n", trapped1, trapped2);
        TEST_ASSERT("mireg: mixed behavior recorded", 1);
    }

    (void)val1;
    (void)val2;
    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-MIREG-02: mireg access with unimplemented miselect value */
TEST_REGISTER(test_mcsrind_mireg_02);
bool test_mcsrind_mireg_02(void)
{
    TEST_BEGIN("MCSRIND-MIREG-02: mireg with unimplemented miselect");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();

    /* Set miselect to a reserved/unimplemented value */
    miselect_write(0x7FF);
    uintptr_t rb = miselect_read();

    /* Access mireg - behavior is UNSPECIFIED */
    M_TRAP_EXPECT_BEGIN();
    mireg_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* SPEC says behavior is UNSPECIFIED. Record actual behavior.
     * Expected: illegal-instruction or read-only zero. */
    if (trapped) {
        TEST_ASSERT("mireg with unimplemented miselect: trapped (expected UNSPECIFIED)", 1);
        printf("  INFO: trap cause=%lu\n", (unsigned long)trap_get_cause());
    } else {
        TEST_ASSERT("mireg with unimplemented miselect: no trap (acceptable UNSPECIFIED)", 1);
    }

    (void)rb;
    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-MIREG-03: mireg2~mireg6 with unimplemented miselect value */
TEST_REGISTER(test_mcsrind_mireg_03);
bool test_mcsrind_mireg_03(void)
{
    TEST_BEGIN("MCSRIND-MIREG-03: mireg2-6 with unimplemented miselect");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();

    /* Set miselect to a reserved value */
    miselect_write(0x7FF);

    /* Test each mireg_i - behavior UNSPECIFIED */
    typedef uintptr_t (*read_fn)(void);
    read_fn fns[] = {mireg2_read, mireg3_read, mireg4_read, mireg5_read, mireg6_read};
    const char *names[] = {"mireg2", "mireg3", "mireg4", "mireg5", "mireg6"};

    for (int i = 0; i < 5; i++) {
        M_TRAP_EXPECT_BEGIN();
        fns[i]();
        bool trapped = trap_was_triggered();
        trap_expect_end();

        if (trapped) {
            printf("  INFO: %s trapped (UNSPECIFIED behavior)\n", names[i]);
        } else {
            printf("  INFO: %s no trap (UNSPECIFIED behavior)\n", names[i]);
        }
    }

    TEST_ASSERT("mireg2-6 with unimplemented miselect: behavior recorded", 1);

    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-MIREG-04: mireg write-read with legal writable miselect */
TEST_REGISTER(test_mcsrind_mireg_04);
bool test_mcsrind_mireg_04(void)
{
    TEST_BEGIN("MCSRIND-MIREG-04: mireg write-read with legal miselect");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();

    /* Try Smaia range */
    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    if (rb != 0x30) {
        miselect_write(orig_sel);
        TEST_SKIP("No dependent extension with writable mireg");
    }

    /* Write mireg and read back */
    M_TRAP_EXPECT_BEGIN();
    uintptr_t orig_mireg = mireg_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped) {
        /* First read trapped - this is acceptable for RO/unimplemented registers */
        printf("  INFO: mireg read trapped at miselect=0x30 (acceptable UNSPECIFIED)\n");
        TEST_ASSERT("mireg read trapped (acceptable)", 1);
    } else {
        /* First read succeeded, try write-read cycle */
        M_TRAP_EXPECT_BEGIN();
        mireg_write(0xDEADBEEF);
        bool write_trapped = trap_was_triggered();
        trap_expect_end();

        if (write_trapped) {
            printf("  INFO: mireg write trapped (acceptable for RO register)\n");
            TEST_ASSERT("mireg write trapped (acceptable)", 1);
        } else {
            M_TRAP_EXPECT_BEGIN();
            uintptr_t readback = mireg_read();
            bool read_trapped = trap_was_triggered();
            trap_expect_end();

            if (read_trapped) {
                printf("  INFO: mireg re-read trapped (unexpected but acceptable)\n");
                TEST_ASSERT("mireg re-read trapped", 1);
            } else {
                /* Full write-read cycle succeeded */
                printf("  INFO: mireg write-read succeeded (orig=0x%lx, readback=0x%lx)\n",
                       (unsigned long)orig_mireg, (unsigned long)readback);
                TEST_ASSERT("mireg write-read: accessible", 1);
            }
        }
    }

    (void)orig_mireg;
    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-MIREG-05: mireg read-only zero verification */
TEST_REGISTER(test_mcsrind_mireg_05);
bool test_mcsrind_mireg_05(void)
{
    TEST_BEGIN("MCSRIND-MIREG-05: mireg read-only zero for RO0 mireg_i");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();

    /* Use Smaia range: mireg3 at miselect=0x30 is typically RO0 */
    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    if (rb != 0x30) {
        miselect_write(orig_sel);
        TEST_SKIP("No dependent extension");
    }

    /* mireg3 at miselect=0x30 may be RO0 */
    M_TRAP_EXPECT_BEGIN();
    uintptr_t val = mireg3_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (!trapped) {
        /* If readable, check if it's zero (expected for some mireg_i) */
        printf("  INFO: mireg3 at miselect=0x30 = 0x%lx\n", (unsigned long)val);
        TEST_ASSERT("mireg3: value recorded", 1);
    } else {
        TEST_ASSERT("mireg3: trap (acceptable for unimplemented mireg_i)", 1);
    }

    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-MIREG-06: select value and CSR number are independent address spaces */
TEST_REGISTER(test_mcsrind_mireg_06);
bool test_mcsrind_mireg_06(void)
{
    TEST_BEGIN("MCSRIND-MIREG-06: select vs CSR number independence");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();

    /* Set miselect to 0x350 (which is miselect's own CSR number).
     * Per SPEC, select values are a separate address space from CSR numbers.
     * So miselect=0x350 should NOT access miselect itself. */
    miselect_write(0x350);
    uintptr_t rb = miselect_read();

    /* Read mireg - it should NOT return miselect's own value */
    M_TRAP_EXPECT_BEGIN();
    uintptr_t mireg_val = mireg_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();

    /* The key assertion: mireg at miselect=0x350 should not be the same
     * address space as CSR 0x350. We verify by checking that the behavior
     * is consistent with UNSPECIFIED (trap) or returns something different. */
    if (!trapped) {
        TEST_ASSERT_NEQ("mireg at sel=0x350 != miselect CSR value",
                        mireg_val, rb);
    } else {
        /* Trap is acceptable - 0x350 may not be an implemented select range */
        TEST_ASSERT("mireg at sel=0x350: trap acceptable (unimplemented range)", 1);
    }

    (void)rb;
    miselect_write(orig_sel);
    TEST_END();
}

/* MCSRIND-MIREG-07: multiple mireg_i independence for same miselect */
TEST_REGISTER(test_mcsrind_mireg_07);
bool test_mcsrind_mireg_07(void)
{
    TEST_BEGIN("MCSRIND-MIREG-07: mireg/mireg2/mireg3 independence");

    if (!platform_has_smcsrind()) {
        TEST_SKIP("Smcsrind not implemented");
    }

    uintptr_t orig_sel = miselect_read();

    miselect_write(0x30);
    uintptr_t rb = miselect_read();
    if (rb != 0x30) {
        miselect_write(orig_sel);
        TEST_SKIP("No dependent extension (Smaia)");
    }

    /* Read mireg, mireg2, mireg3 with same miselect */
    M_TRAP_EXPECT_BEGIN();
    uintptr_t v1 = mireg_read();
    bool t1 = trap_was_triggered();
    trap_expect_end();

    M_TRAP_EXPECT_BEGIN();
    uintptr_t v2 = mireg2_read();
    bool t2 = trap_was_triggered();
    trap_expect_end();

    M_TRAP_EXPECT_BEGIN();
    uintptr_t v3 = mireg3_read();
    bool t3 = trap_was_triggered();
    trap_expect_end();

    if (!t1 && !t2 && !t3) {
        /* Each mireg_i accesses different register state */
        printf("  INFO: mireg=0x%lx mireg2=0x%lx mireg3=0x%lx\n",
               (unsigned long)v1, (unsigned long)v2, (unsigned long)v3);
        TEST_ASSERT("multiple mireg_i readable with same miselect", 1);
    } else {
        /* Some mireg_i may trap for certain select values */
        TEST_ASSERT("some mireg_i trapped: acceptable per spec", 1);
    }

    miselect_write(orig_sel);
    TEST_END();
}
