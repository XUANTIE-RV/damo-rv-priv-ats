/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ssdbltrp_trap_asm.S - Ssdbltrp/Smdbltrp M-mode trap entry
 *
 * When Smdbltrp is enabled, a trap to M-mode sets mstatus.MDT=1.
 * Any CSR access with MDT=1 may cause a double-trap (fatal).
 *
 * Strategy: Use single MRET with MPP=M to clear MDT while staying in
 * M-mode, similar to ssctr_trap_asm.S. Save sp and CSRs to static
 * memory BEFORE MRET, then restore after MDT is cleared.
 */

#include "encoding.h"

#if __riscv_xlen == 64
    #define REGSIZE 8
    #define LOAD    ld
    #define STORE   sd
#else
    #define REGSIZE 4
    #define LOAD    lw
    #define STORE   sw
#endif

#define CONTEXT_SIZE (REGSIZE * 31)

/* MDT bit position in mstatus (bit 42 for RV64) */
#define MSTATUS_MDT_BIT     (1 << 42)

/* Save all general-purpose registers (x1-x31) */
.macro SAVE_CONTEXT
    addi    sp, sp, -CONTEXT_SIZE

    STORE   x1,  0*REGSIZE(sp)
    STORE   x3,  2*REGSIZE(sp)
    STORE   x4,  3*REGSIZE(sp)
    STORE   x5,  4*REGSIZE(sp)
    STORE   x6,  5*REGSIZE(sp)
    STORE   x7,  6*REGSIZE(sp)
    STORE   x8,  7*REGSIZE(sp)
    STORE   x9,  8*REGSIZE(sp)
    STORE   x10, 9*REGSIZE(sp)
    STORE   x11, 10*REGSIZE(sp)
    STORE   x12, 11*REGSIZE(sp)
    STORE   x13, 12*REGSIZE(sp)
    STORE   x14, 13*REGSIZE(sp)
    STORE   x15, 14*REGSIZE(sp)
    STORE   x16, 15*REGSIZE(sp)
    STORE   x17, 16*REGSIZE(sp)
    STORE   x18, 17*REGSIZE(sp)
    STORE   x19, 18*REGSIZE(sp)
    STORE   x20, 19*REGSIZE(sp)
    STORE   x21, 20*REGSIZE(sp)
    STORE   x22, 21*REGSIZE(sp)
    STORE   x23, 22*REGSIZE(sp)
    STORE   x24, 23*REGSIZE(sp)
    STORE   x25, 24*REGSIZE(sp)
    STORE   x26, 25*REGSIZE(sp)
    STORE   x27, 26*REGSIZE(sp)
    STORE   x28, 27*REGSIZE(sp)
    STORE   x29, 28*REGSIZE(sp)
    STORE   x30, 29*REGSIZE(sp)
    STORE   x31, 30*REGSIZE(sp)
.endm

/* Restore all general-purpose registers (x1-x31) */
.macro RESTORE_CONTEXT
    LOAD    x1,  0*REGSIZE(sp)
    LOAD    x3,  2*REGSIZE(sp)
    LOAD    x4,  3*REGSIZE(sp)
    LOAD    x5,  4*REGSIZE(sp)
    LOAD    x6,  5*REGSIZE(sp)
    LOAD    x7,  6*REGSIZE(sp)
    LOAD    x8,  7*REGSIZE(sp)
    LOAD    x9,  8*REGSIZE(sp)
    LOAD    x10, 9*REGSIZE(sp)
    LOAD    x11, 10*REGSIZE(sp)
    LOAD    x12, 11*REGSIZE(sp)
    LOAD    x13, 12*REGSIZE(sp)
    LOAD    x14, 13*REGSIZE(sp)
    LOAD    x15, 14*REGSIZE(sp)
    LOAD    x16, 15*REGSIZE(sp)
    LOAD    x17, 16*REGSIZE(sp)
    LOAD    x18, 17*REGSIZE(sp)
    LOAD    x19, 18*REGSIZE(sp)
    LOAD    x20, 19*REGSIZE(sp)
    LOAD    x21, 20*REGSIZE(sp)
    LOAD    x22, 21*REGSIZE(sp)
    LOAD    x23, 22*REGSIZE(sp)
    LOAD    x24, 23*REGSIZE(sp)
    LOAD    x25, 24*REGSIZE(sp)
    LOAD    x26, 25*REGSIZE(sp)
    LOAD    x27, 26*REGSIZE(sp)
    LOAD    x28, 27*REGSIZE(sp)
    LOAD    x29, 28*REGSIZE(sp)
    LOAD    x30, 29*REGSIZE(sp)
    LOAD    x31, 30*REGSIZE(sp)

    addi    sp, sp, CONTEXT_SIZE
