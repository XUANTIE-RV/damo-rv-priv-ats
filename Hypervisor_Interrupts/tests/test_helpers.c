/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * test_helpers.c - Hypervisor interrupt test helpers
 *
 * Provides the VS-mode interrupt delivery fixture: the naked VS-mode
 * handler installed at vstvec, its interrupt record, and the setup
 * routine. Shared by Group 1-2 (test_interrupts.c) and Group 4
 * (test_deleg_interrupt.c). See test_helpers.h for why the record
 * variables have external linkage.
 * =================================================================== */

#include "hyp_test_helpers.h"
#include "test_helpers.h"

/* ===================================================================
 * VS-mode interrupt handler
 *
 * This handler is installed at vstvec and runs in VS-mode when a
 * VS-level interrupt is delegated via hideleg. It records the first
 * interrupt cause, disables SIE to prevent re-entry, and returns
 * via sret.
 * =================================================================== */

volatile uintptr_t g_vs_int_cause;
volatile bool g_vs_int_triggered;

static void vs_int_handler(void) __attribute__((naked, aligned(4)));
static void vs_int_handler(void)
{
    asm volatile (
        "addi   sp, sp, -32\n\t"
        "sd     ra, 0(sp)\n\t"
        "sd     t0, 8(sp)\n\t"
        "sd     t1, 16(sp)\n\t"
        "sd     t2, 24(sp)\n\t"

        /* Record cause only if not already triggered (for priority) */
        "la     t2, g_vs_int_triggered\n\t"
        "lb     t0, 0(t2)\n\t"
        "bnez   t0, 1f\n\t"

        "csrr   t0, scause\n\t"
        "la     t2, g_vs_int_cause\n\t"
        "sd     t0, 0(t2)\n\t"
        "li     t0, 1\n\t"
        "la     t2, g_vs_int_triggered\n\t"
        "sb     t0, 0(t2)\n\t"

        "1:\n\t"
        /* Disable S-mode interrupts to prevent re-entry.
         * Must clear both SIE (bit 1) and SPIE (bit 5) because
         * sret restores SIE from SPIE. If we only clear SIE,
         * sret would restore SIE=1 from SPIE and re-trigger
         * the interrupt (infinite loop for read-only pending bits
         * like VSTIP/VSEIP that can't be cleared from VS-mode).
         * Use register form since 0x22 exceeds csrci 5-bit range. */
        "li     t0, 0x22\n\t"
        "csrc   sstatus, t0\n\t"
        /* Clear VSSIP pending bit via sip (alias for hvip when delegated).
         * This only works for VSSIP; VSTIP/VSEIP are read-only in vsip. */
        "csrc   sip, 0x2\n\t"

        /* Force SPP=1 (S-mode) so sret returns to VS-mode, not VU.
         * When the interrupt came from VU-mode, hardware set SPP=0. */
        "li     t0, 0x100\n\t"
        "csrs   sstatus, t0\n\t"

        /* Restore registers */
        "ld     ra, 0(sp)\n\t"
        "ld     t0, 8(sp)\n\t"
        "ld     t1, 16(sp)\n\t"
        "ld     t2, 24(sp)\n\t"
        "addi   sp, sp, 32\n\t"

        "sret\n\t"
    );
}

/* ===================================================================
 * Helper: set up VS interrupt test infrastructure
 *
 * Configures delegation, vstvec, and vsstatus.SIE for VS-mode
 * interrupt delivery testing.
 * =================================================================== */
void setup_vs_int_test(uintptr_t hideleg_mask)
{
    /* Clear global interrupt record */
    g_vs_int_cause = 0;
    g_vs_int_triggered = false;

    /* Clear all interrupt sources to prevent spurious interrupts.
     * hie/hvip are HS-level CSRs, directly accessible from HS-mode. */
    CSRW(CSR_HIE, 0);   /* hie = 0 */
    CSRW(CSR_HVIP, 0);   /* hvip = 0 */
    /* Disarm the vstimecmp (0x24D) timer source so a timer-driven VSTIP cannot
     * mix with the injected one. From HS-mode vstimecmp is reachable only when
     * menvcfg.STCE=1, and platforms differ (QEMU tends to reset STCE=1,
     * spike/hardware STCE=0), so guard the write: where STCE=0 the access traps
     * and is swallowed, which is harmless because with STCE=0 the comparator
     * cannot raise VSTIP anyway. (The old code wrote stimecmp/0x14D, the wrong
     * register, which HS-mode also gates on STCE.) */
    trap_expect_begin();
    CSRW(CSR_VSTIMECMP, (uintptr_t)-1);
    trap_expect_end();

    /* Dual-layer delegation: M -> HS (mideleg) -> VS (hideleg).
     * mideleg is M-level config, unreachable from S(HS)-mode: proxy the
     * update to M-mode explicitly via tsbi_csr_set(). */
    tsbi_csr_set(CSR_MIDELEG, hideleg_mask);
    hideleg_write(hideleg_mask);

    /* Install VS-mode trap handler (Direct mode).
     * IMPORTANT: vs_int_handler must be 4-byte aligned so that
     * vstvec base address matches the handler entry point exactly. */
    vs_trap_setup_direct((uintptr_t)vs_int_handler);

    /* Enable VS-mode interrupts: set vsstatus.SIE (bit 1). vsstatus is an
     * HS-level CSR, directly accessible from HS-mode. */
    uintptr_t vsstatus;
    vsstatus = CSRR(CSR_VSSTATUS);
    vsstatus |= 0x2;  /* SIE */
    CSRW(CSR_VSSTATUS, vsstatus);

    /* (removed) The old code preset mstatus.MPIE=1 "so that after mret,
     * MIE=1" for interrupt delivery. Under the S-mode-normal + T-SBI model
     * this is both unimplementable and unnecessary: MPIE is a return-path bit
     * the tsbi_csr_ proxy guard refuses, and every framework excursion
     * re-enters via mret (MPIE<-1). mstatus.MIE only gates interrupts while
     * executing IN M-mode; in VS-mode (privilege < M) M-interrupts are always
     * globally enabled. VS-level delivery is gated by vsstatus.SIE (set above)
     * + hideleg + vsie + hie, not by mstatus.MIE. */
}
