/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1.2 ~ 1.5 - htinst transformed of compressed load/store
 *   1.2 register-based  c.lw/c.sw/c.ld/c.sd      (HZCA-07 ~ HZCA-10)
 *   1.3 stack-pointer   c.lwsp/c.swsp/c.ldsp/c.sdsp (HZCA-11 ~ HZCA-14)
 *   1.4 load/store format distinction + fields   (HZCA-15 ~ HZCA-17)
 *   1.5 Addr. Offset + htinst may be zero        (HZCA-18 ~ HZCA-20)
 *
 * Spec basis:
 *   htinst_transformed_compressed - a compressed trapping instruction
 *     is transformed by expand -> transform -> clear bit 1, so a nonzero
 *     htinst has bits[1:0] = 01 and equals the golden value exactly.
 *   htinst_transformed_load  - load keeps funct3/rd/opcode, imm zeroed,
 *     bits19:15 <- Addr. Offset.
 *   htinst_transformed_store - store keeps rs2/funct3/opcode, both imm
 *     halves zeroed, bits19:15 <- Addr. Offset.
 *   htinst_addr_offset - Addr. Offset = faulting VA - original VA, which
 *     is 0 for an aligned access.
 *   norm:H_trap_xtinst - zero is always allowed for an explicit fault.
 *
 * Every case here routes the guest-page fault into HS-mode (medeleg) so
 * hardware writes htinst; the symmetric mtinst path is Group 1.6.
 * =================================================================== */

/* Shared body for a single-instruction compressed htinst case: fire the
 * fault routed to HS-mode, check the cause, confirm the trapping
 * instruction is the expected compressed encoding, then assert htinst is
 * 0 or exactly the compressed transformed golden value. */
static void hzca_htinst_case(const char *cause_msg,
                             uintptr_t (*probe)(uintptr_t),
                             uintptr_t g_flags, uintptr_t exp_cause,
                             uintptr_t enc_half, uintptr_t expanded)
{
    uintptr_t va = (uintptr_t)test_fault_page;
    hzca_trap_t t = hzca_fire_mem_fault(probe, va, g_flags, exp_cause, true);

    TEST_ASSERT("compressed guest fault fired", t.fired);
    TEST_ASSERT_EQ(cause_msg, t.cause, exp_cause);
    hzca_check_trap_inst_compressed(t.epc, enc_half);
    hzca_assert_xtinst_compressed("htinst == 0 or compressed transformed",
                                  t.xtinst, expanded, t.tval, va);
}

/* ===================================================================
 * Group 1.2 - register-based compressed load/store
 * =================================================================== */

TEST_REGISTER(test_hzca_07_c_lw_htinst);
bool test_hzca_07_c_lw_htinst(void)
{
    TEST_BEGIN("HZCA-07: c.lw load guest-page-fault htinst = 0/transformed");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzca_htinst_case("cause == load guest-page-fault (21)",
                     hz_vs_c_lw, HZ_G_INV, CAUSE_LOAD_GUEST_PAGE_FAULT,
                     HZCA_ENC_C_LW_A0, HZCA_EXP_LW_A0_A0);
    HYP_TEST_END();
}

TEST_REGISTER(test_hzca_08_c_sw_htinst);
bool test_hzca_08_c_sw_htinst(void)
{
    TEST_BEGIN("HZCA-08: c.sw store guest-page-fault htinst (store branch)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzca_htinst_case("cause == store/AMO guest-page-fault (23)",
                     hz_vs_c_sw, HZ_G_RU, CAUSE_STORE_GUEST_PAGE_FAULT,
                     HZCA_ENC_C_SW_A0, HZCA_EXP_SW_A0_A0);
    HYP_TEST_END();
}

TEST_REGISTER(test_hzca_09_c_ld_htinst);
bool test_hzca_09_c_ld_htinst(void)
{
    TEST_BEGIN("HZCA-09: c.ld load guest-page-fault htinst (RV64)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    if (__riscv_xlen != 64) TEST_SKIP("RV64-only compressed doubleword instruction");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzca_htinst_case("cause == load guest-page-fault (21)",
                     hz_vs_c_ld, HZ_G_INV, CAUSE_LOAD_GUEST_PAGE_FAULT,
                     HZCA_ENC_C_LD_A0, HZCA_EXP_LD_A0_A0);
    HYP_TEST_END();
}

