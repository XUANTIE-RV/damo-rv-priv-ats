/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 8:  vsstatus csr
 *
 * Tests VSST-01 through VSST-12 verify vsstatus register behavior.
 */

#include "test_helpers.h"

/* ===================================================================
 * Local constants
 * =================================================================== */

/* sstatus / vsstatus field positions and values */
#define SSTATUS_SIE_BIT    (1UL << 1)
#define SSTATUS_FS_SHIFT   13
#define SSTATUS_FS_MASK    (3UL << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_OFF     (0UL << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_INITIAL (1UL << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_CLEAN   (2UL << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_DIRTY   (3UL << SSTATUS_FS_SHIFT)
#define SSTATUS_VS_SHIFT   9
#define SSTATUS_VS_MASK    (3UL << SSTATUS_VS_SHIFT)
#define SSTATUS_VS_OFF     (0UL << SSTATUS_VS_SHIFT)
#define SSTATUS_VS_INITIAL (1UL << SSTATUS_VS_SHIFT)
#define SSTATUS_VS_DIRTY   (3UL << SSTATUS_VS_SHIFT)
#define SSTATUS_SD_BIT     (1UL << 63)
#define SSTATUS_UXL_SHIFT  32
#define SSTATUS_UXL_MASK   (3UL << SSTATUS_UXL_SHIFT)

/* misa extension bits for platform capability detection */
#define MISA_F  (1UL << ('F' - 'A'))
#define MISA_V  (1UL << ('V' - 'A'))

/* ===================================================================
 * Platform capability detection
 * =================================================================== */

static bool platform_has_f_ext(void)
{
    uintptr_t misa;
    asm volatile ("csrr %0, misa" : "=r"(misa));
    return (misa & MISA_F) != 0;
}

static bool platform_has_v_ext(void)
{
    uintptr_t misa;
    asm volatile ("csrr %0, misa" : "=r"(misa));
    return (misa & MISA_V) != 0;
}

/* ===================================================================
 * VS-mode trampoline: floating-point instruction
 *
 * fadd.s f0, f0, f0 — modifies f0 to trigger FS dirty.
 * Encoded as raw .4byte to avoid -march dependency.
 * =================================================================== */
static uintptr_t vs_exec_fp_inst(uintptr_t arg)
{
    (void)arg;
    asm volatile (".4byte 0x00000053" ::: "memory");
    return 0;
}

/* ===================================================================
 * VS-mode trampoline: vector instruction
 *
 * vsetvli t0, zero, e8, m1, ta, ma — touches vector config state.
 * Encoded as raw .4byte to avoid -march dependency.
 * =================================================================== */
static uintptr_t vs_exec_vector_inst(uintptr_t arg)
{
    (void)arg;
    asm volatile (".4byte 0x0C0072D7" ::: "memory");
    return 0;
}

/* ===================================================================
 * VSST-01: vsstatus basic read/write
 *
 * Verify vsstatus can be written and read back from M-mode.
 * =================================================================== */
