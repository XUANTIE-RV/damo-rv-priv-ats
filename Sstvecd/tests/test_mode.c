/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_mode.c - Group 1: stvec.MODE writability
 *
 * Per SPEC/sstvecd.adoc:4-5 (norm:Sstvecd_mode_direct_writable):
 *   stvec.MODE must be capable of holding the value 0 (Direct).
 *
 * Sstvecd does NOT require Vectored mode (MODE=1) to be implemented;
 * MODE=2 and MODE=3 are reserved values whose WARL behavior must map
 * back to a legal value (0 or 1) -- they must NOT lock the field.
 *
 * All Group 1 tests run entirely in M-mode and only read/write stvec
 * via csr instructions; no VM / S-mode entry is needed.
 *
 * Test coverage:
 *   STVEC-MODE-01: MODE writable to 0 (Direct)
 *   STVEC-MODE-02: MODE 0 -> 1 -> 0 round-trip
 *   STVEC-MODE-03: MODE write reserved value 2 -> readback in {0, 1}
 *   STVEC-MODE-04: MODE write reserved value 3 -> readback in {0, 1}
 */

TEST_REGISTER(test_sstvecd_mode_01_direct_writable);
bool test_sstvecd_mode_01_direct_writable(void) {
    TEST_BEGIN("STVEC-MODE-01: stvec.MODE writable to 0 (Direct)");

    STVEC_SAVE();

    /* Write BASE=0, MODE=0 */
    stvec_write((uintptr_t)0);

    uintptr_t rb = stvec_read();
    TEST_ASSERT("stvec.MODE reads back as 0 (Direct)",
                (rb & STVEC_MODE_MASK) == STVEC_MODE_DIRECT);

    STVEC_RESTORE();
    TEST_END();
}

TEST_REGISTER(test_sstvecd_mode_02_round_trip);
bool test_sstvecd_mode_02_round_trip(void) {
    TEST_BEGIN("STVEC-MODE-02: stvec.MODE 0 -> 1 -> 0 round-trip");

    STVEC_SAVE();

    /* Step 1: write MODE=0, expect readback == 0 */
    stvec_write((uintptr_t)STVEC_MODE_DIRECT);
    uintptr_t rb1 = stvec_read() & STVEC_MODE_MASK;
    TEST_ASSERT("after write MODE=0, readback == 0",
                rb1 == STVEC_MODE_DIRECT);

    /* Step 2: write MODE=1 (Vectored). Sstvecd does NOT require
     * Vectored to be implemented. Either readback (0 or 1) is legal. */
    stvec_write((uintptr_t)STVEC_MODE_VECTORED);
    uintptr_t rb2 = stvec_read() & STVEC_MODE_MASK;
    TEST_ASSERT("after write MODE=1, readback in {0, 1}",
                rb2 == STVEC_MODE_DIRECT || rb2 == STVEC_MODE_VECTORED);
    if (rb2 == STVEC_MODE_VECTORED) {
        printf("    [INFO] platform implements Vectored mode\n");
    } else {
        printf("    [INFO] platform does NOT implement Vectored "
               "(MODE=1 -> 0)\n");
    }

    /* Step 3: write MODE=0 again, expect readback == 0 */
    stvec_write((uintptr_t)STVEC_MODE_DIRECT);
    uintptr_t rb3 = stvec_read() & STVEC_MODE_MASK;
    TEST_ASSERT("after write MODE=0 again, readback == 0",
                rb3 == STVEC_MODE_DIRECT);

    STVEC_RESTORE();
    TEST_END();
}

TEST_REGISTER(test_sstvecd_mode_03_reserved_value_2);
bool test_sstvecd_mode_03_reserved_value_2(void) {
    TEST_BEGIN("STVEC-MODE-03: stvec.MODE reserved value 2 maps to legal");

    STVEC_SAVE();

    /* Write BASE=0, MODE=2 (reserved) */
    stvec_write((uintptr_t)0x2);

    uintptr_t mode = stvec_read() & STVEC_MODE_MASK;
    TEST_ASSERT("MODE reserved value 2 maps to {0, 1} (WARL)",
                mode == STVEC_MODE_DIRECT || mode == STVEC_MODE_VECTORED);

    STVEC_RESTORE();
    TEST_END();
}

TEST_REGISTER(test_sstvecd_mode_04_reserved_value_3);
bool test_sstvecd_mode_04_reserved_value_3(void) {
    TEST_BEGIN("STVEC-MODE-04: stvec.MODE reserved value 3 maps to legal");

    STVEC_SAVE();

    /* Write BASE=0, MODE=3 (reserved) */
    stvec_write((uintptr_t)0x3);

    uintptr_t mode = stvec_read() & STVEC_MODE_MASK;
    TEST_ASSERT("MODE reserved value 3 maps to {0, 1} (WARL)",
                mode == STVEC_MODE_DIRECT || mode == STVEC_MODE_VECTORED);

    STVEC_RESTORE();
    TEST_END();
}
