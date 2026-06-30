/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * test_virtual_inst.c - VINST: virtual-instruction exception tests
 *
 * Group 12 of Hypervisor extension test plan.
 * =================================================================== */

#include "test_helpers.h"

/* ===================================================================
 * VINST-01: VS executes HLV
 *
 * Verify that HLV executed from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_01_vs_hlv);
bool vinst_01_vs_hlv(void)
{
    TEST_BEGIN("VINST-01: VS executes HLV");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hlv_w, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-02: VS executes HSV
 *
 * Verify that HSV executed from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_02_vs_hsv);
bool vinst_02_vs_hsv(void)
{
    TEST_BEGIN("VINST-02: VS executes HSV");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hsv_w, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-03: VS executes HLVX
 *
 * Verify that HLVX executed from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_03_vs_hlvx);
bool vinst_03_vs_hlvx(void)
{
    TEST_BEGIN("VINST-03: VS executes HLVX");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hlvx_wu, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-04: VS executes HFENCE.VVMA
 *
 * Verify that HFENCE.VVMA executed from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_04_vs_hfence_vvma);
bool vinst_04_vs_hfence_vvma(void)
{
    TEST_BEGIN("VINST-04: VS executes HFENCE.VVMA");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hfence_vvma, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-05: VS executes HFENCE.GVMA
 *
 * Verify that HFENCE.GVMA executed from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_05_vs_hfence_gvma);
bool vinst_05_vs_hfence_gvma(void)
{
    TEST_BEGIN("VINST-05: VS executes HFENCE.GVMA");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hfence_gvma, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-06: VU executes HLV
 *
 * Verify that HLV executed from VU-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_06_vu_hlv);
bool vinst_06_vu_hlv(void)
{
    TEST_BEGIN("VINST-06: VU executes HLV");

    EXPECT_VIRTUAL_INST(run_in_vu_mode(vs_exec_hlv_w, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-07: VS accesses hstatus
 *
 * Verify that VS-mode access to hstatus triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_07_vs_hstatus);
bool vinst_07_vs_hstatus(void)
{
    TEST_BEGIN("VINST-07: VS accesses hstatus");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_hstatus, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-08: VS accesses hedeleg
 *
 * Verify that VS-mode access to hedeleg triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_08_vs_hedeleg);
bool vinst_08_vs_hedeleg(void)
{
    TEST_BEGIN("VINST-08: VS accesses hedeleg");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_hedeleg, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-09: VS accesses hgatp
 *
 * Verify that VS-mode access to hgatp triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_09_vs_hgatp);
bool vinst_09_vs_hgatp(void)
{
    TEST_BEGIN("VINST-09: VS accesses hgatp");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_hgatp, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-10: VS accesses vsstatus directly
 *
 * Verify that VS-mode direct access to vsstatus (CSR 0x200) triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_10_vs_vsstatus_direct);
bool vinst_10_vs_vsstatus_direct(void)
{
    TEST_BEGIN("VINST-10: VS accesses vsstatus directly");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_vsstatus_direct, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-11: VU executes WFI with TW=0
 *
 * Verify that WFI executed from VU-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_11_vu_wfi);
bool vinst_11_vu_wfi(void)
{
    TEST_BEGIN("VINST-11: VU executes WFI with TW=0");

    EXPECT_VIRTUAL_INST(run_in_vu_mode(vs_exec_wfi, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-12: VU executes SRET
 *
 * Verify that SRET executed from VU-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_12_vu_sret);
bool vinst_12_vu_sret(void)
{
    TEST_BEGIN("VINST-12: VU executes SRET");

    EXPECT_VIRTUAL_INST(run_in_vu_mode(vu_exec_sret, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-13: VU executes SFENCE.VMA
 *
 * Verify that SFENCE.VMA executed from VU-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_13_vu_sfence_vma);
bool vinst_13_vu_sfence_vma(void)
{
    TEST_BEGIN("VINST-13: VU executes SFENCE.VMA");

    EXPECT_VIRTUAL_INST(run_in_vu_mode(vs_exec_sfence_vma, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-14: VU accesses sstatus
 *
 * Verify that VU-mode access to sstatus triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_14_vu_sstatus);
bool vinst_14_vu_sstatus(void)
{
    TEST_BEGIN("VINST-14: VU accesses sstatus");

    EXPECT_VIRTUAL_INST(run_in_vu_mode(vs_read_sstatus, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-15: VU accesses scause
 *
 * Verify that VU-mode access to scause triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_15_vu_scause);
bool vinst_15_vu_scause(void)
{
    TEST_BEGIN("VINST-15: VU accesses scause");

    EXPECT_VIRTUAL_INST(run_in_vu_mode(vs_read_scause, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-16: VS WFI with VTW=1 and TW=0
 *
 * Verify that WFI from VS-mode with VTW=1 triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_16_vs_wfi_vtw);
bool vinst_16_vs_wfi_vtw(void)
{
    TEST_BEGIN("VINST-16: VS WFI with VTW=1 and TW=0");

    hstatus_set_vtw(true);
    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_wfi, 0));
    hstatus_set_vtw(false);

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-17: VS SRET with VTSR=1
 *
 * Verify that SRET from VS-mode with VTSR=1 triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_17_vs_sret_vtsr);
bool vinst_17_vs_sret_vtsr(void)
{
    TEST_BEGIN("VINST-17: VS SRET with VTSR=1");

    hstatus_set_vtsr(true);
    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_sret, 0));
    hstatus_set_vtsr(false);

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-18: VS SFENCE with VTVM=1
 *
 * Verify that SFENCE.VMA from VS-mode with VTVM=1 triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_18_vs_sfence_vtvm);
bool vinst_18_vs_sfence_vtvm(void)
{
    TEST_BEGIN("VINST-18: VS SFENCE with VTVM=1");

    hstatus_set_vtvm(true);
    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_sfence_vma, 0));
    hstatus_set_vtvm(false);

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-19: VS read satp with VTVM=1
 *
 * Verify that satp read from VS-mode with VTVM=1 triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_19_vs_satp_vtvm);
bool vinst_19_vs_satp_vtvm(void)
{
    TEST_BEGIN("VINST-19: VS read satp with VTVM=1");

    hstatus_set_vtvm(true);
    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_satp, 0));
    hstatus_set_vtvm(false);

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-20: VS SINVAL with VTVM=1
 *
 * Verify that SINVAL.VMA from VS-mode with VTVM=1 triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_20_vs_sinval_vtvm);
bool vinst_20_vs_sinval_vtvm(void)
{
    TEST_BEGIN("VINST-20: VS SINVAL with VTVM=1");

    hstatus_set_vtvm(true);
    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_sinval_vma, 0));
    hstatus_set_vtvm(false);

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-21: FS=0 illegal not virtual
 *
 * Verify that floating-point instructions with FS=0 cause illegal instruction,
 * not virtual instruction (requires FP support detection).
 * =================================================================== */
