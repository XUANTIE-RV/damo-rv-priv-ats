/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * test_helpers.c - Shared helpers for Hypervisor Extension tests
 *
 * Provides VS/VU-mode trampoline functions for CSR access, instruction
 * execution, delegation configuration, and G-stage fault helpers.
 * =================================================================== */

#include "test_helpers.h"

/* ===================================================================
 * VS/VU-mode CSR access trampolines.
 *
 * These are passed to run_in_vs_mode() / run_in_vu_mode(). In V=1,
 * S-level CSR names map to their VS counterparts automatically.
 * =================================================================== */

uintptr_t vs_read_sstatus(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, sstatus" : "=r"(v));
    return v;
}

uintptr_t vs_write_sstatus(uintptr_t val) {
    asm volatile ("csrw sstatus, %0" :: "r"(val));
    return 0;
}

uintptr_t vs_read_sie(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, sie" : "=r"(v));
    return v;
}

uintptr_t vs_read_sip(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, sip" : "=r"(v));
    return v;
}

uintptr_t vs_read_satp(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, satp" : "=r"(v));
    return v;
}

uintptr_t vs_read_stvec(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, stvec" : "=r"(v));
    return v;
}

uintptr_t vs_read_sscratch(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, sscratch" : "=r"(v));
    return v;
}

uintptr_t vs_read_sepc(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, sepc" : "=r"(v));
    return v;
}

uintptr_t vs_read_scause(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, scause" : "=r"(v));
    return v;
}

uintptr_t vs_read_stval(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, stval" : "=r"(v));
    return v;
}

uintptr_t vs_write_satp(uintptr_t val) {
    asm volatile ("csrw satp, %0" :: "r"(val));
    return 0;
}

uintptr_t vs_write_sie(uintptr_t val) {
    asm volatile ("csrw sie, %0" :: "r"(val));
    return 0;
}

uintptr_t vs_write_sip_ssip(uintptr_t val) {
    uintptr_t bit = val ? (1UL << 1) : 0;
    asm volatile ("csrw sip, %0" :: "r"(bit));
    return 0;
}

uintptr_t vs_write_sscratch(uintptr_t val) {
    asm volatile ("csrw sscratch, %0" :: "r"(val));
    return 0;
}

uintptr_t vs_write_sepc(uintptr_t val) {
    asm volatile ("csrw sepc, %0" :: "r"(val));
    return 0;
}

uintptr_t vs_write_scause(uintptr_t val) {
    asm volatile ("csrw scause, %0" :: "r"(val));
    return 0;
}

uintptr_t vs_write_stval(uintptr_t val) {
    asm volatile ("csrw stval, %0" :: "r"(val));
    return 0;
}

uintptr_t vs_write_stvec(uintptr_t val) {
    asm volatile ("csrw stvec, %0" :: "r"(val));
    return 0;
}

uintptr_t vs_read_senvcfg(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x10A" : "=r"(v));  /* senvcfg */
    return v;
}

uintptr_t vs_write_senvcfg(uintptr_t val) {
    asm volatile ("csrw 0x10A, %0" :: "r"(val));  /* senvcfg */
    return 0;
}

uintptr_t vs_read_scounteren(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x106" : "=r"(v));  /* scounteren */
    return v;
}

uintptr_t vs_write_scounteren(uintptr_t val) {
    asm volatile ("csrw 0x106, %0" :: "r"(val));  /* scounteren */
    return 0;
}

/* ===================================================================
 * VS/VU-mode instruction execution trampolines.
 * =================================================================== */

extern uintptr_t ecall_args[2];

uintptr_t vs_exec_sret(uintptr_t arg) {
    (void)arg;
    /* Set sepc (= vsepc when V=1) to the instruction after sret,
     * so that if VTSR=0 and sret actually executes, it lands safely
     * at the return path instead of jumping to address 0. */
    asm volatile (
        "la   t0, 1f\n\t"
        "csrw sepc, t0\n\t"
        "sret\n\t"
        "1:\n\t"
        ::: "t0", "memory"
    );
    return 0;
}

uintptr_t vu_exec_sret(uintptr_t arg) {
    (void)arg;
    /* VU-mode variant: execute sret directly without writing sepc first,
     * since VU-mode cannot access S-level CSRs (csrw sepc would itself
     * trap and consume the armed state before sret is reached). */
    asm volatile ("sret" ::: "memory");
    return 0;
}

