/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/hyp_csr.c — Hypervisor CSR accessors (G-stage subset)
 * =================================================================== */

#include "hyp_csr.h"
#include "encoding.h"
#include "sm_defs.h"          /* CSR_MHPMCOUNTER3 (counter existence probe) */
#include "test_framework.h"   /* PRIV_S / PRIV_U / PRIV_M */

/* ===================================================================
 * hgatp
 * =================================================================== */

uintptr_t hgatp_read(void) {
    return CSRR(CSR_HGATP);
}

/* Raw write: write the value untouched. Reserved for tests that
 * deliberately probe WARL behaviour (see hgatp_write_raw doc in
 * hyp_csr.h). */
void hgatp_write_raw(uintptr_t value) {
    CSRW(CSR_HGATP, value);
}

/* Spec-correct write: apply the WARL masking required by the
 * H-extension spec before writing the underlying CSR.
 *
 * Per spec (Hypervisor extension, hgatp encoding):
 *   - MODE = Bare        → PPN and VMID must read as zero
 *   - MODE = Sv39x4 / Sv48x4 / Sv57x4
 *                        → PPN[1:0] are hard-wired to zero
 *                          (root page table is 16KB-aligned)
 *
 * Some implementations (notably current QEMU `virt`) do not enforce
 * these WARL constraints in their write path, so we mask here so
 * that the caller-observable hgatp behaviour is always spec-correct.
 * Tests that explicitly probe HW WARL behaviour for reserved MODE
 * values must call hgatp_write_raw() instead. */
void hgatp_write(uintptr_t value) {
    uintptr_t mode = (uintptr_t)((uint64_t)value >> HGATP64_MODE_SHIFT) & 0xFUL;
    if (mode == HGATP_MODE_BARE) {
        value = 0;
    } else if (mode == HGATP_MODE_SV39X4 ||
               mode == HGATP_MODE_SV48X4 ||
               mode == HGATP_MODE_SV57X4) {
        value &= ~((uintptr_t)0x3UL);
    }
    CSRW(CSR_HGATP, value);
}

void hgatp_set(int mode, unsigned vmid, uintptr_t root_ppn) {
    hgatp_write(MAKE_HGATP(mode, vmid, root_ppn));
    hfence_gvma_all();
}

void hgatp_set_bare(void) {
    hgatp_write(0);
    hfence_gvma_all();
}

bool hgatp_supports_mode(int mode) {
    uintptr_t saved = hgatp_read();
    hgatp_set(mode, /*vmid=*/0, /*ppn=*/0);
    uintptr_t got = hgatp_read();
    hgatp_write(saved);
    hfence_gvma_all();
    return HGATP_GET_MODE(got) == (uintptr_t)mode;
}

/* ===================================================================
 * hstatus
 * =================================================================== */

uintptr_t hstatus_read(void) {
    return CSRR(CSR_HSTATUS);
}

void hstatus_write(uintptr_t value) {
    CSRW(CSR_HSTATUS, value);
}

void hstatus_set_spv(bool v) {
    uintptr_t hs = hstatus_read();
    if (v) hs |=  HSTATUS_SPV;
    else   hs &= ~HSTATUS_SPV;
    hstatus_write(hs);
}

void hstatus_set_spvp(unsigned priv) {
    uintptr_t hs = hstatus_read();
    if (priv == PRIV_S) hs |=  HSTATUS_SPVP;
    else                hs &= ~HSTATUS_SPVP;
    hstatus_write(hs);
}

unsigned hstatus_get_gva(void) {
    return (hstatus_read() & HSTATUS_GVA) ? 1U : 0U;
}

unsigned hstatus_get_spv(void) {
    return (hstatus_read() & HSTATUS_SPV) ? 1U : 0U;
}

/* ===================================================================
 * hedeleg / hideleg
 * =================================================================== */

void hedeleg_write(uintptr_t value) {
    CSRW(CSR_HEDELEG, value);
}
uintptr_t hedeleg_read(void) {
    return CSRR(CSR_HEDELEG);
}

void hideleg_write(uintptr_t value) {
    CSRW(CSR_HIDELEG, value);
}
uintptr_t hideleg_read(void) {
    return CSRR(CSR_HIDELEG);
}

/* ===================================================================
 * henvcfg / hcounteren / htimedelta
 * =================================================================== */