TEST_REGISTER(test_hzca_10_c_sd_htinst);
bool test_hzca_10_c_sd_htinst(void)
{
    TEST_BEGIN("HZCA-10: c.sd store guest-page-fault htinst (RV64)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    if (__riscv_xlen != 64) TEST_SKIP("RV64-only compressed doubleword instruction");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzca_htinst_case("cause == store/AMO guest-page-fault (23)",
                     hz_vs_c_sd, HZ_G_RU, CAUSE_STORE_GUEST_PAGE_FAULT,
                     HZCA_ENC_C_SD_A0, HZCA_EXP_SD_A0_A0);
    HYP_TEST_END();
}

/* ===================================================================
 * Group 1.3 - stack-pointer-based compressed load/store
 *
 * The base register is fixed to x2, so the probe points sp at the
 * victim page (see HZCA_SP_PROBE safety note). The rs1 field (x2) of
 * the expansion is replaced by Addr. Offset (=0), so the golden value
 * matches the register-based form with the same rd/rs2.
 * =================================================================== */

TEST_REGISTER(test_hzca_11_c_lwsp_htinst);
bool test_hzca_11_c_lwsp_htinst(void)
{
    TEST_BEGIN("HZCA-11: c.lwsp load guest-page-fault htinst");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzca_htinst_case("cause == load guest-page-fault (21)",
                     hz_vs_c_lwsp, HZ_G_INV, CAUSE_LOAD_GUEST_PAGE_FAULT,
                     HZCA_ENC_C_LWSP_A0, HZCA_EXP_LW_A0_SP);
    HYP_TEST_END();
}

TEST_REGISTER(test_hzca_12_c_swsp_htinst);
bool test_hzca_12_c_swsp_htinst(void)
{
    TEST_BEGIN("HZCA-12: c.swsp store guest-page-fault htinst");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzca_htinst_case("cause == store/AMO guest-page-fault (23)",
                     hz_vs_c_swsp, HZ_G_RU, CAUSE_STORE_GUEST_PAGE_FAULT,
                     HZCA_ENC_C_SWSP_A0, HZCA_EXP_SW_A0_SP);
    HYP_TEST_END();
}

TEST_REGISTER(test_hzca_13_c_ldsp_htinst);
bool test_hzca_13_c_ldsp_htinst(void)
{
    TEST_BEGIN("HZCA-13: c.ldsp load guest-page-fault htinst (RV64)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    if (__riscv_xlen != 64) TEST_SKIP("RV64-only compressed doubleword instruction");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzca_htinst_case("cause == load guest-page-fault (21)",
                     hz_vs_c_ldsp, HZ_G_INV, CAUSE_LOAD_GUEST_PAGE_FAULT,
                     HZCA_ENC_C_LDSP_A0, HZCA_EXP_LD_A0_SP);
    HYP_TEST_END();
}

