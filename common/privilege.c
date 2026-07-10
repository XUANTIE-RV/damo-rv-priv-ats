/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "types.h"
#include "encoding.h"
#include "uart.h"

/* ===================================================================
 * Privilege mode definitions
 * =================================================================== */
#define PRIV_U  0
#define PRIV_S  1
#define PRIV_M  3

#ifdef ENABLE_HYP
/* Virtualized privilege levels (V=1).
 * Bit 2 marks V=1; low 2 bits hold the nominal privilege.
 * Must match definitions in common/test_framework.h. */
#define PRIV_VU 4   /* V=1, nominal U-mode */
#define PRIV_VS 5   /* V=1, nominal S-mode */

/* Check if a privilege target requires V=1 (virtualized mode) */
static inline bool is_virt_target(unsigned target) {
    return (target & 0x4) != 0;
}
#endif

/* Current privilege level tracking */
unsigned current_priv = PRIV_M;

/* Ecall arguments (shared with trap.c) */
extern uintptr_t ecall_args[2];
#define ECALL_GOTO_PRIV  1

/* ===================================================================
 * get_current_priv - return current tracked privilege level
 * =================================================================== */
unsigned get_current_priv(void) {
    return current_priv;
}

/* ===================================================================
 * set_prev_priv - configure mstatus.MPP / sstatus.SPP for mret/sret
 *
 * Called before mret/sret to set the target privilege level.
 * =================================================================== */
static void set_prev_priv(unsigned target) {
    switch (current_priv) {
    case PRIV_M: {
        uintptr_t mstatus = CSRR(mstatus);
        /* Clear MPP field [12:11] and MPV (bit 39, RV64 only) */
#if __riscv_xlen > 32
        mstatus &= ~(MSTATUS_MPP_MASK | MSTATUS_MPV);
#else
        mstatus &= ~MSTATUS_MPP_MASK;
#endif
        /* Set MPP to nominal privilege (low 2 bits) */
        mstatus |= ((uintptr_t)(target & 3) << MSTATUS_MPP_OFF);
#ifdef ENABLE_HYP
        /* Set MPV=1 for virtualized targets (VS/VU) */
        if (is_virt_target(target))
            mstatus |= MSTATUS_MPV;
#endif
        CSRW(mstatus, mstatus);
        break;
    }
#ifdef ENABLE_HYP
    case PRIV_VS:
#endif
    case PRIV_S: {
#ifdef ENABLE_HYP
        if (is_virt_target(target)) {
            printf("ERROR: set_prev_priv from S/VS-mode for virtualized target %d\n",
                   target);
            break;
        }
#endif
        uintptr_t sstatus = CSRR(sstatus);
        /* Clear SPP bit [8] */
        sstatus &= ~MSTATUS_SPP_BIT;
        /* SPP=1 for S-mode, SPP=0 for U-mode */
        if (target == PRIV_S)
            sstatus |= MSTATUS_SPP_BIT;
        CSRW(sstatus, sstatus);
        break;
    }
    default:
        printf("ERROR: set_prev_priv from unsupported priv %d\n", current_priv);
        break;
    }
}

/* Forward declaration: do_ecall is used by lower_priv (ENABLE_HYP path) */
static void do_ecall(uintptr_t a0, uintptr_t a1);

#ifdef ENABLE_HYP
/* Forward declaration: priv_cmp is used by lower_priv and goto_priv */
static int priv_cmp(unsigned a, unsigned b);
#endif

/* ===================================================================
 * lower_priv - switch to a lower privilege level via mret/sret
 * =================================================================== */
static void lower_priv(unsigned target) {
    if (target == current_priv)
        return;

#ifdef ENABLE_HYP
    /* Virtualized targets require M-mode mret (only mret consults MPV).
     * If not in M-mode, ecall up to M-mode first, then lower from there. */
    if (is_virt_target(target) && current_priv != PRIV_M) {
        do_ecall(ECALL_GOTO_PRIV, PRIV_M);
        /* Now in M-mode, fall through to mret path below */
    }

    /* Check if we're actually going down in privilege using proper comparison */
    if (priv_cmp(target, current_priv) > 0) {
        printf("ERROR: lower_priv called with higher target %d > %d\n",
               target, current_priv);
        return;
    }
#else
    if (target > current_priv) {
        printf("ERROR: lower_priv called with higher target %d > %d\n",
               target, current_priv);
        return;
    }
#endif

    set_prev_priv(target);

    if (current_priv == PRIV_M) {
        current_priv = target;
        asm volatile(
            "la t0, 1f\n\t"
            "csrw mepc, t0\n\t"
            "mret\n\t"
            "1:\n\t"
            ::: "t0", "memory"
        );
    } else if (current_priv == PRIV_S
#ifdef ENABLE_HYP
               || current_priv == PRIV_VS
#endif
              ) {
        current_priv = target;
        asm volatile(
            "la t0, 1f\n\t"
            "csrw sepc, t0\n\t"
            "sret\n\t"
            "1:\n\t"
            ::: "t0", "memory"
        );
    }
}

