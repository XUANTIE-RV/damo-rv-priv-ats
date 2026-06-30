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
 * memory BEFORE MRET (works even with MDT=1 on Spike), then restore
 * after MDT is cleared.
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
     * 2. Read mepc/mcause/mtval and save to memory (CSR reads work
     *    with MDT=1 on Spike since no exception is triggered)
     * 3. Set mepc to after_mret, MPP=M, execute MRET -> clears MDT
     * 4. After MRET in M-mode with MDT=0: restore sp, SAVE_CONTEXT,
     *    restore CSRs from memory, call C handler, restore, MRET
     */

    /* Step 1: Save sp to memory (works with MDT=1) */
    la      t0, smdbltrp_saved_sp
    STORE   sp, 0(t0)

    /* Step 2: Save mepc, mcause, mtval to memory (CSR reads work on Spike) */
    csrr    t0, mepc
    la      t1, smdbltrp_saved_mepc
    STORE   t0, 0(t1)

    csrr    t0, mcause
    la      t1, smdbltrp_saved_mcause
    STORE   t0, 0(t1)

    csrr    t0, mtval
    la      t1, smdbltrp_saved_mtval
    STORE   t0, 0(t1)

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

    /* Also save mcause/mtval/mepc to stack for C handler access */
    la      t0, smdbltrp_saved_mcause
    LOAD    t0, 0(t0)
    STORE   t0, 25*REGSIZE(sp)

    la      t0, smdbltrp_saved_mtval
    LOAD    t0, 0(t0)
    STORE   t0, 26*REGSIZE(sp)

    la      t0, smdbltrp_saved_mepc
    LOAD    t0, 0(t0)
    STORE   t0, 27*REGSIZE(sp)

    /* Call the C trap handler (defined in common/trap.c) */
    jal     ra, m_trap_handler

    /* Restore mcause and mtval from stack (C handler may have modified) */
    LOAD    t0, 25*REGSIZE(sp)
    csrw    mcause, t0
    LOAD    t0, 26*REGSIZE(sp)
    csrw    mtval, t0

    /* Restore mepc from stack */
    LOAD    t0, 27*REGSIZE(sp)
    csrw    mepc, t0

    /* Restore all registers */
    RESTORE_CONTEXT

    /* Return from trap */
    mret
