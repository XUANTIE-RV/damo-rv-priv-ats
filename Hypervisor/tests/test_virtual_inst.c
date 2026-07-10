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
 * Local constants
 *
 * Note: SSTATUS_FS_*, SSTATUS_VS_*, MISA_F, MISA_V are already
 * defined in test_vsstatus.c (included earlier in test_register.c).
 * =================================================================== */

/* ===================================================================
 * Local platform capability detection
 * =================================================================== */

static bool vinst_has_f_ext(void)
{
    uintptr_t misa;
    asm volatile ("csrr %0, misa" : "=r"(misa));
    return (misa & MISA_F) != 0;
}

static bool vinst_has_v_ext(void)
{
    uintptr_t misa;
    asm volatile ("csrr %0, misa" : "=r"(misa));
    return (misa & MISA_V) != 0;
}

/* ===================================================================
 * Local FP/Vector instruction trampolines.
 *
 * Encoded as raw .4byte to avoid -march dependency.
 * =================================================================== */

/* fadd.s f0, f0, f0 */
static uintptr_t vinst_exec_fp_inst(uintptr_t arg)
{
    (void)arg;
    asm volatile (".4byte 0x00000053" ::: "memory");
    return 0;
}

/* vsetvli t0, zero, e8, m1, ta, ma */
static uintptr_t vinst_exec_vector_inst(uintptr_t arg)
{
    (void)arg;
    asm volatile (".4byte 0x0C0072D7" ::: "memory");
    return 0;
}

/* ===================================================================
 * Read a 32-bit instruction from a potentially misaligned address.
 *
 * RISC-V allows PCs to be 2-byte aligned (RVC compressed instructions),
 * so a direct uint32_t* dereference can cause a load-misaligned trap.
 * Use byte-wise loads to avoid this.
 * =================================================================== */