uintptr_t vu_nop(uintptr_t arg) {
    /* Simple identity function safe for VU-mode (no CSR access). */
    return arg;
}

uintptr_t vs_exec_wfi(uintptr_t arg) {
    (void)arg;
    asm volatile ("wfi");
    return 0;
}

uintptr_t vs_exec_sfence_vma(uintptr_t arg) {
    (void)arg;
    asm volatile ("sfence.vma x0, x0");
    return 0;
}

uintptr_t vs_exec_sinval_vma(uintptr_t arg) {
    (void)arg;
    /* sinval.vma x0, x0 encoding: 0x16000073 */
    asm volatile (".4byte 0x16000073");
    return 0;
}

uintptr_t vs_exec_ecall(uintptr_t arg) {
    (void)arg;
    ecall_args[0] = 0;  /* NOT ECALL_GOTO_PRIV */
    asm volatile ("ecall" ::: "memory");
    return 0;
}

uintptr_t vs_exec_ebreak(uintptr_t arg) {
    (void)arg;
    asm volatile ("ebreak");
    return 0;
}

uintptr_t vs_exec_illegal(uintptr_t arg) {
    (void)arg;
    /* UNIMP (0x00000000) is guaranteed illegal on all implementations. */
    asm volatile (".word 0x00000000");
    return 0;
}

uintptr_t vs_nop_fn(uintptr_t arg) {
    (void)arg;
    return 0;
}

/* ===================================================================
 * VS/VU-mode H-CSR access trampolines (should cause virtual-inst).
 * =================================================================== */

uintptr_t vs_read_hstatus(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x600" : "=r"(v));  /* hstatus */
    return v;
}

uintptr_t vs_read_hedeleg(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x602" : "=r"(v));  /* hedeleg */
    return v;
}

uintptr_t vs_read_hgatp(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x680" : "=r"(v));  /* hgatp */
    return v;
}

uintptr_t vs_read_vsstatus_direct(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x200" : "=r"(v));  /* vsstatus addr */
    return v;
}

uintptr_t vs_exec_hlv_w(uintptr_t arg) {
    uintptr_t v;
    /* hlv.w rd, (rs1): .insn r 0x73, 0x4, 0x34, rd, rs1, x0 */
    asm volatile (".insn r 0x73, 0x4, 0x34, %0, %1, x0"
                  : "=r"(v) : "r"(arg));
    return v;
}

uintptr_t vs_exec_hsv_w(uintptr_t arg) {
    /* hsv.w rs2, (rs1): .insn r 0x73, 0x4, 0x35, x0, rs1, rs2 */
    asm volatile (".insn r 0x73, 0x4, 0x35, x0, %0, x0"
                  :: "r"(arg));
    return 0;
}

uintptr_t vs_exec_hlvx_wu(uintptr_t arg) {
    uintptr_t v;
    /* hlvx.wu rd, (rs1): .insn r 0x73, 0x4, 0x34, rd, rs1, x3 */
    asm volatile (".insn r 0x73, 0x4, 0x34, %0, %1, x3"
                  : "=r"(v) : "r"(arg));
    return v;
}

uintptr_t vs_exec_hfence_vvma(uintptr_t arg) {
    (void)arg;
    asm volatile (".4byte 0x22000073");  /* hfence.vvma x0, x0 */
    return 0;
}

uintptr_t vs_exec_hfence_gvma(uintptr_t arg) {
    (void)arg;
    asm volatile (".4byte 0x62000073");  /* hfence.gvma x0, x0 */
    return 0;
}

/* ===================================================================
 * Additional H-CSR access trampolines.
 *
 * Each attempts to read an HS-only CSR by its raw address.
 * All should trigger virtual-instruction exception when V=1.
 * =================================================================== */

uintptr_t vs_read_hideleg(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x603" : "=r"(v));  /* hideleg */
    return v;
}

uintptr_t vs_read_hcounteren(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x606" : "=r"(v));  /* hcounteren */
    return v;
}

uintptr_t vs_read_htimedelta(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x605" : "=r"(v));  /* htimedelta */
    return v;
}

