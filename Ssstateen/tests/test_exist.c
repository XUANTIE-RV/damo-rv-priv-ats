/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1: sstateen CSR Existence and Accessibility
 *
 * Spec anchors:
 *   norm:sstateen_rv64_csrs       — sstateen0-3 exist with S-mode
 *   norm:sstateen_bit_allocation  — bits allocated from bit 0 upward
 *
 * 6 tests: SS-EXIST-01 ~ SS-EXIST-06
 * =================================================================== */

/* ---- SS-EXIST-01: sstateen0 readable in S-mode ---- */

TEST_REGISTER(test_ss_exist_sstateen0_read);
bool test_ss_exist_sstateen0_read(void) {
    TEST_BEGIN("SS-EXIST-01: sstateen0 readable in S-mode");

    /* Ensure mstateen0.SE0=1 to allow S-mode access */
    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bit63(0, true);

    SS_TEST_SMODE_ALLOWED("S-mode read sstateen0", sstateen0_read());

    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-EXIST-02: sstateen0 writable in S-mode ---- */

TEST_REGISTER(test_ss_exist_sstateen0_write);
bool test_ss_exist_sstateen0_write(void) {
    TEST_BEGIN("SS-EXIST-02: sstateen0 writable in S-mode");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    mstateen_set_bit63(0, true);

    /* Write from S-mode should not trap */
    SS_TEST_SMODE_ALLOWED("S-mode write sstateen0",
                          sstateen0_write(sstateen0_read()));

    mstateen_write(0, saved_mstateen0);
    TEST_END();
}

/* ---- SS-EXIST-03: sstateen1 readable/writable in S-mode ---- */

TEST_REGISTER(test_ss_exist_sstateen1_rw);
bool test_ss_exist_sstateen1_rw(void) {
    TEST_BEGIN("SS-EXIST-03: sstateen1 readable/writable in S-mode");

    uintptr_t saved = mstateen_read(1);
    mstateen_set_bit63(1, true);

    if (!(mstateen_read(1) & STATEEN_BIT63)) {
        mstateen_write(1, saved);
        TEST_SKIP("mstateen1 bit 63 not writable (sstateen1 not accessible)");
    }

    SS_TEST_SMODE_ALLOWED("S-mode read/write sstateen1", {
        uintptr_t v = sstateen1_read();
        sstateen1_write(v);
    });

    mstateen_write(1, saved);
    TEST_END();
}

/* ---- SS-EXIST-04: sstateen2 readable/writable in S-mode ---- */

TEST_REGISTER(test_ss_exist_sstateen2_rw);
bool test_ss_exist_sstateen2_rw(void) {
    TEST_BEGIN("SS-EXIST-04: sstateen2 readable/writable in S-mode");

    uintptr_t saved = mstateen_read(2);
    mstateen_set_bit63(2, true);

    if (!(mstateen_read(2) & STATEEN_BIT63)) {
        mstateen_write(2, saved);
        TEST_SKIP("mstateen2 bit 63 not writable (sstateen2 not accessible)");
    }

    SS_TEST_SMODE_ALLOWED("S-mode read/write sstateen2", {
        uintptr_t v = sstateen2_read();
        sstateen2_write(v);
    });

    mstateen_write(2, saved);
    TEST_END();
}

/* ---- SS-EXIST-05: sstateen3 readable/writable in S-mode ---- */

TEST_REGISTER(test_ss_exist_sstateen3_rw);
bool test_ss_exist_sstateen3_rw(void) {
    TEST_BEGIN("SS-EXIST-05: sstateen3 readable/writable in S-mode");

    uintptr_t saved = mstateen_read(3);
    mstateen_set_bit63(3, true);

    if (!(mstateen_read(3) & STATEEN_BIT63)) {
        mstateen_write(3, saved);
        TEST_SKIP("mstateen3 bit 63 not writable (sstateen3 not accessible)");
    }

    SS_TEST_SMODE_ALLOWED("S-mode read/write sstateen3", {
        uintptr_t v = sstateen3_read();
        sstateen3_write(v);
    });

    mstateen_write(3, saved);
    TEST_END();
}

/* ---- SS-EXIST-06: sstateen0 is 32-bit effective width ---- */

TEST_REGISTER(test_ss_exist_sstateen0_width);
bool test_ss_exist_sstateen0_width(void) {
    TEST_BEGIN("SS-EXIST-06: sstateen0 is 32-bit effective width");

    uintptr_t saved_mstateen0 = mstateen_read(0);
    /* Allow all bits through from mstateen0 */
    mstateen_write(0, ~0UL);

    uintptr_t saved_sstateen0 = sstateen_read(0);

    /* Write all-1s to sstateen0 */
    sstateen_write(0, ~0UL);
    uintptr_t readback = sstateen_read(0);

    /* sstateen registers are 32 bits; high 32 bits should not exist.
     * Per spec, sstateen bit allocation is from bit 0 toward bit 31. */
#if __riscv_xlen == 64
    uintptr_t high_bits = readback & 0xFFFFFFFF00000000UL;
    TEST_ASSERT("sstateen0 high 32 bits are zero", high_bits == 0);
#endif
    printf("  sstateen0 readback after all-1 write: 0x%lx\n",
           (unsigned long)readback);

    sstateen_write(0, saved_sstateen0);
    mstateen_write(0, saved_mstateen0);
    TEST_END();
}
