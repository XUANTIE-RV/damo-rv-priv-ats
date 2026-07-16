/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/hyp_reset.c — clean-slate reset of all H-ext state
 * =================================================================== */

#include "hyp_reset.h"
#include "hyp_csr.h"
#include "encoding.h"

extern void reset_state(void);   /* base reset in test_framework.c */

/* ===================================================================
 * PMP bootstrap for V=1 execution
 *
 * QEMU virt with -bios none leaves all PMP entries at reset value
 * (cfg=0, addr=0). With H-extension enabled, V=1 (VS/VU) accesses
 * are checked against PMP just like ordinary S/U accesses, so a
 * zero-CSR PMP denies *every* S/U/V instruction fetch and data
 * access — the very first `mret` into a VS-mode trampoline then
 * immediately raises mcause=1 (instruction access fault).
 *
 * The G-stage tests here run inside a single trusted image, so we
 * install one wide-open PMP entry covering the whole 64-bit address
 * space (RWX for S/U/V). We do this exactly once per reset; further
 * calls are idempotent (the same value is rewritten).
 *
 * Encoding: pmpaddr0 = (1<<54)-1  (all-ones in the low 54 bits, so
 * the NAPOT range is the entire space starting at 0). pmpcfg0[7:0] =
 * R|W|X | A=NAPOT (0x18) = 0x1F. Not locked (L=0) so test code that
 * deliberately pokes PMP later can still take effect.
 * =================================================================== */
static void hyp_pmp_open_all(void) {
    uintptr_t napot_all = ~(uintptr_t)0 >> 10; /* (1<<54)-1 — full NAPOT */
    CSRW(CSR_PMPADDR0, napot_all);
    /* R=1, W=1, X=1, A=NAPOT(3) → byte = 0x1F. Place in entry 0. */
    uintptr_t cfg = 0x1FUL;
    CSRW(CSR_PMPCFG0, cfg);
}

void hyp_reset_state(void) {
    /* ----- PMP: ensure VS/VU accesses are not blanket-denied ----- */
    hyp_pmp_open_all();

    /* ----- HS-level hypervisor CSRs ----- */
    hgatp_set_bare();           /* hgatp = Bare + HFENCE.GVMA all */
    hedeleg_write(0);
    hideleg_write(0);
    /* hie / hip / hvip / hgeie are interrupt-related — clear hvip + hie. */
    CSRW(CSR_HIE, 0);
    CSRW(CSR_HVIP, 0);
    CSRW(CSR_HGEIE, 0);

    /* hstatus: clear VTSR/VTW/VTVM/HU/SPV/SPVP/GVA. Preserve VSXL
     * (it is WARL on RV64; writing 0 may be ignored, but is fine). */
    hstatus_write(0);

    /* ----- M-mode menvcfg: force ADUE=0 -----
     *
     * Per norm:menvcfg_adue_op (machine.adoc), menvcfg.ADUE controls
     * G-stage A/D update behavior:
     *   ADUE=1 -> hardware auto-updates A/D bits (Svadu enabled)
     *   ADUE=0 -> behaves as Svade: page-fault when A/D needs setting
     *             (norm:Svadu_disabled_hw_update_falls_back_to_svade,
     *              svade in supervisor.adoc)
     *
     * QEMU 8.2.94 default `-cpu rv64,h=true` resets menvcfg.ADUE=1,
     * which silently breaks tests that expect Svade fault semantics
     * (sv39x4 GAD-01/02/03). Force ADUE=0 here to put every test
     * onto the spec-mandated Svade path. See diagnostic report at
     * docs/gad_diagnose_report.md for full root-cause analysis.
     *
     * Preserve PBMTE and other bits (Svpbmt etc.) -- only clear ADUE.
     * Per norm:menvcfg_adue_henvcfg_adue_rdonly0, this also forces
     * henvcfg.ADUE to read-only zero, so the henvcfg_write(0) below
     * remains effective.
     *
     * SFENCE.VMA synchronizes address-translation caches w.r.t. the
     * new A/D interpretation (norm:menvcfg_adue_fence); the trailing
     * hfence_gvma_all() at the end of this function further covers
     * G-stage caches (hyp-mm-fences). */
    {
        uintptr_t mv = menvcfg_read();
        menvcfg_write(mv & ~MENVCFG_ADUE);
        asm volatile ("sfence.vma" ::: "memory");
    }

    henvcfg_write(0);
    hcounteren_write(0);
    htimedelta_write(0);

    /* htval / htinst are read-only side-effects of traps; clearing
     * them is not strictly required, but keeps the trap_record clean. */
    CSRW(CSR_HTVAL, 0);
    CSRW(CSR_HTINST, 0);

    /* ----- VS-level CSRs ----- */
    CSRW(CSR_VSSTATUS, 0);
    CSRW(CSR_VSIE, 0);
    CSRW(CSR_VSTVEC, 0);
    CSRW(CSR_VSSCRATCH, 0);
    CSRW(CSR_VSEPC, 0);
    CSRW(CSR_VSCAUSE, 0);
    CSRW(CSR_VSTVAL, 0);
    CSRW(CSR_VSIP, 0);
    CSRW(CSR_VSATP, 0);

    /* Final TLB flush in case any G-stage entries linger. */
    hfence_gvma_all();

    /* ----- Forward to base reset (PMP, satp, M-mode return, etc.) ----- */
    reset_state();
}