void henvcfg_write(uintptr_t value) {
    CSRW(CSR_HENVCFG, value);
}
uintptr_t henvcfg_read(void) {
    return CSRR(CSR_HENVCFG);
}

void hcounteren_write(uintptr_t value) {
    CSRW(CSR_HCOUNTEREN, value);
}

void htimedelta_write(uintptr_t value) {
    CSRW(CSR_HTIMEDELTA, value);
}

/* ===================================================================
 * menvcfg (M-mode CSR 0x30A)
 *
 * Defined here (rather than in a separate machine_csr.c) because the
 * GAD diagnostics in sv39x4 are the only current consumer, and they
 * already pull in hyp_csr for henvcfg / hgatp.
 * =================================================================== */

uintptr_t menvcfg_read(void) {
    return CSRR(CSR_MENVCFG);
}

void menvcfg_write(uintptr_t value) {
    CSRW(CSR_MENVCFG, value);
}

/* ===================================================================
 * mstatus (CSR 0x300)
 * =================================================================== */

uintptr_t mstatus_read(void) {
    return CSRR(mstatus);
}

void mstatus_write(uintptr_t value) {
    CSRW(mstatus, value);
}

uintptr_t senvcfg_read(void) {
    return CSRR(CSR_SENVCFG);
}

void senvcfg_write(uintptr_t value) {
    CSRW(CSR_SENVCFG, value);
}

/* ===================================================================
 * hstatus field control
 * =================================================================== */

void hstatus_set_vtsr(bool enable) {
    uintptr_t hs = hstatus_read();
    if (enable) hs |=  HSTATUS_VTSR;
    else        hs &= ~HSTATUS_VTSR;
    hstatus_write(hs);
}

void hstatus_set_vtw(bool enable) {
    uintptr_t hs = hstatus_read();
    if (enable) hs |=  HSTATUS_VTW;
    else        hs &= ~HSTATUS_VTW;
    hstatus_write(hs);
}

void hstatus_set_vtvm(bool enable) {
    uintptr_t hs = hstatus_read();
    if (enable) hs |=  HSTATUS_VTVM;
    else        hs &= ~HSTATUS_VTVM;
    hstatus_write(hs);
}

void hstatus_set_hu(bool enable) {
    uintptr_t hs = hstatus_read();
    if (enable) hs |=  HSTATUS_HU;
    else        hs &= ~HSTATUS_HU;
    hstatus_write(hs);
}

/* ===================================================================
 * Trap delegation
 * =================================================================== */

void hyp_delegate_to_vs(uintptr_t hedeleg_mask, uintptr_t hideleg_mask) {
    /* First ensure medeleg/mideleg delegates these to HS-mode.
     * Read current medeleg and OR in the requested bits.
     * Local variables use _val suffix to avoid shadowing the CSR
     * name macros used in CSRR/CSRW. */
    uintptr_t md_val  = CSRR(medeleg);
    uintptr_t mid_val = CSRR(mideleg);
    md_val  |= hedeleg_mask;
    mid_val |= hideleg_mask;
    CSRW(medeleg, md_val);
    CSRW(mideleg, mid_val);

    /* Then delegate from HS-mode to VS-mode via hedeleg/hideleg. */
    hedeleg_write(hedeleg_read() | hedeleg_mask);
    hideleg_write(hideleg_read() | hideleg_mask);
}

void hyp_undelegate(void) {
    hedeleg_write(0);
    hideleg_write(0);
}

/* ===================================================================
 * Virtual interrupt injection (hvip)
 * =================================================================== */

/* hvip bit positions (interrupt cause numbers):
 *   VSSIP = bit 2  (VS software interrupt)
 *   VSTIP = bit 6  (VS timer interrupt)
 *   VSEIP = bit 10 (VS external interrupt)
 *
 * HVIP_VSTIP is already defined in encoding.h; define the others
 * only if not already available.
 */
#ifndef HVIP_VSSIP
#define HVIP_VSSIP  (1UL << 2)
#endif
#ifndef HVIP_VSTIP
#define HVIP_VSTIP  (1UL << 6)
#endif
#ifndef HVIP_VSEIP
#define HVIP_VSEIP  (1UL << 10)
#endif

uintptr_t hvip_read(void) {
    return CSRR(CSR_HVIP);
}

void hvip_write(uintptr_t value) {
    CSRW(CSR_HVIP, value);
}