TEST_REGISTER(vinst_21_fs0_illegal);
bool vinst_21_fs0_illegal(void)
{
    TEST_BEGIN("VINST-21: FS=0 illegal not virtual");

    /* Skip test if FP support detection is not available */
    TEST_SKIP("requires FP support detection");

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-22: VS=0 illegal not virtual
 *
 * Verify that vector instructions with VS=0 cause illegal instruction,
 * not virtual instruction (requires Vector support detection).
 * =================================================================== */
TEST_REGISTER(vinst_22_vs0_illegal);
bool vinst_22_vs0_illegal(void)
{
    TEST_BEGIN("VINST-22: VS=0 illegal not virtual");

    /* Skip test if Vector support detection is not available */
    TEST_SKIP("requires Vector support detection");

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-23: virtual-instruction trap stval correct
 *
 * Verify that when a virtual-instruction exception occurs, stval is set correctly.
 * =================================================================== */
TEST_REGISTER(vinst_23_virtual_inst_stval);
bool vinst_23_virtual_inst_stval(void)
{
    TEST_BEGIN("VINST-23: virtual-instruction trap stval correct");

    /* Trigger virtual-instruction exception */
    trap_expect_begin();
    run_in_vs_mode(vs_read_hstatus, 0);
    TEST_ASSERT("virtual-inst trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
        /* Check stval/tval */
        uintptr_t tval = trap_get_tval();
        /* stval for virtual-instruction should contain faulting instruction bits */
        TEST_ASSERT("stval should be set", tval != 0);
    }
    trap_expect_end();

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-24: mstatus.TW=1 overrides VTW
 *
 * Verify that mstatus.TW=1 causes illegal instruction, overriding VTW.
 * =================================================================== */
TEST_REGISTER(vinst_24_tw_overrides_vtw);
bool vinst_24_tw_overrides_vtw(void)
{
    TEST_BEGIN("VINST-24: mstatus.TW=1 overrides VTW");

    /* Set mstatus.TW=1 (bit 21) */
    uintptr_t ms;
    asm volatile ("csrr %0, mstatus" : "=r"(ms));
    ms |= (1UL << 21);
    asm volatile ("csrw mstatus, %0" :: "r"(ms));

    /* Execute WFI from VS-mode - should get illegal instruction (cause=2) */
    EXPECT_ILLEGAL_INST(run_in_vs_mode(vs_exec_wfi, 0));

    /* Clear mstatus.TW */
    asm volatile ("csrr %0, mstatus" : "=r"(ms));
    ms &= ~(1UL << 21);
    asm volatile ("csrw mstatus, %0" :: "r"(ms));

    HYP_TEST_END();
}