/* ===================================================================
 * ecall - trigger ecall with arguments
 * =================================================================== */
static void do_ecall(uintptr_t a0, uintptr_t a1) {
    ecall_args[0] = a0;
    ecall_args[1] = a1;
    asm volatile("ecall" ::: "memory");
}

#ifdef ENABLE_HYP
/* ===================================================================
 * priv_cmp - compare privilege levels correctly for virtualized modes
 *
 * Returns:
 *   >0 if a is higher privilege than b
 *   <0 if a is lower privilege than b
 *    0 if equal
 *
 * Encoding: PRIV_U=0, PRIV_S=1, PRIV_M=3, PRIV_VU=4(V=1), PRIV_VS=5(V=1)
 * Actual hierarchy: M > HS/S > U, and M > VS > VU
 * =================================================================== */
static int priv_cmp(unsigned a, unsigned b) {
    bool a_virt = is_virt_target(a);
    bool b_virt = is_virt_target(b);

    if (a == b) return 0;

    if (!a_virt && !b_virt) {
        /* Both non-virtualized: simple numeric comparison */
        return (int)a - (int)b;
    } else if (a_virt && !b_virt) {
        /* a is virtualized, b is not:
         * VS/VU are always lower than M and HS/S */
        return -1;
    } else if (!a_virt && b_virt) {
        /* b is virtualized, a is not:
         * M and HS/S are always higher than VS/VU */
        return 1;
    } else {
        /* Both virtualized: compare nominal privilege (low 2 bits) */
        return (int)(a & 3) - (int)(b & 3);
    }
}
#endif /* ENABLE_HYP */

/* ===================================================================
 * goto_priv - switch to any privilege level (bidirectional)
 *
 * M -> S: via mret
 * M -> U: via mret
 * S -> U: via sret
 * U -> S: via ecall -> S-mode handler
 * U -> M: via ecall -> M-mode handler
 * S -> M: via ecall -> M-mode handler
 *
 * ENABLE_HYP virtualized paths:
 * M -> VS/VU: mret with MPV=1, MPP=S/U (lower_priv path)
 * S -> VS/VU: ecall to M -> mret with MPV=1, MPP=S/U
 * VS -> VU:   sret with SPP=U (lower_priv path)
 * VS -> M:    ecall to M-mode handler
 * VS -> HS:   ecall to M -> mret with MPV=0, MPP=S
 * VU -> VS:   ecall to M -> mret with MPV=1, MPP=S
 * VU -> M:    ecall to M -> mret with MPV=0, MPP=M
 * =================================================================== */
void goto_priv(unsigned target) {
    if (target == current_priv)
        return;

#ifdef ENABLE_HYP
    if (priv_cmp(target, current_priv) > 0) {
        /* Need to go up: use ecall */
        do_ecall(ECALL_GOTO_PRIV, target);
    } else {
        /* Going down: use mret/sret */
        lower_priv(target);
    }
#else
    if (target > current_priv) {
        /* Need to go up: use ecall */
        do_ecall(ECALL_GOTO_PRIV, target);
    } else {
        /* Going down: use mret/sret */
        lower_priv(target);
    }
#endif
}

/* ===================================================================
 * run_in_priv - execute a function in a target privilege level
 *
 * Switches to target priv, calls fn(arg), then returns to M-mode.
 * Returns the function's return value.
 *
 * Note: fn must call goto_priv(PRIV_M) or ecall back to M-mode
 * before returning. This wrapper handles it automatically via
 * the test framework's trap handler.
 * =================================================================== */

