/*
 * sstvecd_strap.S — Local S-mode trap entry for the Sstvecd test suite.
 *
 * Strategy (see docs/sstvecd_test_plan.md "Design Point 2"):
 *   On any trap (sync exception or async interrupt), the handler
 *     1) records g_sstvecd_trap_pc = address of this entry
 *        (which equals stvec.BASE that the test installed),
 *     2) records g_sstvecd_trap_cause = scause,
 *     3) advances sepc by 4 to skip the offending instruction,
 *     4) clears sip.SSIP if the trap is a SSI interrupt (otherwise the
 *        same SSIP would re-fire immediately after sret),
 *     5) returns via sret.
 *
 *   We do NOT touch common/trap.c. The S-mode test trampoline (in
 *   tests/test_helpers.h) writes stvec to point at sstvecd_trap_entry
 *   on entry and restores the previous stvec on exit.
 *
 * Register usage:
 *   - sscratch holds the address of sstvecd_trap_scratch (16 bytes,
 *     two doublewords) used to spill t0/t1.
 *   - Only t0, t1 are clobbered; restored before sret.
 *
 * PC recording rationale:
 *   Instead of using auipc-based PC self-detection (which is fragile
 *   under instruction reordering), we directly load &sstvecd_trap_entry
 *   into the global. The Sstvecd Direct-mode invariant is:
 *       trap_landing_pc == stvec.BASE
 *   and the test installs stvec.BASE = &sstvecd_trap_entry, so this
 *   identity is exactly what we want to assert.
 */

    .section .text
    .globl  sstvecd_trap_entry
    .align  2                          /* 4 byte aligned -> valid stvec.BASE */
sstvecd_trap_entry:
    /* Spill t0/t1 into the scratch region pointed at by sscratch. */
    csrrw   t0, sscratch, t0           /* t0 <-> sscratch (t0 = scratch ptr) */
    sd      t1,  0(t0)                 /* save user t1 */
    csrr    t1, sscratch               /* t1 = user's original t0 */
    sd      t1,  8(t0)                 /* save user t0 */
    csrw    sscratch, t0               /* restore sscratch -> scratch ptr */

    /* Record trap-landing PC = entry address (== stvec.BASE) */
    la      t1, sstvecd_trap_entry
    la      t0, g_sstvecd_trap_pc
    sd      t1, 0(t0)

    /* Record scause */
    csrr    t1, scause
    la      t0, g_sstvecd_trap_cause
    sd      t1, 0(t0)

    /* If this is an interrupt (scause MSB set), clear sip.SSIP so we
     * don't re-enter the handler immediately after sret. We don't
     * inspect the cause-low further: SSIP is the only interrupt source
     * the Sstvecd test suite enables. */
    csrr    t1, scause
    bgez    t1, .Lnot_int              /* MSB == 0 -> sync exception */
    li      t1, (1 << 1)               /* SSIP bit (supervisor software int) */
    csrc    sip, t1
.Lnot_int:

    /* Advance sepc by 4 to skip the offending instruction.
     * For sync exceptions, this skips the trapping insn (ecall, lw,
     * unimp). For SSIP interrupts, sepc points at the instruction that
     * was about to execute (or just executed) when the interrupt was
     * taken; advancing by 4 skips one nop/csrs and the test program
     * naturally falls through to the next instruction. */
    csrr    t1, sepc
    addi    t1, t1, 4
    csrw    sepc, t1

    /* Restore t0/t1 from the scratch region (sscratch is still ptr). */
    csrr    t0, sscratch
    ld      t1,  0(t0)                 /* restore t1 */
    ld      t0,  8(t0)                 /* restore t0 (must be last) */
    sret


/* ===================================================================
 * Scratch region (16 bytes, naturally aligned) for spilling t0/t1.
 * main.c writes &sstvecd_trap_scratch into sscratch before any S-mode
 * entry that may take a trap; the per-test trampoline re-arms it for
 * safety.
 * =================================================================== */
    .section .bss
    .globl  sstvecd_trap_scratch
    .align  4
sstvecd_trap_scratch:
    .zero   16


/* ===================================================================
 * Test-observable globals: trap-landing PC and scause snapshot.
 * Defined in .data so they live at well-known addresses and are not
 * zero-initialized by the bss clear (the entry overwrites them on
 * every trap; tests reset them via sstvecd_setup_smode_entry()).
 * =================================================================== */
    .section .data
    .globl  g_sstvecd_trap_pc
    .globl  g_sstvecd_trap_cause
    .align  3
g_sstvecd_trap_pc:    .dword 0
g_sstvecd_trap_cause: .dword 0