void hvip_set_vssi(bool pending) {
    uintptr_t v = hvip_read();
    if (pending) v |=  HVIP_VSSIP;
    else         v &= ~HVIP_VSSIP;
    hvip_write(v);
}

void hvip_set_vsti(bool pending) {
    uintptr_t v = hvip_read();
    if (pending) v |=  HVIP_VSTIP;
    else         v &= ~HVIP_VSTIP;
    hvip_write(v);
}

void hvip_set_vsei(bool pending) {
    uintptr_t v = hvip_read();
    if (pending) v |=  HVIP_VSEIP;
    else         v &= ~HVIP_VSEIP;
    hvip_write(v);
}

/* ===================================================================
 * henvcfg fine-grained control
 * =================================================================== */

/* henvcfg bit positions (same as menvcfg for the shared fields) */
#define HENVCFG_PBMTE  (1ULL << 62)
#define HENVCFG_ADUE   (1ULL << 61)
/* HENVCFG_STCE already defined in encoding.h as (1ULL << 63) */

void henvcfg_set_pbmte(bool enable) {
    uintptr_t v = henvcfg_read();
    if (enable) v |=  HENVCFG_PBMTE;
    else        v &= ~HENVCFG_PBMTE;
    henvcfg_write(v);
}

void henvcfg_set_adue(bool enable) {
    uintptr_t v = henvcfg_read();
    if (enable) v |=  HENVCFG_ADUE;
    else        v &= ~HENVCFG_ADUE;
    henvcfg_write(v);
}

void henvcfg_set_stce(bool enable) {
    uintptr_t v = henvcfg_read();
    if (enable) v |=  HENVCFG_STCE;
    else        v &= ~HENVCFG_STCE;
    henvcfg_write(v);
}

/* ===================================================================
 * hcounteren / htimedelta extended API
 * =================================================================== */

uintptr_t hcounteren_read(void) {
    return CSRR(CSR_HCOUNTEREN);
}

void hcounteren_set(uintptr_t mask) {
    CSRS(CSR_HCOUNTEREN, mask);
}

void hcounteren_clear(uintptr_t mask) {
    CSRC(CSR_HCOUNTEREN, mask);
}

void htimedelta_set(uint64_t delta) {
    htimedelta_write((uintptr_t)delta);
}

/* ===================================================================
 * mcounteren (M-mode Counter Enable, CSR 0x306)
 * =================================================================== */

uintptr_t mcounteren_read(void) {
    return CSRR(CSR_MCOUNTEREN);
}

void mcounteren_write(uintptr_t value) {
    CSRW(CSR_MCOUNTEREN, value);
}

void mcounteren_set(uintptr_t mask) {
    CSRS(CSR_MCOUNTEREN, mask);
}

void mcounteren_clear(uintptr_t mask) {
    CSRC(CSR_MCOUNTEREN, mask);
}

/* ===================================================================
 * scounteren (S-mode Counter Enable, CSR 0x106)
 * =================================================================== */

uintptr_t scounteren_read(void) {
    return CSRR(CSR_SCOUNTEREN);
}

void scounteren_write(uintptr_t value) {
    CSRW(CSR_SCOUNTEREN, value);
}

void scounteren_set(uintptr_t mask) {
    CSRS(CSR_SCOUNTEREN, mask);
}

void scounteren_clear(uintptr_t mask) {
    CSRC(CSR_SCOUNTEREN, mask);
}

/* ===================================================================
 * vstvec (CSR 0x205)
 * =================================================================== */

uintptr_t vstvec_read(void) {
    return CSRR(CSR_VSTVEC);
}

void vstvec_write(uintptr_t value) {
    CSRW(CSR_VSTVEC, value);
}

void vstvec_set_mode(int mode) {
    uintptr_t v = vstvec_read();
    v = (v & VSTVEC_BASE_MASK) | ((uintptr_t)mode & VSTVEC_MODE_MASK);
    vstvec_write(v);
}

uintptr_t vstvec_get_base(void) {
    return vstvec_read() & VSTVEC_BASE_MASK;
}

int vstvec_get_mode(void) {
    return (int)(vstvec_read() & VSTVEC_MODE_MASK);
}

/* ===================================================================
 * vstval (CSR 0x243)
 * =================================================================== */

uintptr_t vstval_read(void) {
    return CSRR(CSR_VSTVAL);
}

void vstval_write(uintptr_t value) {
    CSRW(CSR_VSTVAL, value);
}

