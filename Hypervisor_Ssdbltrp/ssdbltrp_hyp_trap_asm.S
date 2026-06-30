/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ssdbltrp_hyp_trap_asm.S - Ssdbltrp M-mode trap handler for Hypervisor tests
 *
 * When Ssdbltrp is enabled, a trap to M-mode sets mstatus.MDT=1.
 * Any subsequent S-mode trap with MDT=1 triggers a double-trap to M-mode.
 *
 * Strategy: Use a single MRET to clear MDT before processing the trap.
 * Save context to a global structure, MRET to clear MDT, then call
 * the C trap handler.
 */

    .section .text
    .globl ssdbltrp_hyp_m_trap_entry
    .type  ssdbltrp_hyp_m_trap_entry, @function
    .balign 4

ssdbltrp_hyp_m_trap_entry:
    /* Save all caller-saved registers to the trap context structure.
     * We use t0-t6 as temporaries since they're caller-saved anyway.
     */

    /* Save sp to global */
    la      t0, ssdbltrp_hyp_trap_ctx
    sd      sp, 16(t0)      /* ctx.sp */

    /* Save ra, gp, tp */
    sd      ra, 0(t0)       /* ctx.ra */
    sd      gp, 8(t0)       /* ctx.gp */
    sd      tp, 24(t0)      /* ctx.tp */

    /* Save t0-t6 */
    sd      t0, 32(t0)      /* ctx.t0 */
    sd      t1, 40(t0)      /* ctx.t1 */
    sd      t2, 48(t0)      /* ctx.t2 */
    sd      t3, 56(t0)      /* ctx.t3 */
    sd      t4, 64(t0)      /* ctx.t4 */
    sd      t5, 72(t0)      /* ctx.t5 */
    sd      t6, 80(t0)      /* ctx.t6 */

    /* Save a0-a7 */
    sd      a0, 88(t0)      /* ctx.a0 */
    sd      a1, 96(t0)      /* ctx.a1 */
    sd      a2, 104(t0)     /* ctx.a2 */
    sd      a3, 112(t0)     /* ctx.a3 */
    sd      a4, 120(t0)     /* ctx.a4 */
    sd      a5, 128(t0)     /* ctx.a5 */
    sd      a6, 136(t0)     /* ctx.a6 */
    sd      a7, 144(t0)     /* ctx.a7 */

    /* Read trap CSRs (these reads are safe even with MDT=1) */
    csrr    t1, mepc
    csrr    t2, mcause
    csrr    t3, mtval
    csrr    t4, mstatus

    /* Save trap CSRs to context */
    la      t0, ssdbltrp_hyp_trap_ctx
    sd      t1, 152(t0)     /* ctx.mepc */
    sd      t2, 160(t0)     /* ctx.mcause */
    sd      t3, 168(t0)     /* ctx.mtval */
    sd      t4, 176(t0)     /* ctx.mstatus */

    /* Prepare to MRET to clear MDT:
     * - Set mstatus.MPP = M-mode (bits [12:11] = 0b11)
     * - Set mepc to the label after MRET
     * - Execute MRET (this clears MDT and returns to M-mode)
     */

    /* Set MPP = M-mode */
    li      t5, (3 << 11)
    csrw    mstatus, t5

    /* Set mepc to after_mret */
    la      t5, after_mret
    csrw    mepc, t5

    /* Execute MRET to clear MDT */
    mret

after_mret:
    /* Now MDT is cleared, we can safely access CSRs and call C code */

    /* Restore sp from context */
    la      t0, ssdbltrp_hyp_trap_ctx
    ld      sp, 16(t0)

    /* Call the C trap handler: m_trap_handler(ctx)
     * Pass pointer to trap context in a0
     */
    mv      a0, t0
    call    m_trap_handler

    /* Restore trap CSRs from context (handler may have modified them) */
    la      t0, ssdbltrp_hyp_trap_ctx
    ld      t1, 152(t0)     /* ctx.mepc */
    ld      t2, 160(t0)     /* ctx.mcause */
    ld      t3, 168(t0)     /* ctx.mtval */
    ld      t4, 176(t0)     /* ctx.mstatus */

    csrw    mepc, t1
    csrw    mcause, t2
    csrw    mtval, t3

    /* Restore mstatus but preserve MDT=0 (clear MDT bit before MRET) */
    li      t5, ~(1 << 42)  /* Clear MDT bit */
    and     t4, t4, t5
    csrw    mstatus, t4

    /* Restore all caller-saved registers */
    la      t0, ssdbltrp_hyp_trap_ctx
    ld      ra, 0(t0)
    ld      gp, 8(t0)
    ld      tp, 24(t0)
    ld      t1, 40(t0)      /* skip t0, we're using it */
    ld      t2, 48(t0)
    ld      t3, 56(t0)
    ld      t4, 64(t0)
    ld      t5, 72(t0)
    ld      t6, 80(t0)
    ld      a0, 88(t0)
    ld      a1, 96(t0)
    ld      a2, 104(t0)
    ld      a3, 112(t0)
    ld      a4, 120(t0)
    ld      a5, 128(t0)
    ld      a6, 136(t0)
    ld      a7, 144(t0)

    /* Finally restore t0 */
    ld      t0, 32(t0)

    /* Return from trap */
    mret

    .size ssdbltrp_hyp_m_trap_entry, .-ssdbltrp_hyp_m_trap_entry

/* Trap context structure (matches ssdbltrp_hyp_trap_ctx_t in test_helpers.h) */
    .section .bss
    .globl ssdbltrp_hyp_trap_ctx
    .type  ssdbltrp_hyp_trap_ctx, @object
    .balign 8
ssdbltrp_hyp_trap_ctx:
    .space 184              /* 23 uint64_t fields */
    .size  ssdbltrp_hyp_trap_ctx, 184
