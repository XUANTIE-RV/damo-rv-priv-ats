/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/hyp_csr.c — Hypervisor CSR accessors (G-stage subset)
 * =================================================================== */

#include "hyp_csr.h"
#include "encoding.h"
#include "test_framework.h"   /* PRIV_S / PRIV_U / PRIV_M */

/* ===================================================================
 * hgatp
 * =================================================================== */

uintptr_t hgatp_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x680" : "=r"(v) :: "memory");
    return v;
}

/* Raw write: write the value untouched. Reserved for tests that
 * deliberately probe WARL behaviour (see hgatp_write_raw doc in
 * hyp_csr.h). */
void hgatp_write_raw(uintptr_t value) {
    asm volatile ("csrw 0x680, %0" :: "r"(value) : "memory");
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
    uintptr_t mode = (value >> HGATP64_MODE_SHIFT) & 0xFUL;
    if (mode == HGATP_MODE_BARE) {
        value = 0;
    } else if (mode == HGATP_MODE_SV39X4 ||
               mode == HGATP_MODE_SV48X4 ||
               mode == HGATP_MODE_SV57X4) {
        value &= ~((uintptr_t)0x3UL);
    }
    asm volatile ("csrw 0x680, %0" :: "r"(value) : "memory");
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
    uintptr_t v;
    asm volatile ("csrr %0, 0x600" : "=r"(v) :: "memory");
    return v;
}

void hstatus_write(uintptr_t value) {
    asm volatile ("csrw 0x600, %0" :: "r"(value) : "memory");
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
    asm volatile ("csrw 0x602, %0" :: "r"(value) : "memory");
}
uintptr_t hedeleg_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x602" : "=r"(v) :: "memory");
    return v;
}

void hideleg_write(uintptr_t value) {
    asm volatile ("csrw 0x603, %0" :: "r"(value) : "memory");
}
uintptr_t hideleg_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x603" : "=r"(v) :: "memory");
    return v;
}

/* ===================================================================
 * henvcfg / hcounteren / htimedelta
 * =================================================================== */

void henvcfg_write(uintptr_t value) {
    asm volatile ("csrw 0x60A, %0" :: "r"(value) : "memory");
}
uintptr_t henvcfg_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x60A" : "=r"(v) :: "memory");
    return v;
}

void hcounteren_write(uintptr_t value) {
    asm volatile ("csrw 0x606, %0" :: "r"(value) : "memory");
}

void htimedelta_write(uintptr_t value) {
    asm volatile ("csrw 0x605, %0" :: "r"(value) : "memory");
}

/* ===================================================================
 * menvcfg (M-mode CSR 0x30A)
 *
 * Defined here (rather than in a separate machine_csr.c) because the
 * GAD diagnostics in sv39x4 are the only current consumer, and they
 * already pull in hyp_csr for henvcfg / hgatp.
 * =================================================================== */

uintptr_t menvcfg_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x30A" : "=r"(v) :: "memory");
    return v;
}

void menvcfg_write(uintptr_t value) {
    asm volatile ("csrw 0x30A, %0" :: "r"(value) : "memory");
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
     * Read current medeleg and OR in the requested bits. */
    uintptr_t medeleg;
    uintptr_t mideleg;
    asm volatile ("csrr %0, 0x302" : "=r"(medeleg) :: "memory"); /* medeleg */
    asm volatile ("csrr %0, 0x303" : "=r"(mideleg) :: "memory"); /* mideleg */
    medeleg |= hedeleg_mask;
    mideleg |= hideleg_mask;
    asm volatile ("csrw 0x302, %0" :: "r"(medeleg) : "memory");
    asm volatile ("csrw 0x303, %0" :: "r"(mideleg) : "memory");

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
    uintptr_t v;
    asm volatile ("csrr %0, 0x645" : "=r"(v) :: "memory");
    return v;
}

