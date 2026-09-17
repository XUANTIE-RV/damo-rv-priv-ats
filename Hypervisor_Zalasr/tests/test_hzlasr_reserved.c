/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 5.7 - reserved encodings (load without aq / store without rl)
 *             (HZLASR-34)
 *
 * Spec basis:
 *   norm:ldaq_no_aq_reserved / norm:sdrl_no_rl_reserved - a load-acquire
 *     without aq (including load-release) and a store-release without rl
 *     (including store-acquire) are RESERVED encodings.
 *   norm:H_cause_virtual_instruction - virtual-instruction (cause=22)
 *     replaces illegal-instruction ONLY for HS-qualified instructions that
 *     are blocked when V=1. Reserved encodings are illegal in ALL modes
 *     (not HS-qualified), so V=1 execution raises illegal-instruction
 *     (cause=2), NOT virtual-instruction (cause=22). Mandatory negative
 *     assertion: reporting cause=22 violates the SPEC (analogue of
 *     HZABHA-37).
 *
 * Gated on ZALASR_AVAILABLE (config-declared), so a platform without
 * Zalasr skips rather than passing a reserved-encoding check "for the
 * wrong reason". The reserved encoding raises illegal-instruction BEFORE
 * any memory access, so no two-stage translation is needed; run_in_vs/vu
 * with vsatp/hgatp left Bare is used.
 * =================================================================== */

static void hzlasr_reserved_case(uintptr_t (*probe)(uintptr_t), int vu,
                                 const char *tag)
{
    uintptr_t va = (uintptr_t)test_data_area;
    hzlasr_store_le64(va, 0x0011223344556677ULL);
    hz_st_val = 0x00009abcu;

    trap_expect_begin();
    if (vu)
        (void)run_in_vu_mode(probe, va);
    else
        (void)run_in_vs_mode(probe, va);
    bool fired = trap_was_triggered();
    uintptr_t cause = fired ? trap_get_cause() : 0;
    trap_expect_end();

    printf("  [INFO] %s (%s): fired=%d cause=%lu\n",
           tag, vu ? "VU" : "VS", (int)fired, (unsigned long)cause);
    TEST_ASSERT("reserved encoding trapped", fired);
    TEST_ASSERT_EQ("reserved Zalasr encoding -> illegal-instruction (cause=2)",
                   cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    TEST_ASSERT_NEQ("reserved encoding NOT virtual-instruction (cause=22)",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
}

TEST_REGISTER(test_hzlasr_34_reserved_cause2);
bool test_hzlasr_34_reserved_cause2(void)
{
    TEST_BEGIN("HZLASR-34: reserved load-acquire/store-release -> cause=2, not 22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZALASR_AVAILABLE) TEST_SKIP("Zalasr not implemented");

    /* VS-mode: all four reserved encodings. */
    hzlasr_reserved_case(hz_vs_ld_noaq_rsv, 0, "load w/o aq (funct7 0x18)");
    hzlasr_reserved_case(hz_vs_ld_rel_rsv,  0, "load-release (funct7 0x19)");
    hzlasr_reserved_case(hz_vs_st_norl_rsv, 0, "store w/o rl (funct7 0x1c)");
    hzlasr_reserved_case(hz_vs_st_acq_rsv,  0, "store-acquire (funct7 0x1e)");

    /* VU-mode: one representative load + one store. */
    hzlasr_reserved_case(hz_vs_ld_noaq_rsv, 1, "load w/o aq (funct7 0x18)");
    hzlasr_reserved_case(hz_vs_st_norl_rsv, 1, "store w/o rl (funct7 0x1c)");

    HYP_TEST_END();
}
