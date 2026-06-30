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
    addi sp, sp, -256

    /* Save all caller-saved + temporary registers */
    sd ra,   0(sp)
    sd t0,   8(sp)
    sd t1,  16(sp)
    sd t2,  24(sp)
    sd t3,  32(sp)
    sd t4,  40(sp)
    sd t5,  48(sp)
    sd t6,  56(sp)
    sd a0,  64(sp)
    sd a1,  72(sp)
    sd a2,  80(sp)
    sd a3,  88(sp)
    sd a4,  96(sp)
    sd a5, 104(sp)
    sd a6, 112(sp)
    sd a7, 120(sp)
    sd s0, 128(sp)
    sd s1, 136(sp)
    sd s2, 144(sp)
    sd s3, 152(sp)
    sd s4, 160(sp)
    sd s5, 168(sp)
    sd s6, 176(sp)
    sd s7, 184(sp)
    sd s8, 192(sp)
    sd s9, 200(sp)
    sd s10, 208(sp)
    sd s11, 216(sp)

    /* Call C handler: unsigned hs_trap_handler(void) */
    call hs_trap_handler

    /* a0 = return value (unused for now, reserved for future use) */

    /* Restore registers */
    ld ra,   0(sp)
    ld t0,   8(sp)
    ld t1,  16(sp)
    ld t2,  24(sp)
    ld t3,  32(sp)
    ld t4,  40(sp)
    ld t5,  48(sp)
    ld t6,  56(sp)
    ld a0,  64(sp)
    ld a1,  72(sp)
    ld a2,  80(sp)
    ld a3,  88(sp)
    ld a4,  96(sp)
    ld a5, 104(sp)
    ld a6, 112(sp)
    ld a7, 120(sp)
    ld s0, 128(sp)
    ld s1, 136(sp)
    ld s2, 144(sp)
    ld s3, 152(sp)
    ld s4, 160(sp)
    ld s5, 168(sp)
    ld s6, 176(sp)
    ld s7, 184(sp)
    ld s8, 192(sp)
    ld s9, 200(sp)
    ld s10, 208(sp)
    ld s11, 216(sp)

    addi sp, sp, 256

    sret

#endif /* ENABLE_HYP */