.endm

    /* Static storage for trap context (used while MDT=1) */
    .section .data
    .align 3
    .global smdbltrp_saved_sp
smdbltrp_saved_sp:
    .dword  0

    .align 3
    .global smdbltrp_saved_mepc
smdbltrp_saved_mepc:
    .dword  0

    .align 3
    .global smdbltrp_saved_mcause
smdbltrp_saved_mcause:
    .dword  0

    .align 3
    .global smdbltrp_saved_mtval
smdbltrp_saved_mtval:
    .dword  0

    /* mstatus snapshot captured at trap entry, BEFORE the MDT-clearing
     * MRET below overwrites it.  Tests use it to verify the hardware
     * trap-entry writes (e.g. MPP on a double-trap delivery). */
    .align 3
    .global smdbltrp_saved_mstatus
smdbltrp_saved_mstatus:
    .dword  0

    /* mtval2 snapshot captured at M-mode trap entry.  On a double-trap
     * delivery (Ssdbltrp, norm:sstatus_sdt_trap) the hardware writes
     * the original trap cause into mtval2.  The value must be captured
     * here because any later M-mode trap entry (e.g. the framework's
     * ecall-based return to M-mode) rewrites mtval2. */
    .align 3
    .global smdbltrp_saved_mtval2
smdbltrp_saved_mtval2:
    .dword  0

    /* mtval2 snapshot captured at S-mode trap entry.  A trap delivered
     * to S-mode must not modify mtval2 (norm:mtval2_trapval); tests
     * compare this entry-time capture against a pre-loaded sentinel. */
    .align 3
    .global ssdbltrp_s_trap_mtval2
ssdbltrp_s_trap_mtval2:
    .dword  0

    /* One-shot gate for the entry mstatus capture: set by the test
     * before triggering, consumed by the first M-mode trap entry.
     * Several cause-16 entries may occur during a probe (escalation
     * loops); only the FIRST entry describes the hardware double-trap
     * delivery under test. */
    .align 3
    .global smdbltrp_mstatus_wanted
smdbltrp_mstatus_wanted:
    .dword  0

    /* Count of M-mode trap entries observed while the gate was set */
    .align 3
    .global smdbltrp_m_entries
smdbltrp_m_entries:
    .dword  0

    /* DBG ring of the last 4 M-mode entries: {mepc, mcause, mstatus,
     * mtval2} per slot, newest at (idx mod 4). */
    .align 3
    .global smdbltrp_dbg_ring
smdbltrp_dbg_ring:
    .rept 16
    .dword  0
    .endr
    .align 3
    .global smdbltrp_dbg_ring_idx
smdbltrp_dbg_ring_idx:
    .dword  0

    /* DBG: sstatus/menvcfg read by the probe trampoline right before
     * its mret into S-mode (verification of the SDT/DTE precondition). */
    .align 3
    .global ssdbltrp_dbg_pre_sstatus
ssdbltrp_dbg_pre_sstatus:
    .dword  0
    .align 3
    .global ssdbltrp_dbg_pre_menvcfg
ssdbltrp_dbg_pre_menvcfg:
    .dword  0

    /* DBG: S-mode trap entry counter */
    .align 3
    .global ssdbltrp_dbg_s_entries
ssdbltrp_dbg_s_entries:
    .dword  0

    /* DBG: probe re-arm action counter */
    .align 3
    .global ssdbltrp_dbg_s_rearms
ssdbltrp_dbg_s_rearms:
    .dword  0

    /* DBG: scause/sepc observed at the last S-mode trap entry */
    .align 3
    .global ssdbltrp_dbg_s_cause
ssdbltrp_dbg_s_cause:
    .dword  0
    .align 3
    .global ssdbltrp_dbg_s_sepc
ssdbltrp_dbg_s_sepc:
    .dword  0

    /* sepc of the last re-armed/handled probe trap; used to detect
     * repeated deliveries of the same trap */
    .align 3
ssdbltrp_s_last_epc:
    .dword  0

    /* DBG: probe_active flag value observed at the last S entry */
    .align 3
    .global ssdbltrp_dbg_s_active_seen
ssdbltrp_dbg_s_active_seen:
    .dword  0

    .section .text
    .balign 16
    .global smdbltrp_m_trap_entry