void hvip_write(uintptr_t value) {
    asm volatile ("csrw 0x645, %0" :: "r"(value) : "memory");
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
    uintptr_t v;
    asm volatile ("csrr %0, 0x606" : "=r"(v) :: "memory");
    return v;
}

void hcounteren_set(uint32_t mask) {
    uintptr_t v = hcounteren_read();
    v |= (uintptr_t)mask;
    hcounteren_write(v);
}

void hcounteren_clear(uint32_t mask) {
    uintptr_t v = hcounteren_read();
    v &= ~(uintptr_t)mask;
    hcounteren_write(v);
}

void htimedelta_set(uint64_t delta) {
    htimedelta_write((uintptr_t)delta);
}

/* ===================================================================
 * mcounteren / scounteren
 * =================================================================== */

uintptr_t mcounteren_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x306" : "=r"(v) :: "memory");
    return v;
}

void mcounteren_write(uintptr_t value) {
    asm volatile ("csrw 0x306, %0" :: "r"(value) : "memory");
}

void mcounteren_set(uint32_t mask) {
    uintptr_t v = mcounteren_read();
    mcounteren_write(v | (uintptr_t)mask);
}

void mcounteren_clear(uint32_t mask) {
    uintptr_t v = mcounteren_read();
    mcounteren_write(v & ~(uintptr_t)mask);
}

uintptr_t scounteren_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x106" : "=r"(v) :: "memory");
    return v;
}

void scounteren_write(uintptr_t value) {
    asm volatile ("csrw 0x106, %0" :: "r"(value) : "memory");
}

void scounteren_set(uint32_t mask) {
    uintptr_t v = scounteren_read();
    scounteren_write(v | (uintptr_t)mask);
}

void scounteren_clear(uint32_t mask) {
    uintptr_t v = scounteren_read();
    scounteren_write(v & ~(uintptr_t)mask);
}

/* ===================================================================
 * vstvec (CSR 0x205)
 * =================================================================== */

uintptr_t vstvec_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x205" : "=r"(v) :: "memory");
    return v;
}

void vstvec_write(uintptr_t value) {
    asm volatile ("csrw 0x205, %0" :: "r"(value) : "memory");
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
    uintptr_t v;
    asm volatile ("csrr %0, 0x243" : "=r"(v) :: "memory");
    return v;
}

void vstval_write(uintptr_t value) {
    asm volatile ("csrw 0x243, %0" :: "r"(value) : "memory");
}

/* ===================================================================
 * vsie (CSR 0x204) / vsip (CSR 0x244)
 * =================================================================== */

uintptr_t vsie_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x204" : "=r"(v) :: "memory");
    return v;
}

void vsie_write(uintptr_t value) {
    asm volatile ("csrw 0x204, %0" :: "r"(value) : "memory");
}

uintptr_t vsip_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x244" : "=r"(v) :: "memory");
    return v;
}

void vsip_write(uintptr_t value) {
    asm volatile ("csrw 0x244, %0" :: "r"(value) : "memory");
}

/* ===================================================================
 * vsatp (CSR 0x280)
 * =================================================================== */

uintptr_t vsatp_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x280" : "=r"(v) :: "memory");
    return v;
}

void vsatp_write(uintptr_t value) {
    asm volatile ("csrw 0x280, %0" :: "r"(value) : "memory");
}

void vsatp_write_raw(uintptr_t value) {
    asm volatile ("csrw 0x280, %0" :: "r"(value) : "memory");
}

/* ===================================================================
 * satp (CSR 0x180)
 * =================================================================== */

uintptr_t satp_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x180" : "=r"(v) :: "memory");
    return v;
}