/* ===================================================================
 * vsie (CSR 0x204) / vsip (CSR 0x244)
 * =================================================================== */

uintptr_t vsie_read(void) {
    return CSRR(CSR_VSIE);
}

void vsie_write(uintptr_t value) {
    CSRW(CSR_VSIE, value);
}

uintptr_t vsip_read(void) {
    return CSRR(CSR_VSIP);
}

void vsip_write(uintptr_t value) {
    CSRW(CSR_VSIP, value);
}

/* ===================================================================
 * vsstatus (CSR 0x200)
 * =================================================================== */

uintptr_t vsstatus_read(void) {
    return CSRR(CSR_VSSTATUS);
}

void vsstatus_write(uintptr_t value) {
    CSRW(CSR_VSSTATUS, value);
}

/* ===================================================================
 * vsatp (CSR 0x280)
 * =================================================================== */

uintptr_t vsatp_read(void) {
    return CSRR(CSR_VSATP);
}

void vsatp_write(uintptr_t value) {
    CSRW(CSR_VSATP, value);
}

/* ===================================================================
 * satp (CSR 0x180)
 * =================================================================== */

uintptr_t satp_read(void) {
    return CSRR(CSR_SATP);
}

void satp_write(uintptr_t value) {
    CSRW(CSR_SATP, value);
}

/* ===================================================================
 * satp / vsatp mode probe
 *
 * Per spec (supervisor.adoc), writing an unsupported MODE to satp
 * causes the entire write to be IGNORED (satp reads back the old
 * value). This differs from hgatp where unsupported MODE still
 * processes the write.
 * =================================================================== */

bool satp_supports_mode(int mode) {
    uintptr_t saved = satp_read();
    uintptr_t probe = MAKE_SATP(mode, 0, 0);
    satp_write(probe);
    uintptr_t got = satp_read();
    satp_write(saved);
    asm volatile ("sfence.vma" ::: "memory");
    return SATP_GET_MODE(got) == (uintptr_t)mode;
}

bool vsatp_supports_mode(int mode) {
    uintptr_t saved = vsatp_read();
    uintptr_t probe = MAKE_SATP(mode, 0, 0);
    vsatp_write(probe);
    uintptr_t got = vsatp_read();
    vsatp_write(saved);
    hfence_vvma_all();
    return SATP_GET_MODE(got) == (uintptr_t)mode;
}

/* ===================================================================
 * ASID / VMID width probing
 *
 * Technique: write all-ones to the ASID/VMID field, read back, count
 * set bits to determine the implemented width.
 * =================================================================== */

static unsigned count_bits(uintptr_t v) {
    unsigned n = 0;
    while (v) { n++; v >>= 1; }
    return n;
}

unsigned satp_asid_width(void) {
    uintptr_t saved = satp_read();
    /* Write Bare mode with all-ones ASID, PPN=0 */
    uintptr_t probe = MAKE_SATP(SATP_MODE_BARE, SATP64_ASID_MASK, 0);
    satp_write(probe);
    uintptr_t got = satp_read();
    satp_write(saved);
    return count_bits(SATP_GET_ASID(got));
}

unsigned vsatp_asid_width(void) {
    uintptr_t saved = vsatp_read();
    uintptr_t probe = MAKE_SATP(SATP_MODE_BARE, SATP64_ASID_MASK, 0);
    vsatp_write(probe);
    uintptr_t got = vsatp_read();
    vsatp_write(saved);
    return count_bits(SATP_GET_ASID(got));
}

unsigned hgatp_vmid_width(void) {
    uintptr_t saved = hgatp_read();
    /* Write Bare=0 won't work (PPN/VMID forced to 0 on Bare).
     * Must use a supported non-Bare mode for the probe. */
    int probe_mode = HGATP_MODE_SV39X4;
    if (!hgatp_supports_mode(probe_mode)) {
        probe_mode = HGATP_MODE_SV48X4;
        if (!hgatp_supports_mode(probe_mode)) {
            probe_mode = HGATP_MODE_SV57X4;
            if (!hgatp_supports_mode(probe_mode)) {
                hgatp_write(saved);
                return 0;
            }
        }
    }
    uintptr_t probe = MAKE_HGATP(probe_mode, HGATP64_VMID_MASK, 0);
    hgatp_write_raw(probe);
    uintptr_t got = hgatp_read();
    hgatp_write(saved);
    hfence_gvma_all();
    return count_bits(HGATP_GET_VMID(got));
}