/* Global storage for run_in_priv result */
static uintptr_t _run_result;
static uintptr_t (*_run_fn)(uintptr_t);
static uintptr_t _run_arg;

__attribute__((used))
static void _run_trampoline(void) {
    _run_result = _run_fn(_run_arg);
    /* Return to M-mode */
    do_ecall(ECALL_GOTO_PRIV, PRIV_M);
}

uintptr_t run_in_priv(unsigned priv, uintptr_t (*fn)(uintptr_t), uintptr_t arg) {
    unsigned saved_priv = current_priv;

    _run_fn  = fn;
    _run_arg = arg;
    _run_result = 0;

#ifdef ENABLE_HYP
    int cmp = priv_cmp(priv, current_priv);

    /* Switch to target privilege and execute */
    if (cmp < 0) {
        /* Going down: set up return to trampoline */
        set_prev_priv(priv);

        if (current_priv == PRIV_M) {
            current_priv = priv;
            /*
             * Use mret to jump to _run_trampoline in the target priv.
             *
             * _run_trampoline calls fn(arg), then does ecall back to
             * M-mode. The ecall handler sets mepc to the instruction
             * after ecall inside _run_trampoline, and mret returns
             * there (in M-mode). _run_trampoline then executes 'ret',
             * which jumps to whatever ra contains.
             *
             * We set ra to point to the label "1:" below so that
             * _run_trampoline's ret lands here, skipping the mret
             * instruction (which must not be re-executed).
             */
            asm volatile(
                "la ra, 1f\n\t"
                "la t0, _run_trampoline\n\t"
                "csrw mepc, t0\n\t"
                "mret\n\t"
                "1:\n\t"
                ::: "ra", "t0", "memory"
            );
        } else if (current_priv == PRIV_S || current_priv == PRIV_VS) {
            if (is_virt_target(priv) && current_priv == PRIV_S) {
                /* Virtualized target from S-mode: route through M-mode */
                do_ecall(ECALL_GOTO_PRIV, PRIV_M);
                /* Now in M-mode, redo the setup */
                set_prev_priv(priv);
                current_priv = priv;
                asm volatile(
                    "la ra, 1f\n\t"
                    "la t0, _run_trampoline\n\t"
                    "csrw mepc, t0\n\t"
                    "mret\n\t"
                    "1:\n\t"
                    ::: "ra", "t0", "memory"
                );
            } else {
                current_priv = priv;
                asm volatile(
                    "la ra, 1f\n\t"
                    "la t0, _run_trampoline\n\t"
                    "csrw sepc, t0\n\t"
                    "sret\n\t"
                    "1:\n\t"
                    ::: "ra", "t0", "memory"
                );
            }
        }
    } else if (cmp > 0) {
        /* Going up: use ecall to switch to higher privilege first,
         * then execute fn in that privilege level via run_in_priv again.
         * This handles the case where we need to execute M-mode-only
         * code (e.g., CSRR mstatus) from S-mode or U-mode. */
        goto_priv(priv);
        /* Now in target privilege, call fn directly */
        _run_result = fn(arg);
    } else {
        /* Same priv: just call directly */
        _run_result = fn(arg);
    }
#else
    /* Switch to target privilege and execute */
    if (priv < current_priv) {
        /* Going down: set up return to trampoline */
        set_prev_priv(priv);

        if (current_priv == PRIV_M) {
            current_priv = priv;
            asm volatile(
                "la ra, 1f\n\t"
                "la t0, _run_trampoline\n\t"
                "csrw mepc, t0\n\t"
                "mret\n\t"
                "1:\n\t"
                ::: "ra", "t0", "memory"
            );
        } else if (current_priv == PRIV_S) {
            current_priv = priv;
            asm volatile(
                "la ra, 1f\n\t"
                "la t0, _run_trampoline\n\t"
                "csrw sepc, t0\n\t"
                "sret\n\t"
                "1:\n\t"
                ::: "ra", "t0", "memory"
            );
        }
    } else if (priv > current_priv) {
        /* Going up: use ecall to switch to higher privilege first */
        goto_priv(priv);
        _run_result = fn(arg);
    } else {
        /* Same priv: just call directly */
        _run_result = fn(arg);
    }
#endif

    /* Ensure we're back in the original privilege */
    if (current_priv != saved_priv)
        goto_priv(saved_priv);

    return _run_result;
}
