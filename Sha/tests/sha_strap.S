/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * sha_strap.S -- VS-mode Direct trap entry for the Sha test suite.
 *
 * When a trap is delegated to VS-mode (via hedeleg), hardware writes
 * vscause, vsepc, vstval and jumps to vstvec.
 *
 * This handler:
 *   1) Records g_sha_vs_cause  = vscause
 *   2) Records g_sha_vs_tval   = vstval
 *   3) For instruction-fetch faults (cause 1, 12), illegal instruction
 *      (cause 2), and breakpoint (cause 3):
 *        sets sepc = ra (return address from the jalr that jumped here)
 *      For all other faults:
 *        advances sepc by the instruction width (2 or 4 bytes)
 *   4) Returns via sret
 *
 * Stack-free design: uses sscratch for temporary register spill so
 * no VS-mode stack is required.
 */

    .section .text
    .globl  sha_vs_trap_entry
    .align  2                          /* 4-byte aligned -> valid vstvec.BASE */
sha_vs_trap_entry:
    /* ---- Spill t0/t1 via sscratch (V=1: actually vsscratch) ---- */
    csrrw   t0, sscratch, t0           /* t0 <-> sscratch (t0 = scratch ptr) */
    sd      t1,  0(t0)                 /* save user t1 */
    csrr    t1, sscratch               /* t1 = user's original t0 */
    sd      t1,  8(t0)                 /* save user t0 */
    csrw    sscratch, t0               /* restore sscratch -> scratch ptr */

    /* ---- Record stval (V=1: actually vstval) ---- */
    csrr    t1, stval
    la      t0, g_sha_vs_tval
    sd      t1, 0(t0)

    /* ---- Record scause (V=1: actually vscause) ---- */
    csrr    t1, scause
    la      t0, g_sha_vs_cause
    sd      t1, 0(t0)

    /* ---- Determine return address based on cause ----
     *
     * For instruction-fetch faults (cause=1 inst access-fault,
     * cause=12 inst page-fault), illegal instruction (cause=2), and
     * breakpoint (cause=3), sepc points to an inaccessible or special
     * address. Using sepc+4 would re-trigger the same fault. Instead,
     * use ra which holds the return address from the jalr/call that
     * jumped to the faulting target.
     *
     * For load/store faults (misaligned, page-fault, access-fault),
     * advance sepc past the faulting instruction.
     */
    li      t0, 1
    beq     t1, t0, .Lsha_use_ra       /* cause == 1 (inst access-fault) */
    li      t0, 2
    beq     t1, t0, .Lsha_use_ra       /* cause == 2 (illegal instruction) */
    li      t0, 3
    beq     t1, t0, .Lsha_use_ra       /* cause == 3 (breakpoint) */
    li      t0, 12
    beq     t1, t0, .Lsha_use_ra       /* cause == 12 (inst page-fault) */

    /* Default: advance sepc by instruction width (2 or 4 bytes).
     * Load the faulting instruction's low halfword and check bits[1:0].
     * Compressed instructions have bits[1:0] != 0b11. */
    csrr    t1, sepc
    lhu     t0, 0(t1)                 /* load low 16 bits of instruction */
    andi    t0, t0, 3                 /* extract bits [1:0] */
    li      t1, 3
    bne     t0, t1, .Lsha_compressed  /* bits[1:0] != 3 -> compressed (2B) */

    /* Standard 4-byte instruction */
    csrr    t1, sepc
    addi    t1, t1, 4
    csrw    sepc, t1
    j       .Lsha_restore

.Lsha_compressed:
    /* Compressed 2-byte instruction */
    csrr    t1, sepc
    addi    t1, t1, 2
    csrw    sepc, t1
    j       .Lsha_restore

.Lsha_use_ra:
    /* Use ra as return address for instruction-fetch faults */
    csrw    sepc, ra

.Lsha_restore:
    /* Restore t0/t1 from scratch region. */
    csrr    t0, sscratch
    ld      t1,  0(t0)                 /* restore t1 */
    ld      t0,  8(t0)                 /* restore t0 (must be last) */
    sret


/* ===================================================================
 * Scratch region (16 bytes) for spilling t0/t1.
 * The test sets vsscratch to point here before entering VS-mode.
 * =================================================================== */
    .section .bss
    .globl  sha_vs_trap_scratch
    .align  4
sha_vs_trap_scratch:
    .zero   16


/* ===================================================================
 * Test-observable globals: vscause and vstval captured by the handler.
 * =================================================================== */
    .section .data
    .globl  g_sha_vs_cause
    .globl  g_sha_vs_tval
    .align  3
g_sha_vs_cause: .dword 0
g_sha_vs_tval:  .dword 0


/* ===================================================================
 * Ecall anchor for GPF recovery page.
 *
 * This tiny function contains only an ecall instruction. It is placed
 * in the .text section (real RAM) so that the G-stage can map a
 * recovery page's SPA to this address. When VS-mode execution reaches
 * the recovery page, the G-stage translates the fetch to this kernel
 * code, executing the ecall which cleanly returns to M-mode.
 * =================================================================== */
    .section .text
    .globl  sha_ecall_anchor
    .align  2
sha_ecall_anchor:
    /* Restore a0/a1 for the trampoline ecall protocol.
     * a0 = ECALL_GOTO_PRIV (1)
     * a1 = PRIV_M (3)
     * These may have been clobbered by the HS trap handler during
     * the ~2048 iterations of the GPF loop. */
    li      a0, 1          /* ECALL_GOTO_PRIV */
    li      a1, 3          /* PRIV_M */
    ecall