TEST_REGISTER(test_hzca_14_c_sdsp_htinst);
bool test_hzca_14_c_sdsp_htinst(void)
{
    TEST_BEGIN("HZCA-14: c.sdsp store guest-page-fault htinst (RV64)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    if (__riscv_xlen != 64) TEST_SKIP("RV64-only compressed doubleword instruction");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);
    hzca_htinst_case("cause == store/AMO guest-page-fault (23)",
                     hz_vs_c_sdsp, HZ_G_RU, CAUSE_STORE_GUEST_PAGE_FAULT,
                     HZCA_ENC_C_SDSP_A0, HZCA_EXP_SD_A0_SP);
    HYP_TEST_END();
}

/* ===================================================================
 * Group 1.4 - transformed load/store format distinction and fields
 * =================================================================== */

/* ------------------------------------------------------------------
 * HZCA-15: load vs store transformed format distinction, field by field.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_15_load_store_format);
bool test_hzca_15_load_store_format(void)
{
    TEST_BEGIN("HZCA-15: compressed load vs store transformed format");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    hzca_trap_t tl = hzca_fire_mem_fault(hz_vs_c_lw, va, HZ_G_INV,
                                         CAUSE_LOAD_GUEST_PAGE_FAULT, true);
    hzca_trap_t ts = hzca_fire_mem_fault(hz_vs_c_sw, va, HZ_G_RU,
                                         CAUSE_STORE_GUEST_PAGE_FAULT, true);

    TEST_ASSERT("c.lw fault fired", tl.fired);
    TEST_ASSERT("c.sw fault fired", ts.fired);
    TEST_ASSERT_EQ("load cause == 21", tl.cause,
                   (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);
    TEST_ASSERT_EQ("store cause == 23", ts.cause,
                   (uintptr_t)CAUSE_STORE_GUEST_PAGE_FAULT);

    if (tl.xtinst != 0 && ts.xtinst != 0) {
        /* Restore bit 1 to recover the 32-bit opcode field: a compressed
         * transformed value carries bits[1:0]=01, so the raw opcode reads
         * 0x01/0x21 until bit 1 is set back. */
        uintptr_t load_r  = tl.xtinst | 2UL;
        uintptr_t store_r = ts.xtinst | 2UL;
        TEST_ASSERT_EQ("load htinst bits[1:0] == 01", tl.xtinst & 3UL, 1UL);
        TEST_ASSERT_EQ("store htinst bits[1:0] == 01", ts.xtinst & 3UL, 1UL);
        TEST_ASSERT_EQ("load opcode == LOAD (0x03)",
                       load_r & 0x7FUL, 0x03UL);
        TEST_ASSERT_EQ("store opcode == STORE (0x23)",
                       store_r & 0x7FUL, 0x23UL);
        TEST_ASSERT_EQ("load funct3 == 010 (lw)",
                       (tl.xtinst >> 12) & 7UL, 2UL);
        TEST_ASSERT_EQ("store funct3 == 010 (sw)",
                       (ts.xtinst >> 12) & 7UL, 2UL);
        /* Load keeps rd (bits11:7); store clears imm[11:7], keeps rs2. */
        TEST_ASSERT_EQ("load rd (bits11:7) == a0 (x10)",
                       (tl.xtinst >> 7) & 0x1FUL, 10UL);
        TEST_ASSERT_EQ("load imm[31:20] cleared",
                       tl.xtinst & 0xFFF00000UL, 0UL);
        TEST_ASSERT_EQ("store rs2 (bits24:20) == a0 (x10)",
                       (ts.xtinst >> 20) & 0x1FUL, 10UL);
        TEST_ASSERT_EQ("store imm[31:25] cleared",
                       ts.xtinst & 0xFE000000UL, 0UL);
        TEST_ASSERT_EQ("store imm[11:7] cleared",
                       ts.xtinst & 0x00000F80UL, 0UL);
    } else {
        printf("  [INFO] load htinst=0x%lx store htinst=0x%lx: zero is legal "
               "for an explicit fault; format contrast not observable\n",
               (unsigned long)tl.xtinst, (unsigned long)ts.xtinst);
    }

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-16: setting bit 1 of a nonzero htinst back to 1 restores the
 *          32-bit encoding; funct3/rd(rs2)/opcode match the expansion.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_16_fields_match_expanded);
bool test_hzca_16_fields_match_expanded(void)
{
    TEST_BEGIN("HZCA-16: transformed fields == expanded 32-bit instruction");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;

    hzca_trap_t t = hzca_fire_mem_fault(hz_vs_c_lw, va, HZ_G_INV,
                                        CAUSE_LOAD_GUEST_PAGE_FAULT, true);
    TEST_ASSERT("c.lw fault fired", t.fired);
    if (t.xtinst != 0) {
        uintptr_t restored = t.xtinst | 2UL;   /* bit1 -> 1: 32-bit form */
        TEST_ASSERT_EQ("restored bits[1:0] == 11 (valid standard encoding)",
                       restored & 3UL, 3UL);
        TEST_ASSERT_EQ("opcode == expanded lw opcode",
                       restored & 0x7FUL, HZCA_EXP_LW_A0_A0 & 0x7FUL);
        TEST_ASSERT_EQ("funct3 == expanded lw funct3",
                       (restored >> 12) & 7UL,
                       (HZCA_EXP_LW_A0_A0 >> 12) & 7UL);
        TEST_ASSERT_EQ("rd == expanded lw rd",
                       (restored >> 7) & 0x1FUL,
                       (HZCA_EXP_LW_A0_A0 >> 7) & 0x1FUL);
        TEST_ASSERT_EQ("imm[31:20] cleared by transform",
                       restored & 0xFFF00000UL, 0UL);
    } else {
        printf("  [INFO] load htinst=0 (legal); field match not observable\n");
    }

    hzca_trap_t s = hzca_fire_mem_fault(hz_vs_c_sw, va, HZ_G_RU,
                                        CAUSE_STORE_GUEST_PAGE_FAULT, true);
    TEST_ASSERT("c.sw fault fired", s.fired);
    if (s.xtinst != 0) {
        uintptr_t restored = s.xtinst | 2UL;
        TEST_ASSERT_EQ("store opcode == expanded sw opcode",
                       restored & 0x7FUL, HZCA_EXP_SW_A0_A0 & 0x7FUL);
        TEST_ASSERT_EQ("store funct3 == expanded sw funct3",
                       (restored >> 12) & 7UL,
                       (HZCA_EXP_SW_A0_A0 >> 12) & 7UL);
        TEST_ASSERT_EQ("store rs2 == expanded sw rs2",
                       (restored >> 20) & 0x1FUL,
                       (HZCA_EXP_SW_A0_A0 >> 20) & 0x1FUL);
    } else {
        printf("  [INFO] store htinst=0 (legal); field match not observable\n");
    }

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-17: compressed source bits[1:0]=01 vs non-compressed bits[1:0]=11
 *          for the same load semantics (c.lw vs lw).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_17_compressed_marker_contrast);
