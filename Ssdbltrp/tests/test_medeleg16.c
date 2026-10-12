/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_medeleg16.c - Group 8: medeleg[16] read-only zero tests
 */

/* MDEL-01: medeleg[16] is read-only zero */
TEST_REGISTER(test_medeleg16_01);
bool test_medeleg16_01(void)
{
    TEST_BEGIN("MDEL-01: medeleg[16] is read-only zero");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Try to set medeleg[16] */
    tsbi_csr_set(CSR_MEDELEG, MEDELEG_DOUBLE_TRAP);

    /* Read back medeleg */
    uint64_t medeleg;
    medeleg = tsbi_csr_read(CSR_MEDELEG);

    /* Clean up */
    tsbi_csr_write(CSR_MEDELEG, 0ULL);

    TEST_ASSERT("medeleg[16] read-only zero", (medeleg & MEDELEG_DOUBLE_TRAP) == 0);
    TEST_END();
}

/* MDEL-02 writes medeleg = all-1s, which would set medeleg[9] (ecall-from-S)
 * and break the T-SBI service channel if done via the proxy from S-mode (the
 * read-back ecall would then be delegated to S and loop).  Do the whole
 * write-read-restore inside one M-excursion with direct access, restoring
 * medeleg (bit 9 cleared) before returning to S. */
static uintptr_t _mdel02_allones_m(uintptr_t arg) {
    (void)arg;
    CSRW(medeleg, 0xFFFFFFFFFFFFFFFFULL);
    uintptr_t md = CSRR(medeleg);
    CSRW(medeleg, 0ULL);
    return md;
}

/* MDEL-02: medeleg all-1 write, bit 16 still zero */
TEST_REGISTER(test_medeleg16_02);
bool test_medeleg16_02(void)
{
    TEST_BEGIN("MDEL-02: medeleg all-1 write, bit 16 still zero");

    if (!check_ssdbltrp()) {
        TEST_SKIP("Ssdbltrp not implemented");
    }

    /* Write all-1s, read back, and restore -- all inside one M-excursion so
     * the transient medeleg[9]=1 never coincides with a T-SBI ecall. */
    uint64_t medeleg = run_in_priv(PRIV_M, _mdel02_allones_m, 0);

    LOG_D("medeleg after all-1 write = 0x%lx\n", (unsigned long)medeleg);

    TEST_ASSERT("medeleg[16] zero after all-1 write", (medeleg & MEDELEG_DOUBLE_TRAP) == 0);
    TEST_END();
}

/* MDEL-03: Double trap not delegated to S-mode */
TEST_REGISTER(test_medeleg16_03);
bool test_medeleg16_03(void)
{
    TEST_BEGIN("MDEL-03: Double trap not delegated to S-mode");
    TEST_SKIP("Not implemented yet (placeholder)");
}
