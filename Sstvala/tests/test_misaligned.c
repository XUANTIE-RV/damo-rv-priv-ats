/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_misaligned.c - Group 3: Misaligned address-type exceptions
 *
 * Per Sstvala (SPEC/sstvala.adoc:4-6):
 *   For load/store/instruction misaligned exceptions, stval must
 *   contain the faulting virtual address.
 *
 * Tests:
 *   TVAL-LMA-01: load misaligned (platform-dependent, may SKIP)
 *   TVAL-SMA-01: store misaligned (platform-dependent, may SKIP)
 *   TVAL-IMA-01: instruction misaligned (jump to odd address)
 *
 * NOTE: Most modern RISC-V implementations (QEMU, Spike) support
 * hardware misaligned load/store, so TVAL-LMA-01 and TVAL-SMA-01
 * will be skipped on those platforms. TVAL-IMA-01 (instruction
 * misaligned) always triggers on non-2-byte-aligned PC.
 */

/* ===================================================================
 * TVAL-LMA-01: load misaligned (non-8-byte-aligned ld)
 * =================================================================== */
TEST_REGISTER(test_sstvala_lma_01);
bool test_sstvala_lma_01(void) {
    TEST_BEGIN("TVAL-LMA-01: load misaligned stval == faulting addr");

    /* Probe in M-mode first: does the platform trap on misaligned load? */
    static volatile uint64_t test_buf[2] = {0x1122334455667788ULL, 0};
    uintptr_t misaligned_addr = ((uintptr_t)test_buf) + 3;

    trap_expect_begin();
    volatile uint64_t val = *(volatile uint64_t *)misaligned_addr;
    (void)val;

    if (!trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("platform supports hardware misaligned load");
    }
    trap_expect_end();

    /* Platform traps on misaligned load — now test in S-mode for stval */
    ensure_smode_pmp();
    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_load64(misaligned_addr));
    goto_priv(PRIV_M);

    CHECK_TRAP("load misaligned trap", CAUSE_LOAD_ADDR_MISALIGN);
    TEST_ASSERT_EQ("stval == misaligned address",
                   trap_get_tval(), misaligned_addr);

    TEST_END();
}

/* ===================================================================
 * TVAL-SMA-01: store misaligned (non-8-byte-aligned sd)
 * =================================================================== */
TEST_REGISTER(test_sstvala_sma_01);
bool test_sstvala_sma_01(void) {
    TEST_BEGIN("TVAL-SMA-01: store misaligned stval == faulting addr");

    /* Probe in M-mode first: does the platform trap on misaligned store? */
    static volatile uint64_t store_buf[2] = {0, 0};
    uintptr_t misaligned_addr = ((uintptr_t)store_buf) + 5;

    trap_expect_begin();
    *(volatile uint64_t *)misaligned_addr = 0xDEADBEEFULL;

    if (!trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("platform supports hardware misaligned store");
    }
    trap_expect_end();

    /* Platform traps on misaligned store — now test in S-mode for stval */
    ensure_smode_pmp();
    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_store64(misaligned_addr, 0xDEADBEEFULL));
    goto_priv(PRIV_M);

    CHECK_TRAP("store misaligned trap", CAUSE_STORE_ADDR_MISALIGN);
    TEST_ASSERT_EQ("stval == misaligned address",
                   trap_get_tval(), misaligned_addr);

    TEST_END();
}

/* ===================================================================
 * TVAL-IMA-01: instruction misaligned stval == faulting addr
 *
 * Trigger an instruction address misaligned exception (cause=0).
 *
 * On platforms WITHOUT the C extension (IALIGN=4), a jump to a
 * 2-byte-aligned but non-4-byte-aligned address triggers cause 0.
 * We use jalr to an address with bit 1 set (but bit 0 clear).
 *
 * On platforms WITH the C extension (IALIGN=2), instruction address
 * misaligned (cause 0) can only occur when bit 0 of the target PC
 * is set. However, the RISC-V spec mandates that jalr clears bit 0
 * of the computed target (PC = (rs1+imm) & ~1), and mepc is a WARL
 * field that may similarly clear bit 0. Therefore, on C-extension
 * platforms it is architecturally impossible to trigger cause 0, and
 * this test must be skipped.
 * =================================================================== */
/*
 * S-mode helper: jump to a misaligned target address via jalr.
 * arg = target address with bit 1 set (2-byte-aligned, non-4-byte-aligned).
 * Returns trap cause on fault, 0 on success.
 */
static uintptr_t smode_jalr_misaligned(uintptr_t arg) {
    trap_expect_begin();
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
        "la   t0, 1f\n\t"
        "la   t1, _exec_return_addr\n\t"
        STORE " t0, 0(t1)\n\t"
        "jalr zero, %0, 0\n\t"
        "1:\n\t"
        STORE " zero, 0(t1)\n\t"
        ".option pop\n\t"
        :
        : "r"(arg)
        : "t0", "t1", "memory"
    );
    trap_expect_end();

    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

TEST_REGISTER(test_sstvala_ima_01);
bool test_sstvala_ima_01(void) {
    TEST_BEGIN("TVAL-IMA-01: instruction misaligned stval == faulting addr");

    /*
     * Detect whether the C extension is present by reading misa.
     * misa bit 2 (C) indicates compressed instruction support.
     * When C is enabled, IALIGN=2 and only bit-0 misalignment
     * would fault — but jalr masks bit 0, making it untriggerable.
     */
    uintptr_t misa;
    asm volatile ("csrr %0, misa" : "=r"(misa));
    if (misa & (1UL << 2)) {
        TEST_SKIP("C extension present (IALIGN=2); "
                  "instruction misaligned untriggerable via jalr");
    }

    /*
     * No C extension: IALIGN=4.
     * jalr to a 2-byte-aligned but non-4-byte-aligned address
     * triggers cause 0.  jalr clears bit 0 but preserves bit 1,
     * so target with bit 1 set will fault.
     */
    extern void _start(void);
    uintptr_t target = ((uintptr_t)&_start) | 0x2UL;

    /* Execute the misaligned jalr in S-mode so that the S-mode trap
     * handler captures stval (not mtval). */
    uintptr_t result = run_in_priv(PRIV_S, smode_jalr_misaligned, target);

    TEST_ASSERT_EQ("cause == instruction address misaligned",
                   result, (uintptr_t)CAUSE_INST_ADDR_MISALIGN);
    TEST_ASSERT_EQ("stval == misaligned target address",
                   trap_get_tval(), target);

    TEST_END();
}