static uint32_t vinst_read_inst_at(uintptr_t addr)
{
    const volatile uint8_t *p = (const volatile uint8_t *)addr;
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

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

    if (!vinst_has_f_ext())
        TEST_SKIP("F extension not available");

    uintptr_t saved_mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

    /* Set mstatus.FS = Initial (HS-level allows FP). */
    uintptr_t ms = (saved_mstatus & ~SSTATUS_FS_MASK) | SSTATUS_FS_INITIAL;
    asm volatile ("csrw mstatus, %0" :: "r"(ms));

    /* Set vsstatus.FS = Off (VS-level disables FP). */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus = (vsstatus & ~SSTATUS_FS_MASK) | SSTATUS_FS_OFF;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /*
     * VS-mode FP with vsstatus.FS=0 must trigger illegal-instruction
     * (cause=2), NOT virtual-instruction (cause=22).
     * This validates norm:H_illegalinst_xstatus_fs_vs.
     */
    EXPECT_ILLEGAL_INST(run_in_vs_mode(vinst_exec_fp_inst, 0));

    /* Restore mstatus.FS. */
    asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));

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

    if (!vinst_has_v_ext())
        TEST_SKIP("V extension not available");

    uintptr_t saved_mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(saved_mstatus));

    /* Set mstatus.VS = Initial (HS-level allows Vector). */
    uintptr_t ms = (saved_mstatus & ~SSTATUS_VS_MASK) | SSTATUS_VS_INITIAL;
    asm volatile ("csrw mstatus, %0" :: "r"(ms));

    /* Set vsstatus.VS = Off (VS-level disables Vector). */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));
    vsstatus = (vsstatus & ~SSTATUS_VS_MASK) | SSTATUS_VS_OFF;
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));

    /*
     * VS-mode Vector with vsstatus.VS=0 must trigger illegal-instruction
     * (cause=2), NOT virtual-instruction (cause=22).
     * This validates norm:H_illegalinst_xstatus_fs_vs.
     */
    EXPECT_ILLEGAL_INST(run_in_vs_mode(vinst_exec_vector_inst, 0));

    /* Restore mstatus. */
    asm volatile ("csrw mstatus, %0" :: "r"(saved_mstatus));

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

    /* Trigger virtual-instruction exception via VS-mode hstatus read. */
    trap_expect_begin();
    run_in_vs_mode(vs_read_hstatus, 0);
    TEST_ASSERT("virtual-inst trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

        /*
         * Verify stval per norm:H_virtinst_xtval:
         * "On a virtual-instruction trap, mtval or stval is written the
         *  same as for an illegal-instruction trap."
         *
         * For illegal-instruction traps, stval contains either the
         * faulting instruction encoding or zero. Read the instruction
         * at epc to compare.
         */
        uintptr_t epc = trap_get_epc();
        uintptr_t tval = trap_get_tval();
        uint32_t inst_at_epc = vinst_read_inst_at(epc);

        if (tval == 0) {
            /* Implementation chose to write 0 (spec-compliant). */
            TEST_ASSERT("stval=0 (implementation choice)", 1);
        }
        else {
            TEST_ASSERT_EQ("stval matches instruction at epc",
                           tval, (uintptr_t)inst_at_epc);
        }
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

/* ===================================================================
 * VINST-25: VS accesses hideleg
 *
 * Verify that VS-mode access to hideleg triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_25_vs_hideleg);
bool vinst_25_vs_hideleg(void)
{
    TEST_BEGIN("VINST-25: VS accesses hideleg");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_hideleg, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-26: VS accesses hcounteren
 *
 * Verify that VS-mode access to hcounteren triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_26_vs_hcounteren);
bool vinst_26_vs_hcounteren(void)
{
    TEST_BEGIN("VINST-26: VS accesses hcounteren");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_hcounteren, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-27: VS accesses htimedelta
 *
 * Verify that VS-mode access to htimedelta triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_27_vs_htimedelta);
bool vinst_27_vs_htimedelta(void)
{
    TEST_BEGIN("VINST-27: VS accesses htimedelta");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_htimedelta, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-28: VS accesses hip
 *
 * Verify that VS-mode access to hip triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_28_vs_hip);
bool vinst_28_vs_hip(void)
{
    TEST_BEGIN("VINST-28: VS accesses hip");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_hip, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-29: VS accesses hie
 *
 * Verify that VS-mode access to hie triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_29_vs_hie);
bool vinst_29_vs_hie(void)
{
    TEST_BEGIN("VINST-29: VS accesses hie");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_hie, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-30: VS accesses hvip
 *
 * Verify that VS-mode access to hvip triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_30_vs_hvip);
bool vinst_30_vs_hvip(void)
{
    TEST_BEGIN("VINST-30: VS accesses hvip");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_hvip, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-31: VS accesses henvcfg
 *
 * Verify that VS-mode access to henvcfg triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_31_vs_henvcfg);
bool vinst_31_vs_henvcfg(void)
{
    TEST_BEGIN("VINST-31: VS accesses henvcfg");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_read_henvcfg, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-32: VS writes hstatus
 *
 * Verify that VS-mode write to hstatus triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_32_vs_write_hstatus);
bool vinst_32_vs_write_hstatus(void)
{
    TEST_BEGIN("VINST-32: VS writes hstatus");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_write_hstatus, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-33: VU accesses sie
 *
 * Verify that VU-mode access to sie triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_33_vu_sie);
bool vinst_33_vu_sie(void)
{
    TEST_BEGIN("VINST-33: VU accesses sie");

    EXPECT_VIRTUAL_INST(run_in_vu_mode(vs_read_sie, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-34: VU accesses sip
 *
 * Verify that VU-mode access to sip triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_34_vu_sip);
bool vinst_34_vu_sip(void)
{
    TEST_BEGIN("VINST-34: VU accesses sip");

    EXPECT_VIRTUAL_INST(run_in_vu_mode(vs_read_sip, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-35: VU accesses stvec
 *
 * Verify that VU-mode access to stvec triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_35_vu_stvec);
bool vinst_35_vu_stvec(void)
{
    TEST_BEGIN("VINST-35: VU accesses stvec");

    EXPECT_VIRTUAL_INST(run_in_vu_mode(vs_read_stvec, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-36: VU accesses sepc
 *
 * Verify that VU-mode access to sepc triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_36_vu_sepc);
bool vinst_36_vu_sepc(void)
{
    TEST_BEGIN("VINST-36: VU accesses sepc");

    EXPECT_VIRTUAL_INST(run_in_vu_mode(vs_read_sepc, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-37: VS write satp with VTVM=1
 *
 * Verify that satp write from VS-mode with VTVM=1 triggers virtual-instruction.
 * =================================================================== */
TEST_REGISTER(vinst_37_vs_write_satp_vtvm);
bool vinst_37_vs_write_satp_vtvm(void)
{
    TEST_BEGIN("VINST-37: VS write satp with VTVM=1");

    hstatus_set_vtvm(true);
    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_write_satp, 0));
    hstatus_set_vtvm(false);

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-38: VS executes HLV.B
 *
 * Verify that HLV.B from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_38_vs_hlv_b);