smdbltrp_m_trap_entry:
    /* When Smdbltrp is enabled and we trap to M-mode:
     * - Hardware sets mstatus.MDT = 1
     * - Any CSR access with MDT=1 may trigger a double trap (fatal)
     * - We must use MRET to clear MDT BEFORE calling the C handler
     *
     * Strategy (single MRET, same as ssctr):
     * 1. Save sp to memory (memory access works with MDT=1)
     * 2. Read mepc/mcause/mtval and save to memory (CSR reads)
     * 3. Set mepc to after_mret, MPP=M, execute MRET -> clears MDT
     * 4. After MRET in M-mode with MDT=0: restore sp, SAVE_CONTEXT,
     *    restore CSRs from memory, call C handler, restore, MRET
     */

    /* Step 1: Save sp to memory (works with MDT=1) */
    la      t0, smdbltrp_saved_sp
    STORE   sp, 0(t0)

    /* Step 2: Save mepc, mcause, mtval to memory */
    csrr    t0, mepc
    la      t1, smdbltrp_saved_mepc
    STORE   t0, 0(t1)

    csrr    t0, mcause
    la      t1, smdbltrp_saved_mcause
    STORE   t0, 0(t1)

    csrr    t0, mtval
    la      t1, smdbltrp_saved_mtval
    STORE   t0, 0(t1)

    csrr    t0, mstatus
    /* One-shot capture: keep the FIRST entry's mstatus only, and
     * count entries so the test can detect escalation loops.
     * (mepc/mcause of the escalation are obtained from the C-side
     * trap_dt_snap; they are not re-read here so the capture cannot
     * observe a later nested entry.) */
    la      t1, smdbltrp_mstatus_wanted
    LOAD    t2, 0(t1)
    beqz    t2, 1f
    li      t2, 0
    STORE   t2, 0(t1)
    la      t1, smdbltrp_saved_mstatus
    STORE   t0, 0(t1)
1:
    la      t1, smdbltrp_m_entries
    LOAD    t2, 0(t1)
    addi    t2, t2, 1
    STORE   t2, 0(t1)

    /* DBG ring slot = (idx mod 4) * 4 dwords */
    la      t1, smdbltrp_dbg_ring_idx
    LOAD    t2, 0(t1)
    andi    t3, t2, 3
    slli    t3, t3, 5
    la      t4, smdbltrp_dbg_ring
    add     t3, t3, t4
    csrr    t4, mepc
    STORE   t4, 0(t3)
    csrr    t4, mcause
    STORE   t4, REGSIZE(t3)
    STORE   t0, 2*REGSIZE(t3)       /* mstatus read above */
    csrr    t4, mtval2
    STORE   t4, 3*REGSIZE(t3)
    addi    t2, t2, 1
    la      t1, smdbltrp_dbg_ring_idx
    STORE   t2, 0(t1)

    /* Step 3: Set up MRET to clear MDT while staying in M-mode */
    la      t0, after_mret
    csrw    mepc, t0

    /* Set mstatus.MPP = M-mode (bits [12:11] = 0b11) */
    li      t0, (3 << 11)       /* MPP = M-mode */
    csrw    mstatus, t0

    /* Execute MRET to M-mode - this clears MDT */
    mret

after_mret:
    /* Now in M-mode with MDT=0, we can safely access CSRs */

    /* mtval2 is captured by the C-handler entry hook instead (see
     * trap_m_entry_mtval2_hook): the hook reads at the earliest
     * handler point where the hardware trap-entry write is guaranteed
     * to be visible. */

    /* Restore sp from memory */
    la      t0, smdbltrp_saved_sp
    LOAD    sp, 0(t0)

    /* Save all registers to stack */
    SAVE_CONTEXT

    /* Restore original mcause and mtval from memory, write to CSRs */
    la      t0, smdbltrp_saved_mcause
    LOAD    t0, 0(t0)
    csrw    mcause, t0

    la      t0, smdbltrp_saved_mtval
    LOAD    t0, 0(t0)
    csrw    mtval, t0

    /* Restore original mepc from memory, write to CSR */
    la      t0, smdbltrp_saved_mepc
    LOAD    t0, 0(t0)
    csrw    mepc, t0

    /* Also save mcause/mtval to stack for C handler access */
    la      t0, smdbltrp_saved_mcause
    LOAD    t0, 0(t0)
    STORE   t0, 25*REGSIZE(sp)

    la      t0, smdbltrp_saved_mtval
    LOAD    t0, 0(t0)
    STORE   t0, 26*REGSIZE(sp)

    /* Call the C trap handler (defined in common/trap.c) */
    jal     ra, m_trap_handler

    /* Restore mcause and mtval from stack (C handler may have modified) */
    LOAD    t0, 25*REGSIZE(sp)
    csrw    mcause, t0
    LOAD    t0, 26*REGSIZE(sp)
    csrw    mtval, t0

    /* mepc: keep the CSR value.  m_trap_handler updates mepc to skip
     * the faulting instruction (or applies a recovery address); the
     * trap-entry value saved in memory is stale and must not be
     * restored, otherwise the faulting instruction would re-execute. */

    /* Restore all registers */
    RESTORE_CONTEXT

    /* Return from trap */
    mret