bool test_hzca_17_compressed_marker_contrast(void)
{
    TEST_BEGIN("HZCA-17: bits[1:0]=01 (compressed) vs 11 (non-compressed)");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    hzca_trap_t c = hzca_fire_mem_fault(hz_vs_c_lw, va, HZ_G_INV,
                                        CAUSE_LOAD_GUEST_PAGE_FAULT, true);
    hzca_trap_t n = hzca_fire_mem_fault(hz_vs_lw_norvc, va, HZ_G_INV,
                                        CAUSE_LOAD_GUEST_PAGE_FAULT, true);

    TEST_ASSERT("compressed c.lw fault fired", c.fired);
    TEST_ASSERT("non-compressed lw fault fired", n.fired);
    hzca_check_trap_inst_compressed(c.epc, HZCA_ENC_C_LW_A0);
    TEST_ASSERT_EQ("non-compressed trapping inst bits[1:0] == 11",
                   (uintptr_t)(*(volatile uint16_t *)n.epc & 3UL), 3UL);

    if (c.xtinst != 0)
        TEST_ASSERT_EQ("compressed htinst bits[1:0] == 01",
                       c.xtinst & 3UL, 1UL);
    else
        printf("  [INFO] compressed htinst=0 (legal); marker not observable\n");
    if (n.xtinst != 0)
        TEST_ASSERT_EQ("non-compressed htinst bits[1:0] == 11",
                       n.xtinst & 3UL, 3UL);
    else
        printf("  [INFO] non-compressed htinst=0 (legal); marker not observable\n");

    HYP_TEST_END();
}

/* ===================================================================
 * Group 1.5 - Addr. Offset and htinst may be zero
 * =================================================================== */

