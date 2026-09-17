/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 4.8 - reserved byte/half lr/sc encodings (HZABHA-37)
 *
 * Spec basis:
 *   zabha.adoc NOTE (zabha_no_byte_halfword_lrsc) - Zabha omits byte and
 *     halfword lr/sc, so funct3=000/001 with funct5=00010(lr)/00011(sc)
 *     are RESERVED encodings.
 *   norm:H_cause_virtual_instruction - virtual-instruction (cause=22)
 *     replaces illegal-instruction ONLY for HS-qualified instructions that
 *     are blocked when V=1. Reserved encodings are illegal in ALL modes
 *     (not HS-qualified), so V=1 execution raises illegal-instruction
 *     (cause=2), NOT virtual-instruction (cause=22). Mandatory negative
 *     assertion: reporting cause=22 violates the SPEC.
 * =================================================================== */

/* Run a reserved byte/half lr/sc probe in VS or VU mode and check the
 * trap is illegal-instruction (cause=2), never virtual-instruction.
 *
 * The reserved encoding raises illegal-instruction BEFORE any memory
 * access, so no two-stage translation is needed; run_in_vs/vu_mode with
 * vsatp/hgatp left Bare (physical addresses) is used, avoiding the
 * two-stage entry path entirely. */
static void hzabha_reserved_case(uintptr_t (*probe)(uintptr_t), int vu,
                                 const char *tag)
{
    uintptr_t va = (uintptr_t)test_data_area;
    *(volatile uint64_t *)va = 0x0011223344556677ULL;

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
    TEST_ASSERT_EQ("reserved byte/half lr/sc -> illegal-instruction (cause=2)",
                   cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    TEST_ASSERT_NEQ("reserved encoding NOT virtual-instruction (cause=22)",
                    cause, (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
}

TEST_REGISTER(test_hzabha_37_reserved_lrsc_cause2);
bool test_hzabha_37_reserved_lrsc_cause2(void)
{
    TEST_BEGIN("HZABHA-37: reserved byte/half lr/sc -> cause=2, not 22");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZABHA_AVAILABLE) TEST_SKIP("Zabha not implemented");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    /* VS-mode: all four reserved encodings. */
    hzabha_reserved_case(hz_vs_lr_b_reserved, 0, "lr.b (reserved)");
    hzabha_reserved_case(hz_vs_sc_b_reserved, 0, "sc.b (reserved)");
    hzabha_reserved_case(hz_vs_lr_h_reserved, 0, "lr.h (reserved)");
    hzabha_reserved_case(hz_vs_sc_h_reserved, 0, "sc.h (reserved)");

    /* VU-mode: one representative. */
    hzabha_reserved_case(hz_vs_lr_b_reserved, 1, "lr.b (reserved)");

    HYP_TEST_END();
}
