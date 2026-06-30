/*
 * Trap entry points (assembly layer)
 *
 * Saves/restores full register context, calls C handlers,
 * and returns via mret or sret based on handler return value.
 */

#if __riscv_xlen == 64
    #define LOAD    ld
    #define STORE   sd
    #define REGSIZE 8
#else
    #define LOAD    lw
    #define STORE   sw
    #define REGSIZE 4
#endif

#define CONTEXT_SIZE (REGSIZE * 31)

/* ===== Save all general-purpose registers (x1-x31) ===== */
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

/* ===== Restore all general-purpose registers (x1-x31) ===== */
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

/* =================================================================
 * M-mode trap entry
 *
 * Calls: unsigned m_trap_handler(void)
 * Return value in a0:
 *   PRIV_M (3) -> mret
 *   PRIV_S (1) -> sret (delegated)
 *   PRIV_U (0) -> sret (delegated)
 * ================================================================= */
    .balign 16
    .global m_trap_entry
m_trap_entry:
    SAVE_CONTEXT
    jal     m_trap_handler
    j       _trap_return

/* =================================================================
 * S-mode trap entry
 *
 * Calls: unsigned s_trap_handler(void)
 * Return value in a0 is always PRIV_S or PRIV_U -> sret
 * ================================================================= */
    .balign 16
    .global s_trap_entry
s_trap_entry:
    SAVE_CONTEXT
    jal     s_trap_handler
    j       _trap_return

/* =================================================================
 * Common return path
 *
 * a0 = privilege level to return to
 *   3 (PRIV_M) -> mret
 *   otherwise  -> sret
 * ================================================================= */
_trap_return:
    li      t0, 3
    beq     a0, t0, _do_mret

_do_sret:
    RESTORE_CONTEXT
    sret

_do_mret:
    RESTORE_CONTEXT
    mret