/* ===================================================================
 * Generic CSR WARL probe
 *
 * Uses the csr_accessors infrastructure (csr_read / csr_write)
 * for arbitrary CSR access.
 * =================================================================== */

/* Forward declarations for csr_accessors.c */
extern uintptr_t csr_read(uint16_t csr);
extern void      csr_write(uint16_t csr, uintptr_t val);

uintptr_t csr_warl_probe(unsigned csr_num, uintptr_t value) {
    uintptr_t saved = csr_read((uint16_t)csr_num);
    csr_write((uint16_t)csr_num, value);
    uintptr_t readback = csr_read((uint16_t)csr_num);
    csr_write((uint16_t)csr_num, saved);
    return readback;
}

/* ===================================================================
 * Counter detection helpers
 * =================================================================== */

/* Probe which mcounteren HPM gate bits (3..31) are WARL-writable.
 *
 * NOTE: this measures mcounteren gate-bit stickiness ONLY; it does
 * NOT prove the corresponding hpmcounter exists. Per
 * norm:mcounteren_flds_rdonly0 (machine.adoc) a read-only-zero gate
 * bit only means the counter is inaccessible from lower-privileged
 * modes, and a writable gate bit carries no existence guarantee
 * either. Use hpmcounter_is_writable() for counter existence
 * detection. */
uint32_t counteren_probe_implemented(void) {
    uintptr_t saved = mcounteren_read();
    uint32_t bitmap = 0;

    /* Probe bits 3..31 (0=cycle, 1=time, 2=instret are always special) */
    for (int i = 3; i < 32; i++) {
        uint32_t bit = (1U << i);
        mcounteren_write(saved | bit);
        if (mcounteren_read() & bit)
            bitmap |= bit;
    }

    mcounteren_write(saved);
    return bitmap;
}

/* Counter existence probe per the Shcounterenw_test_plan.md strategy:
 * in M-mode, write a non-zero value to mhpmcounterN and read it back.
 * If the value sticks (non-zero readback) the counter is considered
 * implemented; a read-only-zero mirror is treated as unimplemented
 * (norm:mhpmcounter_mhpmevent_rdonly0 permits such mirrors, and they
 * are indistinguishable from an absent counter for test purposes).
 *
 * M-mode access is unaffected by mcounteren, so this probe remains
 * valid even when the gate bits are read-only zero (unlike the
 * mcounteren-bit stickiness probe above). The access is trap-armed:
 * when the mhpmcounter CSR does not exist at all, the access traps
 * with illegal-instruction and the counter is reported unimplemented. */
bool hpmcounter_is_writable(int idx) {
    if (idx < 3 || idx > 31)
        return false;

    uint16_t csr = (uint16_t)(CSR_MHPMCOUNTER3 + (idx - 3));
    bool trapped;

    trap_expect_begin();
    uintptr_t saved = csr_read(csr);
    trapped = trap_was_triggered();
    trap_expect_end();
    if (trapped)
        return false;

    trap_expect_begin();
    csr_write(csr, 0xDEADBEEFUL);
    uintptr_t rb = csr_read(csr);
    trapped = trap_was_triggered();
    trap_expect_end();
    if (trapped)
        return false;

    /* Best-effort restore; a read-only-zero mirror ignores it. */
    trap_expect_begin();
    csr_write(csr, saved);
    trap_expect_end();

    return rb != 0;
}

/* ===================================================================
 * Smstateen / Ssstateen CSR accessors
 * =================================================================== */

uintptr_t mstateen_read(int idx) {
    uintptr_t v = 0;
    switch (idx) {
    case 0: v = CSRR(CSR_MSTATEEN0); break;
    case 1: v = CSRR(CSR_MSTATEEN1); break;
    case 2: v = CSRR(CSR_MSTATEEN2); break;
    case 3: v = CSRR(CSR_MSTATEEN3); break;
    default: break;
    }
    return v;
}

void mstateen_write(int idx, uintptr_t value) {
    switch (idx) {
    case 0: CSRW(CSR_MSTATEEN0, value); break;
    case 1: CSRW(CSR_MSTATEEN1, value); break;
    case 2: CSRW(CSR_MSTATEEN2, value); break;
    case 3: CSRW(CSR_MSTATEEN3, value); break;
    default: break;
    }
}