uintptr_t vs_read_hip(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x644" : "=r"(v));  /* hip */
    return v;
}

uintptr_t vs_read_hie(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x604" : "=r"(v));  /* hie */
    return v;
}

uintptr_t vs_read_hvip(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x645" : "=r"(v));  /* hvip */
    return v;
}

uintptr_t vs_read_henvcfg(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, 0x60A" : "=r"(v));  /* henvcfg */
    return v;
}

uintptr_t vs_write_hstatus(uintptr_t val) {
    asm volatile ("csrw 0x600, %0" :: "r"(val));  /* hstatus */
    return 0;
}

/* ===================================================================
 * Additional HLV/HSV width-variant trampolines.
 *
 * Each should trigger virtual-instruction exception when V=1.
 * =================================================================== */

uintptr_t vs_exec_hlv_b(uintptr_t arg) {
    uintptr_t v;
    /* hlv.b rd, (rs1): funct7=0x30, funct3=4 */
    asm volatile (".insn r 0x73, 0x4, 0x30, %0, %1, x0"
                  : "=r"(v) : "r"(arg));
    return v;
}

uintptr_t vs_exec_hlv_h(uintptr_t arg) {
    uintptr_t v;
    /* hlv.h rd, (rs1): funct7=0x32, funct3=4 */
    asm volatile (".insn r 0x73, 0x4, 0x32, %0, %1, x0"
                  : "=r"(v) : "r"(arg));
    return v;
}

uintptr_t vs_exec_hlv_d(uintptr_t arg) {
    uintptr_t v;
    /* hlv.d rd, (rs1): funct7=0x36, funct3=4 */
    asm volatile (".insn r 0x73, 0x4, 0x36, %0, %1, x0"
                  : "=r"(v) : "r"(arg));
    return v;
}

uintptr_t vs_exec_hlvx_bu(uintptr_t arg) {
    uintptr_t v;
    /* hlvx.bu rd, (rs1): funct7=0x30, funct3=4, rs2=3 */
    asm volatile (".insn r 0x73, 0x4, 0x30, %0, %1, x3"
                  : "=r"(v) : "r"(arg));
    return v;
}

uintptr_t vs_exec_hlvx_hu(uintptr_t arg) {
    uintptr_t v;
    /* hlvx.hu rd, (rs1): funct7=0x32, funct3=4, rs2=3 */
    asm volatile (".insn r 0x73, 0x4, 0x32, %0, %1, x3"
                  : "=r"(v) : "r"(arg));
    return v;
}

uintptr_t vs_exec_hsv_b(uintptr_t arg) {
    /* hsv.b rs2, (rs1): funct7=0x31, funct3=4 */
    asm volatile (".insn r 0x73, 0x4, 0x31, x0, %0, x0"
                  :: "r"(arg));
    return 0;
}

uintptr_t vs_exec_hsv_h(uintptr_t arg) {
    /* hsv.h rs2, (rs1): funct7=0x33, funct3=4 */
    asm volatile (".insn r 0x73, 0x4, 0x33, x0, %0, x0"
                  :: "r"(arg));
    return 0;
}

uintptr_t vs_exec_hsv_d(uintptr_t arg) {
    /* hsv.d rs2, (rs1): funct7=0x37, funct3=4 */
    asm volatile (".insn r 0x73, 0x4, 0x37, x0, %0, x0"
                  :: "r"(arg));
    return 0;
}

/* ===================================================================
 * Counter access trampolines.
 * =================================================================== */

uintptr_t vs_read_cycle(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, cycle" : "=r"(v));
    return v;
}

uintptr_t vs_read_time(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, time" : "=r"(v));
    return v;
}

uintptr_t vs_read_instret(uintptr_t arg) {
    (void)arg;
    uintptr_t v;
    asm volatile ("csrr %0, instret" : "=r"(v));
    return v;
}

/* ===================================================================
 * U-mode HLV trampoline (for HU bit testing).
 *
 * Executed via run_in_priv(PRIV_U, ...) in V=0 U-mode.
 * With hstatus.HU=0 this triggers illegal-instruction (cause=2).
 * With hstatus.HU=1 the HLV.W executes (may fault on translation).
 * =================================================================== */