/* ------------------------------------------------------------------
 * HZCA-18: Addr. Offset (bits19:15) of an aligned compressed load/store
 *          is always 0.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_18_addr_offset_zero);
bool test_hzca_18_addr_offset_zero(void)
{
    TEST_BEGIN("HZCA-18: aligned compressed load/store Addr. Offset == 0");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    hzca_trap_t l = hzca_fire_mem_fault(hz_vs_c_lw, va, HZ_G_INV,
                                        CAUSE_LOAD_GUEST_PAGE_FAULT, true);
    hzca_trap_t s = hzca_fire_mem_fault(hz_vs_c_sw, va, HZ_G_RU,
                                        CAUSE_STORE_GUEST_PAGE_FAULT, true);
    TEST_ASSERT("c.lw fault fired", l.fired);
    TEST_ASSERT("c.sw fault fired", s.fired);

    if (l.xtinst != 0)
        TEST_ASSERT_EQ("load htinst Addr. Offset (bits19:15) == 0",
                       (l.xtinst >> 15) & 0x1FUL, 0UL);
    else
        printf("  [INFO] load htinst=0; Addr. Offset not observable\n");
    if (s.xtinst != 0)
        TEST_ASSERT_EQ("store htinst Addr. Offset (bits19:15) == 0",
                       (s.xtinst >> 15) & 0x1FUL, 0UL);
    else
        printf("  [INFO] store htinst=0; Addr. Offset not observable\n");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-19: htinst may be zero for an explicit access fault; a nonzero
 *          value must equal the golden exactly (not an arbitrary value).
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_19_htinst_may_be_zero);
bool test_hzca_19_htinst_may_be_zero(void)
{
    TEST_BEGIN("HZCA-19: htinst == 0 (legal) or exactly golden");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t va = (uintptr_t)test_fault_page;
    hzca_trap_t t = hzca_fire_mem_fault(hz_vs_c_lw, va, HZ_G_INV,
                                        CAUSE_LOAD_GUEST_PAGE_FAULT, true);
    TEST_ASSERT("c.lw guest fault fired", t.fired);
    TEST_ASSERT_EQ("cause == load guest-page-fault (21)", t.cause,
                   (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT);

    uintptr_t golden = hzca_golden_from(HZCA_EXP_LW_A0_A0, t.tval, va);
    TEST_ASSERT("golden computable", golden != 0);
    TEST_ASSERT("htinst == 0 (legal) or == golden exactly",
                t.xtinst == 0 || t.xtinst == golden);
    printf("  [INFO] htinst=0x%lx golden=0x%lx (%s)\n",
           (unsigned long)t.xtinst, (unsigned long)golden,
           t.xtinst == 0 ? "zero accepted" : "nonzero exact match");

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * HZCA-20: (recording type) Addr. Offset of a misaligned compressed
 *          memory access that straddles into the faulting page.
 *
 * Whether the platform splits the misaligned access (faulting VA !=
 * original VA -> a possibly nonzero Addr. Offset) or raises a misaligned
 * exception is platform/MAG/Zicclsm dependent and always legal; MAG
 * granularity verdicts belong to Zama16b_test_plan.md. This case only
 * records the observed htinst Addr. Offset semantics.
 * ------------------------------------------------------------------ */
TEST_REGISTER(test_hzca_20_misaligned_record);
bool test_hzca_20_misaligned_record(void)
{
    TEST_BEGIN("HZCA-20: (recording) misaligned c.lw Addr. Offset");
    if (!H_AVAILABLE) TEST_SKIP("H extension not available");
    if (!ZCA_AVAILABLE) TEST_SKIP("Zca not declared (ZCA_SUPPORTED)");
    REQUIRE_VSATP_MODE(HZ_VSMODE);
    REQUIRE_HGATP_MODE(HZ_GMODE);

    uintptr_t victim = (uintptr_t)test_fault_page;
    uintptr_t mis_va = victim - 2;   /* word straddles into the victim */

    two_stage_ctx_t ctx;
    ts2_setup_with_g_victim(&ctx, HZ_VSMODE, HZ_GMODE, victim, HZ_G_INV);
    hz_route_to_hs(1UL << CAUSE_LOAD_GUEST_PAGE_FAULT);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, hz_vs_c_lw, mis_va);
    bool fired = trap_was_triggered();
    uintptr_t cause  = fired ? trap_get_cause()  : 0;
    uintptr_t xtinst = fired ? trap_get_htinst() : 0;
    uintptr_t tval   = fired ? trap_get_tval()   : 0;
    trap_expect_end();
    hz_unroute_from_hs(1UL << CAUSE_LOAD_GUEST_PAGE_FAULT);
    ts2_finish(&ctx);

    printf("  [RECORD] misaligned c.lw: fired=%d cause=%lu htinst=0x%lx "
           "tval=0x%lx\n", (int)fired, (unsigned long)cause,
           (unsigned long)xtinst, (unsigned long)tval);
    if (fired && cause == (uintptr_t)CAUSE_LOAD_GUEST_PAGE_FAULT &&
        xtinst != 0) {
        uintptr_t off_field = (xtinst >> 15) & 0x1FUL;
        printf("  [RECORD] Addr. Offset field (bits19:15) = %lu "
               "(nonzero allowed for a split misaligned access)\n",
               (unsigned long)off_field);
        /* Even when misaligned, a nonzero compressed transformed value
         * still carries the bits[1:0]=01 marker. */
        TEST_ASSERT_EQ("compressed transformed marker bits[1:0] == 01",
                       xtinst & 3UL, 1UL);
    }

    HYP_TEST_END();
}
