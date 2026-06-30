/*
 * sstc_strap.S — Local S-mode trap entry for the Sstc test suite.
 *
 * On any trap the handler:
 *   1) records g_sstc_trap_cause = scause,
 *   2) if it is an interrupt (scause MSB set), writes stimecmp to
 *      max value (0x14D) to clear the timer interrupt source,
 *   3) advances sepc by 4,
 *   4) returns via sret.
 *
 * Register usage:
 *   - sscratch holds the address of sstc_trap_scratch (16 bytes)
 *     used to spill t0/t1.
 *   - Only t0, t1 are clobbered; restored before sret.
 */

    .section .text
    .globl  sstc_trap_entry
    .align  2                          /* 4 byte aligned -> valid stvec.BASE */
sstc_trap_entry:
    /* Spill t0/t1 into the scratch region pointed at by sscratch. */
    csrrw   t0, sscratch, t0           /* t0 <-> sscratch (t0 = scratch ptr) */
    sd      t1,  0(t0)                 /* save user t1 */
    csrr    t1, sscratch               /* t1 = user's original t0 */
    sd      t1,  8(t0)                 /* save user t0 */
    csrw    sscratch, t0               /* restore sscratch -> scratch ptr */

    /* Record scause */
    csrr    t1, scause
    la      t0, g_sstc_trap_cause
    sd      t1, 0(t0)

    /* If this is an interrupt (scause MSB set), clear timer interrupt
     * by writing stimecmp (CSR 0x14D) to max value. */
    csrr    t1, scause
    bgez    t1, .Lsstc_not_int        /* MSB == 0 -> sync exception */
    li      t1, -1
    csrw    0x14D, t1                  /* stimecmp = MAX */
.Lsstc_not_int:

    /* Advance sepc by 4 to skip the trapping / interrupted instruction. */
    csrr    t1, sepc
    addi    t1, t1, 4
    csrw    sepc, t1

    /* Restore t0/t1 from the scratch region (sscratch is still ptr). */
    csrr    t0, sscratch
    ld      t1,  0(t0)                 /* restore t1 */
    ld      t0,  8(t0)                 /* restore t0 (must be last) */
    sret


/* ===================================================================
 * Scratch region (16 bytes) for spilling t0/t1.
 * =================================================================== */
    .section .bss
    .globl  sstc_trap_scratch
    .align  4
sstc_trap_scratch:
    .zero   16


/* ===================================================================
 * Globals: scause snapshot.
 * =================================================================== */
    .section .data
    .globl  g_sstc_trap_cause
    .align  3
g_sstc_trap_cause: .dword 0