uintptr_t u_exec_hlv_w(uintptr_t addr) {
    uintptr_t v;
    /* hlv.w rd, (rs1): .insn r 0x73, 0x4, 0x34, rd, rs1, x0 */
    asm volatile (".insn r 0x73, 0x4, 0x34, %0, %1, x0"
                  : "=r"(v) : "r"(addr));
    return v;
}

/* ===================================================================
 * Delegation configuration helpers.
 * =================================================================== */

void setup_deleg_to_vs(uintptr_t exc_mask, uintptr_t int_mask) {
    /* First delegate from M to HS via medeleg/mideleg. */
    uintptr_t medeleg;
    asm volatile ("csrr %0, medeleg" : "=r"(medeleg));
    medeleg |= exc_mask;
    asm volatile ("csrw medeleg, %0" :: "r"(medeleg));

    uintptr_t mideleg;
    asm volatile ("csrr %0, mideleg" : "=r"(mideleg));
    mideleg |= int_mask;
    asm volatile ("csrw mideleg, %0" :: "r"(mideleg));

    /* Then delegate from HS to VS via hedeleg/hideleg. */
    uintptr_t hedeleg = hedeleg_read();
    hedeleg |= exc_mask;
    hedeleg_write(hedeleg);

    uintptr_t hideleg = hideleg_read();
    hideleg |= int_mask;
    hideleg_write(hideleg);
}

void setup_deleg_to_hs(uintptr_t exc_mask) {
    /* Delegate from M to HS via medeleg only. */
    uintptr_t medeleg;
    asm volatile ("csrr %0, medeleg" : "=r"(medeleg));
    medeleg |= exc_mask;
    asm volatile ("csrw medeleg, %0" :: "r"(medeleg));

    /* Clear hedeleg so trap stays at HS-mode. */
    uintptr_t hedeleg = hedeleg_read();
    hedeleg &= ~exc_mask;
    hedeleg_write(hedeleg);
}

void clear_all_deleg(void) {
    asm volatile ("csrw medeleg, zero");
    asm volatile ("csrw mideleg, zero");
    hedeleg_write(0);
    hideleg_write(0);
}

/* ===================================================================
 * G-stage fault helpers.
 * =================================================================== */

void setup_gstage_with_victim(two_stage_ctx_t *ctx,
                              uintptr_t victim_gpa,
                              uintptr_t victim_flags)
{
    gpt_pool_reset();
    two_stage_init(ctx, SATP_MODE_BARE, HGATP_MODE_SV39X4);

    /* Kernel/UART at 2MB. */
    uintptr_t lo_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t lo_end  = (uintptr_t)__vm_test_region_start
                        & ~(PAGE_SIZE_2M - 1);
    two_stage_setup_identity(ctx, lo_base, lo_end - lo_base,
                             G_FLAGS_RWXU_AD, PT_LEVEL_2M);

    /* Test region at 4KB. */
    uintptr_t r_start = (uintptr_t)__vm_test_region_start;
    uintptr_t r_size  = (uintptr_t)__vm_test_region_end - r_start;
    two_stage_setup_identity(ctx, r_start, r_size,
                             G_FLAGS_RWXU_AD, PT_LEVEL_4K);

    /* Override victim page flags. */
    uintptr_t victim_page = victim_gpa & ~(PAGE_SIZE_4K - 1);
    two_stage_g_map_page(ctx, victim_page, victim_page,
                         victim_flags, PT_LEVEL_4K);
}

bool fire_vs_load_fault(uintptr_t victim_gpa, uintptr_t flags) {
    two_stage_ctx_t ctx;
    setup_gstage_with_victim(&ctx, victim_gpa, flags);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_load_expect_fault, victim_gpa);
    bool fired = trap_was_triggered();
    trap_expect_end();

    two_stage_cleanup(&ctx);
    return fired;
}

bool fire_vs_store_fault(uintptr_t victim_gpa, uintptr_t flags) {
    two_stage_ctx_t ctx;
    setup_gstage_with_victim(&ctx, victim_gpa, flags);

    trap_expect_begin();
    (void)two_stage_run_in_vs(&ctx, test_vs_store_expect_fault, victim_gpa);
    bool fired = trap_was_triggered();
    trap_expect_end();

    two_stage_cleanup(&ctx);
    return fired;
}
