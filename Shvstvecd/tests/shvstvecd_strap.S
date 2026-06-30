/*
 * shvstvecd_strap.S — VS-mode trap entry for the Shvstvecd test suite.
 *
 * Strategy (see DOCS/testplan/shvstvecd_test_plan.md "Design Point 2"):
 *   When V=1, the S-mode CSR names (scause, sepc, sscratch, sret) all
 *   access their VS counterparts (vscause, vsepc, vsscratch). The
 *   handler:
 *     1) Records g_shvstvecd_trap_pc = address of this entry
 *        (which equals the vstvec.BASE that the test installed),
 *     2) Records g_shvstvecd_trap_cause = scause (actually vscause),
 *     3) Advances sepc (actually vsepc) by 4,
 *     4) Clears sip.SSIP if the trap is an interrupt (prevents re-fire),
 *     5) Returns via sret.
 *
 * Register usage:
 *   - sscratch (vsscratch when V=1) holds scratch area for spilling t0/t1.
 *   - Only t0, t1 are clobbered; restored before sret.
 */

    .section .text
    .globl  shvstvecd_trap_entry
    .align  2                          /* 4 byte aligned -> valid vstvec.BASE */
shvstvecd_trap_entry:
    /* Spill t0/t1 via sscratch (V=1: actually vsscratch). */
    csrrw   t0, sscratch, t0           /* t0 <-> sscratch (t0 = scratch ptr) */
    sd      t1,  0(t0)                 /* save user t1 */
    csrr    t1, sscratch               /* t1 = user's original t0 */
    sd      t1,  8(t0)                 /* save user t0 */
    csrw    sscratch, t0               /* restore sscratch -> scratch ptr */

    /* Record trap-landing PC = entry address (== vstvec.BASE) */
    la      t1, shvstvecd_trap_entry
    la      t0, g_shvstvecd_trap_pc
    sd      t1, 0(t0)

    /* Record scause (V=1: actually vscause) */
    csrr    t1, scause
    la      t0, g_shvstvecd_trap_cause
    sd      t1, 0(t0)

    /* If interrupt (scause MSB set), clear sip.SSIP to prevent re-fire. */
    csrr    t1, scause
    bgez    t1, .Lnot_int              /* MSB == 0 -> sync exception */
    li      t1, (1 << 1)               /* SSIP bit */
    csrc    sip, t1
.Lnot_int:

    /* Advance sepc (V=1: actually vsepc) by 4. */
    csrr    t1, sepc
    addi    t1, t1, 4
    csrw    sepc, t1

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
    .globl  shvstvecd_trap_scratch
    .align  4
shvstvecd_trap_scratch:
    .zero   16


/* ===================================================================
 * Test-observable globals: trap-landing PC and vscause snapshot.
 * =================================================================== */
    .section .data
    .globl  g_shvstvecd_trap_pc
    .globl  g_shvstvecd_trap_cause
    .align  3
g_shvstvecd_trap_pc:    .dword 0
g_shvstvecd_trap_cause: .dword 0