bool vinst_38_vs_hlv_b(void)
{
    TEST_BEGIN("VINST-38: VS executes HLV.B");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hlv_b, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-39: VS executes HLV.H
 *
 * Verify that HLV.H from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_39_vs_hlv_h);
bool vinst_39_vs_hlv_h(void)
{
    TEST_BEGIN("VINST-39: VS executes HLV.H");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hlv_h, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-40: VS executes HLV.D
 *
 * Verify that HLV.D from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_40_vs_hlv_d);
bool vinst_40_vs_hlv_d(void)
{
    TEST_BEGIN("VINST-40: VS executes HLV.D");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hlv_d, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-41: VS executes HSV.B
 *
 * Verify that HSV.B from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_41_vs_hsv_b);
bool vinst_41_vs_hsv_b(void)
{
    TEST_BEGIN("VINST-41: VS executes HSV.B");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hsv_b, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-42: VS executes HSV.H
 *
 * Verify that HSV.H from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_42_vs_hsv_h);
bool vinst_42_vs_hsv_h(void)
{
    TEST_BEGIN("VINST-42: VS executes HSV.H");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hsv_h, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-43: VS executes HSV.D
 *
 * Verify that HSV.D from VS-mode triggers virtual-instruction exception.
 * =================================================================== */
TEST_REGISTER(vinst_43_vs_hsv_d);
bool vinst_43_vs_hsv_d(void)
{
    TEST_BEGIN("VINST-43: VS executes HSV.D");

    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_hsv_d, 0));

    HYP_TEST_END();
}

/* ===================================================================
 * VINST-44: mstatus.TSR=1 does not affect VS-mode SRET
 *
 * Per norm:mstatus_modes:
 *   "The TSR and TVM fields of mstatus affect execution only in
 *    HS-mode, not in VS-mode."
 *
 * Verify that mstatus.TSR=1 has NO effect on SRET in VS-mode.
 * Only hstatus.VTSR=1 controls whether SRET traps in VS-mode.
 * =================================================================== */
TEST_REGISTER(vinst_44_tsr_no_effect_vs_mode);
bool vinst_44_tsr_no_effect_vs_mode(void)
{
    TEST_BEGIN("VINST-44: mstatus.TSR=1 does not affect VS-mode SRET");

    /* Set hstatus.VTSR=1 and mstatus.TSR=1 (bit 22). */
    hstatus_set_vtsr(true);

    uintptr_t ms;
    asm volatile ("csrr %0, mstatus" : "=r"(ms));
    ms |= (1UL << 22);  /* mstatus.TSR */
    asm volatile ("csrw mstatus, %0" :: "r"(ms));

    /*
     * mstatus.TSR affects only HS-mode, not VS-mode (norm:mstatus_modes).
     * In VS-mode, SRET is controlled solely by hstatus.VTSR.
     * VTSR=1 -> virtual-instruction exception (cause=22).
     */
    EXPECT_VIRTUAL_INST(run_in_vs_mode(vs_exec_sret, 0));

    /* Clean up. */
    asm volatile ("csrr %0, mstatus" : "=r"(ms));
    ms &= ~(1UL << 22);
    asm volatile ("csrw mstatus, %0" :: "r"(ms));
    hstatus_set_vtsr(false);

    HYP_TEST_END();
}
