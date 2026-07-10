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

    /* Force SPP=1 so sret returns to VS-mode, not VU-mode.
     * When the trap came from VU-mode, hardware set SPP=0.
     * We must return to VS-mode so the framework's exit ecall
     * (cause=10) reaches M-mode correctly. */
    li      t1, 0x100                  /* SSTATUS_SPP = bit 8 */
    csrs    sstatus, t1

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

/* ===================================================================
 * Vectored trap table for VSTVEC-VEC-02.
 *
 * Mechanism:
 *   Direct mode: CPU lands at slot 0 (nop), falls through to slot 1,
 *     which jumps to shvstvecd_trap_entry.
 *   Vectored mode (cause=1): CPU lands at slot 1, which jumps to
 *     shvstvecd_vectored_handler. This sets a marker global, then
 *     jumps to shvstvecd_trap_entry for normal processing.
 *
 *   The test checks g_shvstvecd_vec_marker:
 *     == 0: Direct mode (vectored handler not reached)
 *     != 0: Vectored mode (vectored handler executed)
 *
 * Layout (each slot is 4 bytes = 1 instruction):
 *   [0] BASE+0:   nop  (Direct landing)
 *   [1] BASE+4:   j shvstvecd_vectored_handler (Vectored cause=1)
 *   [2] BASE+8:   j shvstvecd_trap_entry (Vectored cause=5)
 *   [3] BASE+12:  j shvstvecd_trap_entry (Vectored cause=9)
 *   ...
 * =================================================================== */
    .section .text
    .globl  shvstvecd_vectored_table
    .align  4                          /* 16-byte aligned */
    .option norvc                      /* Disable compressed insns:
                                          each slot must be exactly 4 bytes */
shvstvecd_vectored_table:
    /* Slot 0 (BASE+0): Direct mode landing.
     * Jump directly to shvstvecd_trap_entry, SKIPPING the marker. */
    j       shvstvecd_trap_entry

    /* Slot 1 (BASE+4): Vectored cause=1 (SSI) landing.
     * Jumps to the vectored handler which sets a marker. */
    j       shvstvecd_vectored_handler

    /* Slot 2 (BASE+8): Vectored cause=5 (STI) */
    j       shvstvecd_trap_entry

    /* Slot 3 (BASE+12): Vectored cause=9 (SEI) */
    j       shvstvecd_trap_entry

    /* Slots 4-15: unused causes, point to handler for safety */
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    j       shvstvecd_trap_entry
    .option rvc                        /* Re-enable compressed insns */

/* ===================================================================
 * Vectored-specific handler for VEC-02.
 * Records a non-zero marker to g_shvstvecd_vec_marker, then jumps
 * to the common trap entry for normal processing.
 * =================================================================== */
    .globl  shvstvecd_vectored_handler
    .align  2
shvstvecd_vectored_handler:
    li      t1, 1
    la      t0, g_shvstvecd_vec_marker
    sd      t1, 0(t0)
    j       shvstvecd_trap_entry
    .option rvc                        /* Ensure compressed insns enabled */

/* ===================================================================
 * Vectored marker global.
 * Set to 1 by shvstvecd_vectored_handler in Vectored mode.
 * Stays 0 in Direct mode (handler not reached).
 * =================================================================== */
    .section .data
    .globl  g_shvstvecd_vec_marker
    .align  3
g_shvstvecd_vec_marker: .dword 0