void mstateen_set_bits(int idx, uintptr_t mask) {
    mstateen_write(idx, mstateen_read(idx) | mask);
}

void mstateen_clear_bits(int idx, uintptr_t mask) {
    mstateen_write(idx, mstateen_read(idx) & ~mask);
}

uintptr_t hstateen_read(int idx) {
    uintptr_t v = 0;
    switch (idx) {
    case 0: v = CSRR(CSR_HSTATEEN0); break;
    case 1: v = CSRR(CSR_HSTATEEN1); break;
    case 2: v = CSRR(CSR_HSTATEEN2); break;
    case 3: v = CSRR(CSR_HSTATEEN3); break;
    default: break;
    }
    return v;
}

void hstateen_write(int idx, uintptr_t value) {
    switch (idx) {
    case 0: CSRW(CSR_HSTATEEN0, value); break;
    case 1: CSRW(CSR_HSTATEEN1, value); break;
    case 2: CSRW(CSR_HSTATEEN2, value); break;
    case 3: CSRW(CSR_HSTATEEN3, value); break;
    default: break;
    }
}

void hstateen_set_bits(int idx, uintptr_t mask) {
    hstateen_write(idx, hstateen_read(idx) | mask);
}

void hstateen_clear_bits(int idx, uintptr_t mask) {
    hstateen_write(idx, hstateen_read(idx) & ~mask);
}

uintptr_t sstateen_read(int idx) {
    uintptr_t v = 0;
    switch (idx) {
    case 0: v = CSRR(CSR_SSTATEEN0); break;
    case 1: v = CSRR(CSR_SSTATEEN1); break;
    case 2: v = CSRR(CSR_SSTATEEN2); break;
    case 3: v = CSRR(CSR_SSTATEEN3); break;
    default: break;
    }
    return v;
}

void sstateen_write(int idx, uintptr_t value) {
    switch (idx) {
    case 0: CSRW(CSR_SSTATEEN0, value); break;
    case 1: CSRW(CSR_SSTATEEN1, value); break;
    case 2: CSRW(CSR_SSTATEEN2, value); break;
    case 3: CSRW(CSR_SSTATEEN3, value); break;
    default: break;
    }
}

void sstateen_set_bits(int idx, uintptr_t mask) {
    sstateen_write(idx, sstateen_read(idx) | mask);
}

void sstateen_clear_bits(int idx, uintptr_t mask) {
    sstateen_write(idx, sstateen_read(idx) & ~mask);
}

/* ===================================================================
 * Indirect CSR accessors (Smcsrind / Sscsrind / Ssccfg)
 *
 * Plain CSRR/CSRW wrappers for the miselect/mireg, siselect/sireg*
 * and vsiselect/vsireg* indirect-CSR windows. These centralize the
 * accessors that used to be duplicated per-suite. Trap-armed probing
 * variants (e.g. *_read_safe) remain suite-local because their
 * arming/recovery semantics are test-specific.
 * =================================================================== */

uintptr_t miselect_read(void) { return CSRR(CSR_MISELECT); }
void      miselect_write(uintptr_t v) { CSRW(CSR_MISELECT, v); }
uintptr_t mireg_read(void) { return CSRR(CSR_MIREG); }
void      mireg_write(uintptr_t v) { CSRW(CSR_MIREG, v); }

uintptr_t siselect_read(void) { return CSRR(CSR_SISELECT); }
void      siselect_write(uintptr_t v) { CSRW(CSR_SISELECT, v); }
uintptr_t sireg_read(void) { return CSRR(CSR_SIREG); }
void      sireg_write(uintptr_t v) { CSRW(CSR_SIREG, v); }
uintptr_t sireg2_read(void) { return CSRR(CSR_SIREG2); }
void      sireg2_write(uintptr_t v) { CSRW(CSR_SIREG2, v); }
uintptr_t sireg3_read(void) { return CSRR(CSR_SIREG3); }
void      sireg3_write(uintptr_t v) { CSRW(CSR_SIREG3, v); }
uintptr_t sireg4_read(void) { return CSRR(CSR_SIREG4); }
void      sireg4_write(uintptr_t v) { CSRW(CSR_SIREG4, v); }
uintptr_t sireg5_read(void) { return CSRR(CSR_SIREG5); }
void      sireg5_write(uintptr_t v) { CSRW(CSR_SIREG5, v); }
uintptr_t sireg6_read(void) { return CSRR(CSR_SIREG6); }
void      sireg6_write(uintptr_t v) { CSRW(CSR_SIREG6, v); }