/* ===================================================================
 * S-mode trap entry for Ssdbltrp tests
 *
 * Wraps the common s_trap_handler to capture mtval2 at S-mode trap
 * entry: a trap delivered to S-mode must not modify mtval2, and any
 * later M-mode trap entry (the ecall-based return path) rewrites it,
 * so the only reliable observation point is the S-mode entry itself.
 * =================================================================== */
    .balign 16
    .global ssdbltrp_s_trap_entry
ssdbltrp_s_trap_entry:
    SAVE_CONTEXT

    /* Repeat-delivery robustness is handled in the common
     * s_trap_handler (Ssdbltrp probe tolerance): it keeps the
     * expectation armed across repeated probe-trap deliveries and
     * forces progress when the skip is ignored. */

    /* NOTE: no mtval2 read here — mtval2 is an M-mode CSR; reading it
     * in S-mode raises illegal-instruction on implementations that do
     * not expose it below M-mode.  The S-mode delivery observation is
     * done at the probe continuation point instead. */

    /* Call the C trap handler (defined in common/trap.c) */
    jal     ra, s_trap_handler

    /* Honor the handler return value like common _trap_return:
     * PRIV_M (3) -> mret (GOTO_PRIV escalation path), else sret. */
    li      t0, 3
    beq     a0, t0, .Lssdbltrp_s_do_mret

    /* Restore all registers */
    RESTORE_CONTEXT

    /* Return from trap */
    sret

.Lssdbltrp_s_do_mret:
    RESTORE_CONTEXT
    mret

/* ===================================================================
 * S-mode probe trampoline for Ssdbltrp trap-delivery tests
 *
 * ssdbltrp_run_s_probe() enters S-mode via mret (MPP=S) and runs:
 *   1. An illegal instruction, delegated to S-mode by the test:
 *      - with sstatus.SDT=1 the delivery is an unexpected trap and
 *        escalates to a double-trap into M-mode (cause 16).  The
 *        M-mode handler records it, clears SDT, forces MPP=M and
 *        redirects mepc to _exec_return_addr (the test points it at
 *        ssdbltrp_probe_return), so mret resumes in M-mode there.
 *      - with SDT=0 the trap is delivered to S-mode (SDT becomes 1);
 *        the S-mode handler skips the instruction and srets to the
 *        return label, resuming in S-mode.
 *   2. The return label issues an ECALL_GOTO_PRIV(PRIV_M) ecall that
 *      the test must NOT delegate, so the M-mode handler brings both
 *      scenarios back to M-mode.
 * current_priv tracking intentionally stays at PRIV_M throughout.
 * =================================================================== */
    .balign 16
    .global ssdbltrp_run_s_probe
ssdbltrp_run_s_probe:
    /* Set MPP = S-mode and clear MPV for the entry mret */
    csrr    t0, mstatus
    li      t1, ((3 << 11) | (1 << 39))
    not     t1, t1
    and     t0, t0, t1
    li      t1, (1 << 11)             /* MPP = S-mode */
    or      t0, t0, t1
    csrw    mstatus, t0

    la      t0, .Lssdbltrp_probe_body
    csrw    mepc, t0

    /* DBG: snapshot sstatus/menvcfg immediately before mret */
    csrr    t0, sstatus
    la      t1, ssdbltrp_dbg_pre_sstatus
    STORE   t0, 0(t1)
    csrr    t0, menvcfg
    la      t1, ssdbltrp_dbg_pre_menvcfg
    STORE   t0, 0(t1)

    mret

    .balign 4
.Lssdbltrp_probe_body:
    /* Illegal instruction (trap trigger): all-ones is a 4-byte
     * non-compressed illegal encoding, so handler skip lands exactly
     * on the rearm label. */
    .word   0xffffffff

    .balign 4
    .global ssdbltrp_probe_rearm
ssdbltrp_probe_rearm:
    /* Continuation point after the probe trap.  Reached either by the
     * M-mode double-trap redirect (_exec_return_addr, resumes in
     * M-mode) or by the S-mode handler tolerance skip (resumes in
     * S-mode).  Must therefore be legal in BOTH modes: no M-CSR
     * access, no C call, just set the ecall arguments and ecall.
     * Cause 9 is not delegated by the tests, so the M-mode handler's
     * GOTO_PRIV branch (which does not record) returns to M-mode in
     * both scenarios, preserving the first trap record for the
     * assertions. */
    .global ssdbltrp_probe_return
ssdbltrp_probe_return:
    la      t0, ecall_args
    li      t1, 1                   /* ECALL_GOTO_PRIV */
    STORE   t1, 0(t0)
    li      t1, 3                   /* PRIV_M */
    STORE   t1, REGSIZE(t0)
    ecall
    ret
