/* ===================================================================
 * common/hyp/hyp_trap_asm.S — HS-mode trap vector entry point
 *
 * Provides _hs_trap_entry for use as the stvec value when testing
 * hedeleg-delegated traps (traps from VS/VU-mode delivered to
 * HS-mode via stvec instead of mtvec).
 *
 * Register save/restore follows the same convention as the existing
 * M-mode trap_asm.S: save all caller-saved registers + ra/sp on the
 * stack, call the C handler, then restore and sret.
 *
 * NOTE: This is only needed when tests explicitly set up hedeleg to
 * delegate certain exceptions to VS-mode (which first traps to
 * HS-mode via stvec). For the majority of G-stage tests that keep
 * hedeleg=0, all traps go directly to M-mode and this code is not
 * invoked.
 * =================================================================== */

#ifdef ENABLE_HYP

#if __riscv_xlen == 64
    #define LOAD    ld
    #define STORE   sd
    #define REGSIZE 8
#else
    #define LOAD    lw
    #define STORE   sw
    #define REGSIZE 4
#endif

#define HS_CONTEXT_SIZE (REGSIZE * 23)

.section .text
.global _hs_trap_entry
.align 4

_hs_trap_entry:
    /* Save context on the current stack (sscratch should hold
     * HS-mode stack pointer if entered from VS/VU-mode, or we
     * are already on HS-mode stack if trap from HS-mode itself). */

    /* Swap sp with sscratch if coming from V=1 (SPV=1 in hstatus).
     * For simplicity in this test framework, we assume the stack is
     * already valid (tests run with a shared flat address space). */
    addi sp, sp, -HS_CONTEXT_SIZE

    /* Save s0-s6: VS-mode test code uses these; s7-s11 are unused by
     * VS test payloads AND preserved by hs_trap_handler (C callee-saved). */
    STORE ra,   0*REGSIZE(sp)
    STORE t0,   1*REGSIZE(sp)
    STORE t1,   2*REGSIZE(sp)
    STORE t2,   3*REGSIZE(sp)
    STORE t3,   4*REGSIZE(sp)
    STORE t4,   5*REGSIZE(sp)
    STORE t5,   6*REGSIZE(sp)
    STORE t6,   7*REGSIZE(sp)
    STORE a0,   8*REGSIZE(sp)
    STORE a1,   9*REGSIZE(sp)
    STORE a2,  10*REGSIZE(sp)
    STORE a3,  11*REGSIZE(sp)
    STORE a4,  12*REGSIZE(sp)
    STORE a5,  13*REGSIZE(sp)
    STORE a6,  14*REGSIZE(sp)
    STORE a7,  15*REGSIZE(sp)
    STORE s0,  16*REGSIZE(sp)
    STORE s1,  17*REGSIZE(sp)
    STORE s2,  18*REGSIZE(sp)
    STORE s3,  19*REGSIZE(sp)
    STORE s4,  20*REGSIZE(sp)
    STORE s5,  21*REGSIZE(sp)
    STORE s6,  22*REGSIZE(sp)

    /* Call C handler: unsigned hs_trap_handler(void) */
    call hs_trap_handler

    /* a0 = return value (unused for now, reserved for future use) */

    /* Restore registers */
    LOAD ra,   0*REGSIZE(sp)
    LOAD t0,   1*REGSIZE(sp)
    LOAD t1,   2*REGSIZE(sp)
    LOAD t2,   3*REGSIZE(sp)
    LOAD t3,   4*REGSIZE(sp)
    LOAD t4,   5*REGSIZE(sp)
    LOAD t5,   6*REGSIZE(sp)
    LOAD t6,   7*REGSIZE(sp)
    LOAD a0,   8*REGSIZE(sp)
    LOAD a1,   9*REGSIZE(sp)
    LOAD a2,  10*REGSIZE(sp)
    LOAD a3,  11*REGSIZE(sp)
    LOAD a4,  12*REGSIZE(sp)
    LOAD a5,  13*REGSIZE(sp)
    LOAD a6,  14*REGSIZE(sp)
    LOAD a7,  15*REGSIZE(sp)
    LOAD s0,  16*REGSIZE(sp)
    LOAD s1,  17*REGSIZE(sp)
    LOAD s2,  18*REGSIZE(sp)
    LOAD s3,  19*REGSIZE(sp)
    LOAD s4,  20*REGSIZE(sp)
    LOAD s5,  21*REGSIZE(sp)
    LOAD s6,  22*REGSIZE(sp)

    addi sp, sp, HS_CONTEXT_SIZE

    sret

#endif /* ENABLE_HYP */