TEST_REGISTER(vsst_01_basic_rw);
bool vsst_01_basic_rw(void)
{
    TEST_BEGIN("VSST-01: vsstatus basic read/write");

    uintptr_t test_val = VSSTATUS_WRITABLE_MASK;

    /* Write vsstatus (CSR 0x200). */
    asm volatile ("csrw 0x200, %0" :: "r"(test_val));

    /* Read back and verify writable fields. */
    uintptr_t readback;
    asm volatile ("csrr %0, 0x200" : "=r"(readback));

    uintptr_t hw_writable = readback & VSSTATUS_WRITABLE_MASK;

    /* Write zero and verify all hardware-writable bits clear. */
    asm volatile ("csrw 0x200, zero");
    asm volatile ("csrr %0, 0x200" : "=r"(readback));

    TEST_ASSERT_EQ("vsstatus cleared to zero",
                   readback & hw_writable, 0UL);

    /* Re-write and verify consistent readback (WARL stability). */
    asm volatile ("csrw 0x200, %0" :: "r"(test_val));
    asm volatile ("csrr %0, 0x200" : "=r"(readback));

    TEST_ASSERT_EQ("vsstatus writable fields consistent readback",
                   readback & VSSTATUS_WRITABLE_MASK, hw_writable);

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-02: V=1 sstatus accesses vsstatus (substitution)
 *
 * Cross-reference: VCSR-01/02. Verify that VS-mode sstatus access
 * actually reads/writes vsstatus.
 * =================================================================== */
TEST_REGISTER(vsst_02_substitution);
bool vsst_02_substitution(void)
{
    TEST_BEGIN("VSST-02: V=1 sstatus accesses vsstatus");

    /* Write a known value to vsstatus from M-mode. */
    uintptr_t test_val = (1UL << 5) | (1UL << 19);  /* SPIE | MXR */
    asm volatile ("csrw 0x200, %0" :: "r"(test_val));

    uintptr_t written;
    asm volatile ("csrr %0, 0x200" : "=r"(written));

    /* VS-mode reads sstatus — should return vsstatus value. */
    uintptr_t vs_read = run_in_vs_mode(vs_read_sstatus, 0);

    TEST_ASSERT_EQ("VS sstatus read matches vsstatus",
                   vs_read & VSSTATUS_WRITABLE_MASK,
                   written & VSSTATUS_WRITABLE_MASK);

    /* VS-mode writes sstatus, verify vsstatus is modified. */
    asm volatile ("csrw 0x200, zero");
    uintptr_t write_val = (1UL << 18);  /* SUM */
    run_in_vs_mode(vs_write_sstatus, write_val);

    uintptr_t readback;
    asm volatile ("csrr %0, 0x200" : "=r"(readback));

    TEST_ASSERT_EQ("VS sstatus write lands on vsstatus",
                   readback & (1UL << 18), write_val & (1UL << 18));

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-03: vsstatus.FS=0 causes FP illegal-instruction
 *
 * V=1, vsstatus.FS=Off, mstatus.FS=Initial (HS-level non-zero).
 * FP instruction in VS-mode triggers illegal-instruction (cause=2).
 * =================================================================== */
TEST_REGISTER(vsst_03_fs_off_fp_illegal);
bool vsst_03_fs_off_fp_illegal(void)
{
    TEST_BEGIN("VSST-03: vsstatus.FS=0 FP triggers illegal-inst");

    if (!platform_has_f_ext())
        TEST_SKIP("F extension not available");

    uintptr_t saved_mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

    /* Set mstatus.FS = Initial (HS-level allows FP). */
    uintptr_t mstatus = (saved_mstatus & ~SSTATUS_FS_MASK) | SSTATUS_FS_INITIAL;
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus));

    /* Set vsstatus.FS = Off (VS-level disables FP). */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus = (vsstatus & ~SSTATUS_FS_MASK) | SSTATUS_FS_OFF;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /* VS-mode FP should trigger illegal-instruction (cause=2). */
    EXPECT_ILLEGAL_INST(run_in_vs_mode(vs_exec_fp_inst, 0));

    asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-04: sstatus.FS=0 causes FP illegal-instruction
 *
 * V=1, mstatus.FS=Off (HS-level disables FP), vsstatus.FS=Initial.
 * FP instruction in VS-mode triggers illegal-instruction (cause=2).
 * =================================================================== */
TEST_REGISTER(vsst_04_sstatus_fs_off_fp_illegal);
bool vsst_04_sstatus_fs_off_fp_illegal(void)
{
    TEST_BEGIN("VSST-04: sstatus.FS=0 FP triggers illegal-inst");

    if (!platform_has_f_ext())
        TEST_SKIP("F extension not available");

    uintptr_t saved_mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

    /* Set mstatus.FS = Off (HS-level disables FP). */
    uintptr_t mstatus = (saved_mstatus & ~SSTATUS_FS_MASK) | SSTATUS_FS_OFF;
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus));

    /* Set vsstatus.FS = Initial (VS-level allows FP). */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus = (vsstatus & ~SSTATUS_FS_MASK) | SSTATUS_FS_INITIAL;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /* VS-mode FP should trigger illegal-instruction (cause=2). */
    EXPECT_ILLEGAL_INST(run_in_vs_mode(vs_exec_fp_inst, 0));

    asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-05: both FS non-zero allows FP execution
 *
 * V=1, mstatus.FS=Initial, vsstatus.FS=Initial.
 * FP instruction in VS-mode completes without trap.
 * =================================================================== */
TEST_REGISTER(vsst_05_both_fs_nonzero_fp_ok);
bool vsst_05_both_fs_nonzero_fp_ok(void)
{
    TEST_BEGIN("VSST-05: both FS non-zero, FP executes normally");

    if (!platform_has_f_ext())
        TEST_SKIP("F extension not available");

    uintptr_t saved_mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

    /* Set mstatus.FS = Initial. */
    uintptr_t mstatus = (saved_mstatus & ~SSTATUS_FS_MASK) | SSTATUS_FS_INITIAL;
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus));

    /* Set vsstatus.FS = Initial. */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus = (vsstatus & ~SSTATUS_FS_MASK) | SSTATUS_FS_INITIAL;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /* VS-mode FP should complete without any trap. */
    VS_EXPECT_NO_TRAP(run_in_vs_mode(vs_exec_fp_inst, 0));

    asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-06: FP write makes both FS fields Dirty
 *
 * V=1, both mstatus.FS and vsstatus.FS set to Initial.
 * After VS-mode FP write, both must transition to Dirty (3).
 * =================================================================== */
