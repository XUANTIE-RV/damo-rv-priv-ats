/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_base.c - Group 2: stvec.BASE holding capacity under MODE=Direct
 *
 * Per SPEC/sstvecd.adoc:5-6 (norm:Sstvecd_base_holds_any_4byte_aligned):
 *   When stvec.MODE=Direct, stvec.BASE must be capable of holding any
 *   valid four-byte-aligned address.
 *
 * Per SPEC/supervisor.adoc:335-338 (norm:stvec_sz_base):
 *   The CSR holds bits XLEN-1 through 2 of BASE; the low two bits are
 *   always read as zero (and on write, are the MODE field).
 *
 * All Group 2 tests run entirely in M-mode; we never actually take a
 * trap with these BASE values, so writing arbitrary high addresses is
 * safe (no fetch ever happens at those PCs during Group 2).
 *
 * Test coverage:
 *   STVEC-BASE-01: BASE = 0x0
 *   STVEC-BASE-02: BASE = PLATFORM_MEM_BASE (already 4-aligned)
 *   STVEC-BASE-03: BASE = 0x40000004 (cross 1 GiB boundary)
 *   STVEC-BASE-04: BASE = 0x8000000000 (bit 39, hard probe of Sstvecd)
 *   STVEC-BASE-05: BASE = 0x40000003 (low 2 bits land in MODE field)
 *   STVEC-BASE-06: MODE=0 fixed, sweep BASE in {0x1000,0x2000,0x4000,0x8000}
 *   STVEC-BASE-07: high-bit scan k in {12,16,20,24,28,32,36,38}
 */

TEST_REGISTER(test_sstvecd_base_01_zero);
bool test_sstvecd_base_01_zero(void) {
    TEST_BEGIN("STVEC-BASE-01: BASE = 0x0");

    STVEC_SAVE();

    stvec_write((uintptr_t)0x0);
    uintptr_t rb = stvec_read();
    TEST_ASSERT("readback == 0x0", rb == 0x0UL);

    STVEC_RESTORE();
    TEST_END();
}

TEST_REGISTER(test_sstvecd_base_02_platform_mem_base);
bool test_sstvecd_base_02_platform_mem_base(void) {
    TEST_BEGIN("STVEC-BASE-02: BASE = PLATFORM_MEM_BASE");

    STVEC_SAVE();

    /* PLATFORM_MEM_BASE is page-aligned (>= 4 KiB), so trivially
     * 4-byte aligned and falls in the standard memory range. */
    uintptr_t target = (uintptr_t)PLATFORM_MEM_BASE;
    TEST_ASSERT("PLATFORM_MEM_BASE is 4-byte aligned",
                (target & STVEC_MODE_MASK) == 0);

    stvec_write(target);
    uintptr_t rb = stvec_read();
    TEST_ASSERT("BASE field == PLATFORM_MEM_BASE",
                (rb & STVEC_BASE_MASK) == target);
    TEST_ASSERT("MODE field == 0 (Direct)",
                (rb & STVEC_MODE_MASK) == STVEC_MODE_DIRECT);

    STVEC_RESTORE();
    TEST_END();
}

TEST_REGISTER(test_sstvecd_base_03_cross_1g);
bool test_sstvecd_base_03_cross_1g(void) {
    TEST_BEGIN("STVEC-BASE-03: BASE = 0x40000004 (cross 1 GiB boundary)");

    STVEC_SAVE();

    uintptr_t target = 0x40000004UL;        /* 1 GiB + 4, 4-byte aligned */
    stvec_write(target);
    uintptr_t rb = stvec_read();
    TEST_ASSERT("readback == 0x40000004 (BASE preserved, MODE=0)",
                rb == target);

    STVEC_RESTORE();
    TEST_END();
}