void satp_write(uintptr_t value) {
    asm volatile ("csrw 0x180, %0" :: "r"(value) : "memory");
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
 * Counter implementation detection
 *
 * Strategy: for each counter bit in mcounteren, write the bit, read
 * back. If it sticks, the counter index is considered implemented.
 * This is a conservative probe - some implementations may have
 * writable mcounteren bits without implementing the actual counter.
 * =================================================================== */

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

bool hpmcounter_is_writable(int idx) {
    if (idx < 3 || idx > 31) return false;
    return (counteren_probe_implemented() & (1U << idx)) != 0;
}

/* ===================================================================
 * Smstateen / Ssstateen CSR accessors
 * =================================================================== */

uintptr_t mstateen_read(int idx) {
    uintptr_t v = 0;
    switch (idx) {
    case 0: asm volatile ("csrr %0, 0x30C" : "=r"(v) :: "memory"); break;
    case 1: asm volatile ("csrr %0, 0x30D" : "=r"(v) :: "memory"); break;
    case 2: asm volatile ("csrr %0, 0x30E" : "=r"(v) :: "memory"); break;
    case 3: asm volatile ("csrr %0, 0x30F" : "=r"(v) :: "memory"); break;
    default: break;
    }
    return v;
}

void mstateen_write(int idx, uintptr_t value) {
    switch (idx) {
    case 0: asm volatile ("csrw 0x30C, %0" :: "r"(value) : "memory"); break;
    case 1: asm volatile ("csrw 0x30D, %0" :: "r"(value) : "memory"); break;
    case 2: asm volatile ("csrw 0x30E, %0" :: "r"(value) : "memory"); break;
    case 3: asm volatile ("csrw 0x30F, %0" :: "r"(value) : "memory"); break;
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
    case 0: asm volatile ("csrr %0, 0x60C" : "=r"(v) :: "memory"); break;
    case 1: asm volatile ("csrr %0, 0x60D" : "=r"(v) :: "memory"); break;
    case 2: asm volatile ("csrr %0, 0x60E" : "=r"(v) :: "memory"); break;
    case 3: asm volatile ("csrr %0, 0x60F" : "=r"(v) :: "memory"); break;
    default: break;
    }
    return v;
}

void hstateen_write(int idx, uintptr_t value) {
    switch (idx) {
    case 0: asm volatile ("csrw 0x60C, %0" :: "r"(value) : "memory"); break;
    case 1: asm volatile ("csrw 0x60D, %0" :: "r"(value) : "memory"); break;
    case 2: asm volatile ("csrw 0x60E, %0" :: "r"(value) : "memory"); break;
    case 3: asm volatile ("csrw 0x60F, %0" :: "r"(value) : "memory"); break;
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
    case 0: asm volatile ("csrr %0, 0x10C" : "=r"(v) :: "memory"); break;
    case 1: asm volatile ("csrr %0, 0x10D" : "=r"(v) :: "memory"); break;
    case 2: asm volatile ("csrr %0, 0x10E" : "=r"(v) :: "memory"); break;
    case 3: asm volatile ("csrr %0, 0x10F" : "=r"(v) :: "memory"); break;
    default: break;
    }
    return v;
}

void sstateen_write(int idx, uintptr_t value) {
    switch (idx) {
    case 0: asm volatile ("csrw 0x10C, %0" :: "r"(value) : "memory"); break;
    case 1: asm volatile ("csrw 0x10D, %0" :: "r"(value) : "memory"); break;
    case 2: asm volatile ("csrw 0x10E, %0" :: "r"(value) : "memory"); break;
    case 3: asm volatile ("csrw 0x10F, %0" :: "r"(value) : "memory"); break;
    default: break;
    }
}

void sstateen_set_bits(int idx, uintptr_t mask) {
    sstateen_write(idx, sstateen_read(idx) | mask);
}

void sstateen_clear_bits(int idx, uintptr_t mask) {
    sstateen_write(idx, sstateen_read(idx) & ~mask);
}

bool stateen_is_available(void) {
    uintptr_t saved = mstateen_read(0);
    mstateen_write(0, ~(uintptr_t)0);
    uintptr_t readback = mstateen_read(0);
    mstateen_write(0, saved);
    return (readback != 0);
}
