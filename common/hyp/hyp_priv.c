/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/hyp_priv.c — VS / VU mode execution helpers
 *
 * Implementation strategy:
 *   - Reuse the existing run_in_priv() trampoline pattern from
 *     common/privilege.c, but enter the trampoline with V=1 by
 *     setting mstatus.MPV (RV64 bit 39) before mret.
 *   - The trampoline ends with `ecall`, which (in VS-mode) reports
 *     CAUSE_ECALL_FROM_VS=10. The M-mode ecall handler in trap.c
 *     accepts cause 10 as a privilege-switch ecall and returns to
 *     M-mode normally (clearing MPV).
 *
 * NOTE: We must enter VS-mode from M-mode directly (not via S-mode),
 *       because mstatus.MPV is only consulted by mret. Therefore
 *       these helpers internally hop to M-mode first if necessary.
 * =================================================================== */

#include "hyp_priv.h"
#include "encoding.h"
#include "uart.h"
#include "hyp/hyp_csr.h"

extern unsigned current_priv;
extern uintptr_t ecall_args[2];
#define ECALL_GOTO_PRIV  1

/* ===================================================================
 * Trampoline state
 * =================================================================== */
static uintptr_t (*_v_run_fn)(uintptr_t);
static uintptr_t   _v_run_arg;
static uintptr_t   _v_run_result;

/* The trampoline runs in VS- or VU-mode. It calls fn(arg) and then
 * issues `ecall` to return to M-mode. The trap handler sets mepc to
 * the instruction *after* the ecall (the `ret`), and configures
 * MPP back to M-mode and clears MPV, so mret returns here in M-mode.
 * Then `ret` jumps to ra, which we set to a label in the caller. */
static void __attribute__((used)) _v_trampoline(void) {
    _v_run_result = _v_run_fn(_v_run_arg);
    ecall_args[0] = ECALL_GOTO_PRIV;
    ecall_args[1] = PRIV_M;
    asm volatile ("ecall" ::: "memory");
}

/* ===================================================================
 * Common implementation
 *
 * @target: PRIV_VS (5) or PRIV_VU (4) — bit 2 marks V=1, low 2 bits
 *          give nominal MPP (S=1 / U=0).
 * =================================================================== */
static uintptr_t run_in_virt_priv(unsigned target,
                                  uintptr_t (*fn)(uintptr_t),
                                  uintptr_t arg)
{
    /* Make sure we are in M-mode so that mret with MPV=1 takes effect. */
    extern void goto_priv(unsigned);
    if (current_priv != PRIV_M)
        goto_priv(PRIV_M);

    _v_run_fn     = fn;
    _v_run_arg    = arg;
    _v_run_result = 0;

    /* Configure mstatus:
     *   - clear MPP, set MPP to (target & 3)
     *   - set MPV (bit 39) so mret enters V=1
     *   - clear MPRV (bit 17)
     *   - set MPIE so interrupts behave normally after mret
     */
    uintptr_t ms = CSRR(mstatus);
    ms &= ~((3UL << 11) | (1UL << 17) | MSTATUS_MPV);
    ms |=  ((uintptr_t)(target & 3) << 11);
    ms |=  MSTATUS_MPV;
    CSRW(mstatus, ms);

    /* Update tracked privilege so subsequent ecall returns set MPV/MPP correctly. */
    current_priv = target;

    /* Ensure the trampoline's ecall (cause=10, ecall-from-VS) always
     * traps to M-mode. If medeleg[10]=1 or hedeleg[10]=1, the hardware
     * would delegate the ecall to S/VS-mode instead, breaking the
     * return path. Clear both bits before mret. */
    uintptr_t md = CSRR(medeleg);
    md &= ~(1UL << 10);
    CSRW(medeleg, md);

    uintptr_t hd = hedeleg_read();
    hd &= ~(1UL << 10);
    hedeleg_write(hd);

    /* Enter VS/VU at _v_trampoline, return here via the ecall round-trip. */
    asm volatile (
        "la ra, 1f\n\t"
        "la t0, _v_trampoline\n\t"
        "csrw mepc, t0\n\t"
        "mret\n\t"
        "1:\n\t"
        ::: "ra", "t0", "memory"
    );

    /* On return current_priv has been reset to PRIV_M by the ecall handler. */
    return _v_run_result;
}

uintptr_t run_in_vs_mode(uintptr_t (*fn)(uintptr_t), uintptr_t arg) {
    return run_in_virt_priv(PRIV_VS, fn, arg);
}

uintptr_t run_in_vu_mode(uintptr_t (*fn)(uintptr_t), uintptr_t arg) {
    return run_in_virt_priv(PRIV_VU, fn, arg);
}

/* ===================================================================
 * Direct privilege-level switching
 *
 * These are lower-level than run_in_vs/vu_mode — they switch
 * privilege but do NOT set up a return trampoline. The caller must
 * arrange the return path (e.g., ecall → return_to_hs_mode).
 *
 * NOTE: goto_vs_mode works via M-mode mret (setting MPV=1) since
 * the test framework always traps to M-mode. It sets mepc to the
 * return address of the caller so execution continues there in
 * VS-mode.
 * =================================================================== */

void goto_vs_mode(void) {
    /* Ensure we are in M-mode first */
    extern void goto_priv(unsigned);
    if (current_priv != PRIV_M)
        goto_priv(PRIV_M);

    /* Set mstatus: MPP=S(1), MPV=1, clear MPRV */
    uintptr_t ms = CSRR(mstatus);
    ms &= ~((3UL << 11) | (1UL << 17) | MSTATUS_MPV);
    ms |= (1UL << 11);     /* MPP = S */
    ms |= MSTATUS_MPV;     /* V=1 on mret */
    CSRW(mstatus, ms);

    current_priv = PRIV_VS;

    /* mret to the instruction after this function returns.
     * We set mepc = ra (return address of caller). */
    asm volatile (
        "csrw mepc, ra\n\t"
        "mret\n\t"
        ::: "memory"
    );
}

void goto_vu_mode(void) {
    /* Ensure we are in M-mode first */
    extern void goto_priv(unsigned);
    if (current_priv != PRIV_M)
        goto_priv(PRIV_M);

    /* Set mstatus: MPP=U(0), MPV=1, clear MPRV */
    uintptr_t ms = CSRR(mstatus);
    ms &= ~((3UL << 11) | (1UL << 17) | MSTATUS_MPV);
    /* MPP = U (bits [12:11] = 0) — already cleared */
    ms |= MSTATUS_MPV;     /* V=1 on mret */
    CSRW(mstatus, ms);

    current_priv = PRIV_VU;

    asm volatile (
        "csrw mepc, ra\n\t"
        "mret\n\t"
        ::: "memory"
    );
}

void return_to_hs_mode(void) {
    /* Issue ecall from VS/VU-mode. The M-mode handler recognizes
     * CAUSE_ECALL_FROM_VS (10) as a privilege-switch ecall. */
    ecall_args[0] = ECALL_GOTO_PRIV;
    ecall_args[1] = PRIV_M;
    asm volatile ("ecall" ::: "memory");
}

unsigned get_virt_priv(void) {
    return current_priv;
}
