/*
 * shvstvala_strap.S — VS-mode trap entry for the Shvstvala test suite.
 *
 * When a trap is delegated to VS-mode (via hedeleg), the hardware writes
 * vscause, vsepc, vstval and jumps to vstvec. Since V=1, the S-mode CSR
 * names (scause, sepc, stval, sscratch, sret) access their VS counterparts.
 *
 * This handler:
 *   1) Records g_shvstvala_vstval = stval (actually vstval)
 *   2) Records g_shvstvala_cause  = scause (actually vscause)
 *   3) For instruction-fetch faults (cause 1, 12), illegal instruction
 *        (cause 2), and breakpoint (cause 3):
 *        sets sepc = ra (return address from the jalr that jumped to target)
 *      For all other faults:
 *        advances sepc by 4
 *   4) Returns via sret
 *
 * Rationale: For cause=1 (instruction access-fault) and cause=12
 * (instruction page-fault), sepc is the faulting fetch address (e.g.,
 * a non-executable page). Simply adding 4 would still be in the same
 * faulting page. Instead, we use ra which holds the caller's return
 * address (the instruction after the jalr/call that triggered the fetch).
 */

    .section .text
    .globl  shvstvala_trap_entry
    .align  2                          /* 4-byte aligned -> valid vstvec.BASE */
shvstvala_trap_entry:
    /* Spill t0/t1 via sscratch (V=1: actually vsscratch). */
    csrrw   t0, sscratch, t0           /* t0 <-> sscratch (t0 = scratch ptr) */
    sd      t1,  0(t0)                 /* save user t1 */
    csrr    t1, sscratch               /* t1 = user's original t0 */
    sd      t1,  8(t0)                 /* save user t0 */
    csrw    sscratch, t0               /* restore sscratch -> scratch ptr */

    /* Record stval (V=1: actually vstval) */
    csrr    t1, stval
    la      t0, g_shvstvala_vstval
    sd      t1, 0(t0)

    /* Record scause (V=1: actually vscause) */
    csrr    t1, scause
    la      t0, g_shvstvala_cause
    sd      t1, 0(t0)

    /* Determine return address based on cause:
     * - cause=1 (instruction access-fault): sepc = ra
     * - cause=12 (instruction page-fault): sepc = ra
     * - cause=2 (illegal instruction): sepc = ra
     * - cause=3 (breakpoint from trigger): sepc = ra
     * - all others: sepc += instruction_width (2 or 4 bytes)
     *
     * For fetch faults (1, 12), sepc points to the inaccessible target.
     * For illegal instruction (2) and breakpoint (3), sepc points to
     * the faulting instruction in a data/trigger region; advancing by 4
     * would execute the next data word. ra holds the return address from
     * the jalr/call that jumped to the faulting target.
     *
     * For other exceptions (misaligned, page-fault, access-fault on
     * load/store), we need to advance sepc by the instruction width.
     * RISC-V compressed instructions (C extension) are 2 bytes, while
     * standard instructions are 4 bytes. We detect this by checking the
     * low 2 bits of the faulting instruction at sepc. */
    li      t0, 1
    beq     t1, t0, .Luse_ra          /* cause == 1 (inst access-fault) */
    li      t0, 2
    beq     t1, t0, .Luse_ra          /* cause == 2 (illegal instruction) */
    li      t0, 3
    beq     t1, t0, .Luse_ra          /* cause == 3 (breakpoint) */
    li      t0, 12
    beq     t1, t0, .Luse_ra          /* cause == 12 (inst page-fault) */

    /* Default: advance sepc by instruction width (2 or 4 bytes).
     * Load the faulting instruction and check if it's compressed.
     * Compressed instructions have bits [1:0] != 0b11. */
    csrr    t1, sepc
    lhu     t0, 0(t1)                 /* load low 16 bits of instruction */
    andi    t0, t0, 3                 /* extract bits [1:0] */
    li      t1, 3
    bne     t0, t1, .Lcompressed      /* if bits[1:0] != 3, it's compressed (2 bytes) */

    /* Standard 4-byte instruction */
    csrr    t1, sepc
    addi    t1, t1, 4
    csrw    sepc, t1
    j       .Lrestore

.Lcompressed:
    /* Compressed 2-byte instruction */
    csrr    t1, sepc
    addi    t1, t1, 2
    csrw    sepc, t1
    j       .Lrestore

.Luse_ra:
    /* Use ra as return address for instruction-fetch faults */
    csrw    sepc, ra

.Lrestore:
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
    .globl  shvstvala_trap_scratch
    .align  4
shvstvala_trap_scratch:
    .zero   16


/* ===================================================================
 * Test-observable globals: vstval and vscause captured by the handler.
 * =================================================================== */
    .section .data
    .globl  g_shvstvala_vstval
    .globl  g_shvstvala_cause
    .align  3
g_shvstvala_vstval: .dword 0
g_shvstvala_cause:  .dword 0