uintptr_t vsiselect_read(void) { return CSRR(CSR_VSISELECT); }
void      vsiselect_write(uintptr_t v) { CSRW(CSR_VSISELECT, v); }
uintptr_t vsireg_read(void) { return CSRR(CSR_VSIREG); }
void      vsireg_write(uintptr_t v) { CSRW(CSR_VSIREG, v); }
uintptr_t vsireg2_read(void) { return CSRR(CSR_VSIREG2); }
void      vsireg2_write(uintptr_t v) { CSRW(CSR_VSIREG2, v); }
uintptr_t vsireg3_read(void) { return CSRR(CSR_VSIREG3); }
void      vsireg3_write(uintptr_t v) { CSRW(CSR_VSIREG3, v); }
uintptr_t vsireg4_read(void) { return CSRR(CSR_VSIREG4); }
void      vsireg4_write(uintptr_t v) { CSRW(CSR_VSIREG4, v); }
uintptr_t vsireg5_read(void) { return CSRR(CSR_VSIREG5); }
void      vsireg5_write(uintptr_t v) { CSRW(CSR_VSIREG5, v); }
uintptr_t vsireg6_read(void) { return CSRR(CSR_VSIREG6); }
void      vsireg6_write(uintptr_t v) { CSRW(CSR_VSIREG6, v); }

/* ===================================================================
 * Generic atomic set/clear bit helpers (single CSRS/CSRC instruction)
 *
 * menvcfg / henvcfg / hvip / hstatus / vsstatus. These complement the
 * read/write accessors above and replace the per-suite csrs/csrc
 * wrappers (menvcfg_set/clear, henvcfg_set/clear, hvip_set/clear,
 * hstatus_set/clear, vsstatus_set/clear).
 * =================================================================== */

void menvcfg_set_bits(uintptr_t mask)   { CSRS(CSR_MENVCFG, mask); }
void menvcfg_clear_bits(uintptr_t mask) { CSRC(CSR_MENVCFG, mask); }
void senvcfg_set_bits(uintptr_t mask)   { CSRS(CSR_SENVCFG, mask); }
void senvcfg_clear_bits(uintptr_t mask) { CSRC(CSR_SENVCFG, mask); }
void henvcfg_set_bits(uintptr_t mask)   { CSRS(CSR_HENVCFG, mask); }
void henvcfg_clear_bits(uintptr_t mask) { CSRC(CSR_HENVCFG, mask); }
void hvip_set_bits(uintptr_t mask)      { CSRS(CSR_HVIP, mask); }
void hvip_clear_bits(uintptr_t mask)    { CSRC(CSR_HVIP, mask); }
void hstatus_set_bits(uintptr_t mask)   { CSRS(CSR_HSTATUS, mask); }
void hstatus_clear_bits(uintptr_t mask) { CSRC(CSR_HSTATUS, mask); }
void vsstatus_set_bits(uintptr_t mask)   { CSRS(CSR_VSSTATUS, mask); }
void vsstatus_clear_bits(uintptr_t mask) { CSRC(CSR_VSSTATUS, mask); }

/* ===================================================================
 * Sstc timer CSRs and hip / htimedelta read accessors
 *
 * stimecmp (0x14D) / vstimecmp (0x24D) / time (0xC01); hip (0x644) and
 * htimedelta (0x605) read side (htimedelta_write already exists above).
 * These may be called from VS-mode trampolines: the underlying csrr is
 * a single instruction inside this identity-mapped code, so trap-arm /
 * resume behaviour is identical to a suite-local inline copy.
 * =================================================================== */

uintptr_t stimecmp_read(void)         { return CSRR(CSR_STIMECMP); }
void      stimecmp_write(uintptr_t v) { CSRW(CSR_STIMECMP, v); }
uintptr_t vstimecmp_read(void)        { return CSRR(CSR_VSTIMECMP); }
void      vstimecmp_write(uintptr_t v){ CSRW(CSR_VSTIMECMP, v); }
uintptr_t time_read(void)             { return CSRR(CSR_TIME); }
uintptr_t hip_read(void)              { return CSRR(CSR_HIP); }
uintptr_t htimedelta_read(void)       { return CSRR(CSR_HTIMEDELTA); }