TEST_REGISTER(test_sstvecd_base_04_high_bit_39);
bool test_sstvecd_base_04_high_bit_39(void) {
    TEST_BEGIN("STVEC-BASE-04: BASE = 0x8000000000 (bit 39, Sstvecd probe)");

    STVEC_SAVE();

    /* Hard probe: a Sstvecd-compliant impl must hold any valid
     * 4-byte aligned address. If the BASE field WARL-truncates the
     * high bits (e.g. impl limits to bits [38:2]), this assertion
     * fails -- which is itself the diagnostic that Sstvecd is not
     * really enabled on this platform. */
    uintptr_t target = 1UL << 39;           /* 0x8000000000 */
    stvec_write(target);
    uintptr_t rb = stvec_read();
    TEST_ASSERT("readback == 0x8000000000 "
                "(high bit preserved -> Sstvecd active)",
                rb == target);

    STVEC_RESTORE();
    TEST_END();
}

TEST_REGISTER(test_sstvecd_base_05_unaligned_write);
bool test_sstvecd_base_05_unaligned_write(void) {
    TEST_BEGIN("STVEC-BASE-05: BASE write 0x40000003 -> low 2 bits = MODE");

    STVEC_SAVE();

    /* Writing 0x40000003: BASE field gets 0x40000000; low [1:0]=0b11
     * is the MODE field which is reserved value 3 -- by WARL must
     * map back to {0, 1}. */
    stvec_write((uintptr_t)0x40000003UL);
    uintptr_t rb = stvec_read();

    TEST_ASSERT("BASE field preserved == 0x40000000",
                (rb & STVEC_BASE_MASK) == 0x40000000UL);

    uintptr_t mode = rb & STVEC_MODE_MASK;
    TEST_ASSERT("MODE reserved value 3 maps to {0, 1}",
                mode == STVEC_MODE_DIRECT || mode == STVEC_MODE_VECTORED);

    STVEC_RESTORE();
    TEST_END();
}

TEST_REGISTER(test_sstvecd_base_06_independent_of_mode);
bool test_sstvecd_base_06_independent_of_mode(void) {
    TEST_BEGIN("STVEC-BASE-06: BASE rewrites do not disturb MODE=Direct");

    STVEC_SAVE();

    /* Pin MODE=Direct and rewrite BASE several times. */
    static const uintptr_t bases[] = {
        0x1000UL, 0x2000UL, 0x4000UL, 0x8000UL,
    };
    const unsigned n = sizeof(bases) / sizeof(bases[0]);

    for (unsigned i = 0; i < n; i++) {
        uintptr_t target = bases[i] | STVEC_MODE_DIRECT;
        stvec_write(target);
        uintptr_t rb = stvec_read();
        TEST_ASSERT("BASE preserved across rewrite",
                    (rb & STVEC_BASE_MASK) == bases[i]);
        TEST_ASSERT("MODE remains Direct (=0)",
                    (rb & STVEC_MODE_MASK) == STVEC_MODE_DIRECT);
    }

    STVEC_RESTORE();
    TEST_END();
}

TEST_REGISTER(test_sstvecd_base_07_high_bit_scan);
bool test_sstvecd_base_07_high_bit_scan(void) {
    TEST_BEGIN("STVEC-BASE-07: high-bit scan k in {12..38}");

    STVEC_SAVE();

    /* Scan k from 12 (4 KiB) up to 38 (just below 1<<39, Sv39 limit).
     * For each k, write 1<<k as BASE (already 4-byte aligned for k>=2)
     * and verify exact readback. */
    static const int ks[] = { 12, 16, 20, 24, 28, 32, 36, 38 };
    const unsigned n = sizeof(ks) / sizeof(ks[0]);

    for (unsigned i = 0; i < n; i++) {
        int k = ks[i];
        uintptr_t target = 1UL << k;        /* 4-byte aligned by construction */
        stvec_write(target);
        uintptr_t rb = stvec_read();
        if (rb != target) {
            printf("    [DETAIL] k=%d wrote 0x%lx readback 0x%lx\n",
                   k, (unsigned long)target, (unsigned long)rb);
        }
        TEST_ASSERT("readback == 1<<k (Sstvecd holds any 4-aligned BASE)",
                    rb == target);
    }

    STVEC_RESTORE();
    TEST_END();
}