TEST_REGISTER(vsst_06_fp_makes_both_fs_dirty);
bool vsst_06_fp_makes_both_fs_dirty(void)
{
    TEST_BEGIN("VSST-06: FP write makes both FS Dirty");

    if (!platform_has_f_ext())
        TEST_SKIP("F extension not available");

    uintptr_t saved_mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

    /* Set mstatus.FS = Initial. */
    uintptr_t mstatus = (saved_mstatus & ~SSTATUS_FS_MASK) | SSTATUS_FS_INITIAL;
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus));

    /* Set vsstatus.FS = Initial. */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus = (vsstatus & ~SSTATUS_FS_MASK) | SSTATUS_FS_INITIAL;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /* Execute FP instruction in VS-mode. */
    run_in_vs_mode(vs_exec_fp_inst, 0);

    /* Check mstatus.FS == Dirty (3). */
    uintptr_t mstatus_after;
    asm volatile ("csrr %0, mstatus" : "=r"(mstatus_after));
    TEST_ASSERT_EQ("mstatus.FS == Dirty",
                   mstatus_after & SSTATUS_FS_MASK, SSTATUS_FS_DIRTY);

    /* Check vsstatus.FS == Dirty (3). */
    uintptr_t vsstatus_after;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus_after));
    TEST_ASSERT_EQ("vsstatus.FS == Dirty",
                   vsstatus_after & SSTATUS_FS_MASK, SSTATUS_FS_DIRTY);

    asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-07: vsstatus.VS=0 causes Vector illegal-instruction
 *
 * V=1, vsstatus.VS=Off, mstatus.VS=Initial (HS-level non-zero).
 * Vector instruction in VS-mode triggers illegal-instruction (cause=2).
 * =================================================================== */
TEST_REGISTER(vsst_07_vs_off_vector_illegal);
bool vsst_07_vs_off_vector_illegal(void)
{
    TEST_BEGIN("VSST-07: vsstatus.VS=0 Vector triggers illegal-inst");

    if (!platform_has_v_ext())
        TEST_SKIP("V extension not available");

    uintptr_t saved_mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

    /* Set mstatus.VS = Initial (HS-level allows Vector). */
    uintptr_t mstatus = (saved_mstatus & ~SSTATUS_VS_MASK) | SSTATUS_VS_INITIAL;
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus));

    /* Set vsstatus.VS = Off (VS-level disables Vector). */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus = (vsstatus & ~SSTATUS_VS_MASK) | SSTATUS_VS_OFF;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /* VS-mode Vector should trigger illegal-instruction (cause=2). */
    EXPECT_ILLEGAL_INST(run_in_vs_mode(vs_exec_vector_inst, 0));

    asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-08: sstatus.VS=0 causes Vector illegal-instruction
 *
 * V=1, mstatus.VS=Off (HS-level disables Vector), vsstatus.VS=Initial.
 * Vector instruction in VS-mode triggers illegal-instruction (cause=2).
 * =================================================================== */
TEST_REGISTER(vsst_08_sstatus_vs_off_vector_illegal);
bool vsst_08_sstatus_vs_off_vector_illegal(void)
{
    TEST_BEGIN("VSST-08: sstatus.VS=0 Vector triggers illegal-inst");

    if (!platform_has_v_ext())
        TEST_SKIP("V extension not available");

    uintptr_t saved_mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

    /* Set mstatus.VS = Off (HS-level disables Vector). */
    uintptr_t mstatus = (saved_mstatus & ~SSTATUS_VS_MASK) | SSTATUS_VS_OFF;
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus));

    /* Set vsstatus.VS = Initial (VS-level allows Vector). */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus = (vsstatus & ~SSTATUS_VS_MASK) | SSTATUS_VS_INITIAL;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /* VS-mode Vector should trigger illegal-instruction (cause=2). */
    EXPECT_ILLEGAL_INST(run_in_vs_mode(vs_exec_vector_inst, 0));

    asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-09: Vector write makes both VS fields Dirty
 *
 * V=1, both mstatus.VS and vsstatus.VS set to Initial.
 * After VS-mode Vector instruction, both must transition to Dirty (3).
 * =================================================================== */
TEST_REGISTER(vsst_09_vector_makes_both_vs_dirty);
bool vsst_09_vector_makes_both_vs_dirty(void)
{
    TEST_BEGIN("VSST-09: Vector write makes both VS Dirty");

    if (!platform_has_v_ext())
        TEST_SKIP("V extension not available");

    uintptr_t saved_mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

    /* Set mstatus.VS = Initial. */
    uintptr_t mstatus = (saved_mstatus & ~SSTATUS_VS_MASK) | SSTATUS_VS_INITIAL;
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus));

    /* Set vsstatus.VS = Initial. */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus = (vsstatus & ~SSTATUS_VS_MASK) | SSTATUS_VS_INITIAL;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /* Execute Vector instruction in VS-mode. */
    run_in_vs_mode(vs_exec_vector_inst, 0);

    /* Check mstatus.VS == Dirty (3). */
    uintptr_t mstatus_after;
    asm volatile ("csrr %0, mstatus" : "=r"(mstatus_after));
    TEST_ASSERT_EQ("mstatus.VS == Dirty",
                   mstatus_after & SSTATUS_VS_MASK, SSTATUS_VS_DIRTY);

    /* Check vsstatus.VS == Dirty (3). */
    uintptr_t vsstatus_after;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus_after));
    TEST_ASSERT_EQ("vsstatus.VS == Dirty",
                   vsstatus_after & SSTATUS_VS_MASK, SSTATUS_VS_DIRTY);

    asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-10: vsstatus.SD reflects VS-mode view only
 *
 * SD is computed from FS/VS/XS in vsstatus only (not sstatus).
 * When vsstatus.FS=Clean and vsstatus.VS=Clean, SD should be 0.
 * When vsstatus.FS=Dirty, SD should be 1.
 *
 * Cross-validation (norm:vsstatus_sd_xs_op):
 * HS-level sstatus.FS must NOT affect vsstatus.SD. Setting
 * mstatus.FS=Dirty while vsstatus.FS=Off must leave SD=0.
 * =================================================================== */
TEST_REGISTER(vsst_10_sd_reflects_vs_view);
bool vsst_10_sd_reflects_vs_view(void)
{
    TEST_BEGIN("VSST-10: vsstatus.SD reflects VS-mode view");

    /* Set vsstatus.FS = Clean, vsstatus.VS = Off (no dirty state). */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus = (vsstatus & ~(SSTATUS_FS_MASK | SSTATUS_VS_MASK))
               | SSTATUS_FS_CLEAN | SSTATUS_VS_OFF;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /* Read back — SD should be 0 (nothing dirty). */
    uintptr_t readback;
    asm volatile ("csrr %0, 0x200" : "=r"(readback));
    TEST_ASSERT_EQ("SD=0 when FS=Clean, VS=Off",
                   readback & SSTATUS_SD_BIT, 0UL);

    /* Set vsstatus.FS = Dirty → SD should become 1. */
    vsstatus = (readback & ~SSTATUS_FS_MASK) | SSTATUS_FS_DIRTY;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    asm volatile ("csrr %0, 0x200" : "=r"(readback));
    TEST_ASSERT_EQ("SD=1 when FS=Dirty",
                   readback & SSTATUS_SD_BIT, SSTATUS_SD_BIT);

    /* Set vsstatus.FS = Off, vsstatus.VS = Dirty → SD should be 1. */
    if (platform_has_v_ext()) {
        vsstatus = (readback & ~(SSTATUS_FS_MASK | SSTATUS_VS_MASK))
                   | SSTATUS_FS_OFF | SSTATUS_VS_DIRTY;
        asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

        asm volatile ("csrr %0, 0x200" : "=r"(readback));
        TEST_ASSERT_EQ("SD=1 when VS=Dirty",
                       readback & SSTATUS_SD_BIT, SSTATUS_SD_BIT);
    }

    /* Cross-validation: mstatus.FS=Dirty + vsstatus.FS=Off → SD=0.
     * HS-level sstatus.FS must NOT influence vsstatus.SD. */
    {
        uintptr_t saved_mstatus;
        asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

        /* Set mstatus.FS = Dirty at HS level. */
        uintptr_t mstatus = (saved_mstatus & ~SSTATUS_FS_MASK)
                            | SSTATUS_FS_DIRTY;
        asm volatile ("csrw mstatus, %0" :: "r"(mstatus));

        /* Set vsstatus.FS = Off, vsstatus.VS = Off. */
        vsstatus = (vsstatus & ~(SSTATUS_FS_MASK | SSTATUS_VS_MASK))
                   | SSTATUS_FS_OFF | SSTATUS_VS_OFF;
        asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

        asm volatile ("csrr %0, 0x200" : "=r"(readback));
        TEST_ASSERT_EQ("SD=0 despite mstatus.FS=Dirty (HS independent)",
                       readback & SSTATUS_SD_BIT, 0UL);

        /* Also verify HS sstatus.SD IS set (mstatus.FS=Dirty). */
        uintptr_t hs_sstatus;
        asm volatile ("csrr %0, sstatus" : "=r"(hs_sstatus));
        TEST_ASSERT_EQ("HS sstatus.SD=1 when mstatus.FS=Dirty",
                       hs_sstatus & SSTATUS_SD_BIT, SSTATUS_SD_BIT);

        asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));
    }

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-11: V=0 vsstatus does not affect HS-mode behavior
 *
 * Setting vsstatus.SIE=0 in V=0 context should NOT disable
 * HS-mode supervisor interrupts (sstatus.SIE is independent).
 * =================================================================== */
TEST_REGISTER(vsst_11_v0_no_effect);
bool vsst_11_v0_no_effect(void)
{
    TEST_BEGIN("VSST-11: V=0 vsstatus does not affect HS behavior");

    /* Save and set sstatus.SIE = 1 (HS-mode interrupt enable). */
    uintptr_t saved_sstatus;
    asm volatile ("csrr %0, sstatus" : "=r"(saved_sstatus));
    asm volatile ("csrs sstatus, %0" :: "r"(SSTATUS_SIE_BIT));

    /* Clear vsstatus.SIE = 0 — must NOT affect HS-mode. */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus &= ~SSTATUS_SIE_BIT;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /* Verify HS-mode sstatus.SIE is still 1. */
    uintptr_t sstatus;
    asm volatile ("csrr %0, sstatus" : "=r"(sstatus));
    TEST_ASSERT_EQ("HS sstatus.SIE unaffected by vsstatus.SIE=0",
                   sstatus & SSTATUS_SIE_BIT, SSTATUS_SIE_BIT);

    /* Restore sstatus. */
    asm volatile ("csrw sstatus, %0" :: "r"(saved_sstatus));

    HYP_TEST_END();
}

/* ===================================================================
 * VSST-12: UXL field read/write (WARL)
 *
 * vsstatus.UXL controls VU-mode XLEN. On RV64, valid values
 * are typically 2 (XLEN=64). The field is WARL; write and read back
 * to verify the implementation accepts or silently constrains the value.
 * =================================================================== */
TEST_REGISTER(vsst_12_uxl_rw);
bool vsst_12_uxl_rw(void)
{
    TEST_BEGIN("VSST-12: UXL field read/write WARL");

    /* Read current vsstatus.UXL. */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    uintptr_t original_uxl = (vsstatus >> SSTATUS_UXL_SHIFT) & 3UL;

    /* UXL should have a valid value (1=32, 2=64, 3=128). */
    TEST_ASSERT("UXL has valid initial value",
                original_uxl >= 1 && original_uxl <= 3);

    /* Try writing UXL = 2 (XLEN=64, the common RV64 value). */
    uintptr_t new_vsstatus = (vsstatus & ~SSTATUS_UXL_MASK)
                             | (2UL << SSTATUS_UXL_SHIFT);
    asm volatile ("csrw 0x200, %0" :: "r"(new_vsstatus));

    uintptr_t readback;
    asm volatile ("csrr %0, 0x200" : "=r"(readback));
    uintptr_t readback_uxl = (readback >> SSTATUS_UXL_SHIFT) & 3UL;

    /* WARL: implementation may keep original value or accept new one. */
    TEST_ASSERT("UXL WARL: value is valid after write",
                readback_uxl >= 1 && readback_uxl <= 3);

    /* Try writing an unsupported value (e.g., UXL=3 for 128-bit).
     * The field is WARL, so the implementation should either accept
     * it or silently constrain it to a legal value. */
    new_vsstatus = (vsstatus & ~SSTATUS_UXL_MASK)
                   | (3UL << SSTATUS_UXL_SHIFT);
    asm volatile ("csrw 0x200, %0" :: "r"(new_vsstatus));

    asm volatile ("csrr %0, 0x200" : "=r"(readback));
    readback_uxl = (readback >> SSTATUS_UXL_SHIFT) & 3UL;

    TEST_ASSERT("UXL WARL: legal value after writing 3",
                readback_uxl >= 1 && readback_uxl <= 3);

    /* Restore original UXL. */
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    HYP_TEST_END();
}
