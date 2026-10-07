/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * csr_ops.h - M/S-level named CSR accessors (single authoritative source)
 *
 * Named, compile-time CSR accessors for the machine- and supervisor-
 * level registers that do NOT depend on the H extension. Each wraps a
 * fixed CSR number and expands to a single csrr/csrw/csrs/csrc.
 *
 * This header is the single source of truth for these accessors:
 *   - non-HYP suites (Sm / Ss families) include it directly, because
 *     common/hyp/hyp_csr.o is linked only under ENABLE_HYP and is thus
 *     unreachable to them;
 *   - common/hyp/hyp_csr.h includes it, so HYP builds resolve the same
 *     names from here (hyp_csr.c no longer defines them).
 *
 * Scope boundary (M/S only): do NOT add H-extension CSRs
 * (henvcfg, hstatus, hvip, vs-registers, hgatp, ...) or logic beyond
 * plain read/write/set/clear here; those stay in hyp_csr. Trap-armed
 * probing variants remain suite-local.
 *
 * For accessing a CSR whose number is only known at runtime, use the
 * generic dispatcher csr_read()/csr_write() from csr_accessors.h instead.
 */

#ifndef COMMON_CSR_OPS_H
#define COMMON_CSR_OPS_H

#include <stdbool.h>
#include "types.h"
#include "encoding.h"

/* ===================================================================
 * menvcfg (CSR 0x30A)
 * =================================================================== */
static inline uintptr_t menvcfg_read(void) {
    return CSRR(CSR_MENVCFG);
}

static inline void menvcfg_write(uintptr_t value) {
    CSRW(CSR_MENVCFG, value);
}

static inline void menvcfg_set_bits(uintptr_t mask) {
    CSRS(CSR_MENVCFG, mask);
}

static inline void menvcfg_clear_bits(uintptr_t mask) {
    CSRC(CSR_MENVCFG, mask);
}

/* ===================================================================
 * senvcfg (CSR 0x10A)
 * =================================================================== */
static inline uintptr_t senvcfg_read(void) {
    return CSRR(CSR_SENVCFG);
}

static inline void senvcfg_write(uintptr_t value) {
    CSRW(CSR_SENVCFG, value);
}

static inline void senvcfg_set_bits(uintptr_t mask) {
    CSRS(CSR_SENVCFG, mask);
}

static inline void senvcfg_clear_bits(uintptr_t mask) {
    CSRC(CSR_SENVCFG, mask);
}

/* ===================================================================
 * mcounteren (CSR 0x306)
 * =================================================================== */
static inline uintptr_t mcounteren_read(void) {
    return CSRR(CSR_MCOUNTEREN);
}

static inline void mcounteren_write(uintptr_t value) {
    CSRW(CSR_MCOUNTEREN, value);
}

static inline void mcounteren_set(uintptr_t mask) {
    CSRS(CSR_MCOUNTEREN, mask);
}

static inline void mcounteren_clear(uintptr_t mask) {
    CSRC(CSR_MCOUNTEREN, mask);
}

/* ===================================================================
 * scounteren (CSR 0x106)
 * =================================================================== */
static inline uintptr_t scounteren_read(void) {
    return CSRR(CSR_SCOUNTEREN);
}

static inline void scounteren_write(uintptr_t value) {
    CSRW(CSR_SCOUNTEREN, value);
}

static inline void scounteren_set(uintptr_t mask) {
    CSRS(CSR_SCOUNTEREN, mask);
}

static inline void scounteren_clear(uintptr_t mask) {
    CSRC(CSR_SCOUNTEREN, mask);
}

/* ===================================================================
 * mstatus (CSR 0x300) and trap CSRs
 * =================================================================== */
static inline uintptr_t mstatus_read(void) {
    return CSRR(mstatus);
}

static inline void mstatus_write(uintptr_t value) {
    CSRW(mstatus, value);
}

static inline void mstatus_set(uintptr_t mask) {
    CSRS(mstatus, mask);
}

static inline void mstatus_clear(uintptr_t mask) {
    CSRC(mstatus, mask);
}

/* ===================================================================
 * Smstateen: mstateen0-3 (0x30C-0x30F), hstateen0-3 (0x60C-0x60F),
 * sstateen0-3 (0x10C-0x10F)
 * =================================================================== */
static inline uintptr_t mstateen_read(int idx) {
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

static inline void mstateen_write(int idx, uintptr_t value) {
    switch (idx) {
    case 0: CSRW(CSR_MSTATEEN0, value); break;
    case 1: CSRW(CSR_MSTATEEN1, value); break;
    case 2: CSRW(CSR_MSTATEEN2, value); break;
    case 3: CSRW(CSR_MSTATEEN3, value); break;
    default: break;
    }
}

static inline void mstateen_set_bits(int idx, uintptr_t mask) {
    mstateen_write(idx, mstateen_read(idx) | mask);
}

static inline void mstateen_clear_bits(int idx, uintptr_t mask) {
    mstateen_write(idx, mstateen_read(idx) & ~mask);
}

static inline uintptr_t hstateen_read(int idx) {
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

static inline void hstateen_write(int idx, uintptr_t value) {
    switch (idx) {
    case 0: CSRW(CSR_HSTATEEN0, value); break;
    case 1: CSRW(CSR_HSTATEEN1, value); break;
    case 2: CSRW(CSR_HSTATEEN2, value); break;
    case 3: CSRW(CSR_HSTATEEN3, value); break;
    default: break;
    }
}

static inline void hstateen_set_bits(int idx, uintptr_t mask) {
    hstateen_write(idx, hstateen_read(idx) | mask);
}

static inline void hstateen_clear_bits(int idx, uintptr_t mask) {
    hstateen_write(idx, hstateen_read(idx) & ~mask);
}

static inline uintptr_t sstateen_read(int idx) {
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

static inline void sstateen_write(int idx, uintptr_t value) {
    switch (idx) {
    case 0: CSRW(CSR_SSTATEEN0, value); break;
    case 1: CSRW(CSR_SSTATEEN1, value); break;
    case 2: CSRW(CSR_SSTATEEN2, value); break;
    case 3: CSRW(CSR_SSTATEEN3, value); break;
    default: break;
    }
}

static inline void sstateen_set_bits(int idx, uintptr_t mask) {
    sstateen_write(idx, sstateen_read(idx) | mask);
}

static inline void sstateen_clear_bits(int idx, uintptr_t mask) {
    sstateen_write(idx, sstateen_read(idx) & ~mask);
}

/* ===================================================================
 * Ssqosid: srmcfg (CSR 0x181)
 * =================================================================== */
static inline uintptr_t srmcfg_read(void) { return CSRR(CSR_SRMCFG); }
static inline void      srmcfg_write(uintptr_t v) { CSRW(CSR_SRMCFG, v); }

/* ===================================================================
 * Indirect CSR windows (Smcsrind / Sscsrind / Ssccfg)
 * =================================================================== */
static inline uintptr_t miselect_read(void) { return CSRR(CSR_MISELECT); }
static inline void      miselect_write(uintptr_t v) { CSRW(CSR_MISELECT, v); }
static inline uintptr_t mireg_read(void) { return CSRR(CSR_MIREG); }
static inline void      mireg_write(uintptr_t v) { CSRW(CSR_MIREG, v); }
static inline uintptr_t mireg2_read(void) { return CSRR(CSR_MIREG2); }
static inline void      mireg2_write(uintptr_t v) { CSRW(CSR_MIREG2, v); }
static inline uintptr_t mireg3_read(void) { return CSRR(CSR_MIREG3); }
static inline void      mireg3_write(uintptr_t v) { CSRW(CSR_MIREG3, v); }
static inline uintptr_t mireg4_read(void) { return CSRR(CSR_MIREG4); }
static inline void      mireg4_write(uintptr_t v) { CSRW(CSR_MIREG4, v); }
static inline uintptr_t mireg5_read(void) { return CSRR(CSR_MIREG5); }
static inline void      mireg5_write(uintptr_t v) { CSRW(CSR_MIREG5, v); }
static inline uintptr_t mireg6_read(void) { return CSRR(CSR_MIREG6); }
static inline void      mireg6_write(uintptr_t v) { CSRW(CSR_MIREG6, v); }

static inline uintptr_t siselect_read(void) { return CSRR(CSR_SISELECT); }
static inline void      siselect_write(uintptr_t v) { CSRW(CSR_SISELECT, v); }
static inline uintptr_t sireg_read(void) { return CSRR(CSR_SIREG); }
static inline void      sireg_write(uintptr_t v) { CSRW(CSR_SIREG, v); }
static inline uintptr_t sireg2_read(void) { return CSRR(CSR_SIREG2); }
static inline void      sireg2_write(uintptr_t v) { CSRW(CSR_SIREG2, v); }
static inline uintptr_t sireg3_read(void) { return CSRR(CSR_SIREG3); }
static inline void      sireg3_write(uintptr_t v) { CSRW(CSR_SIREG3, v); }
static inline uintptr_t sireg4_read(void) { return CSRR(CSR_SIREG4); }
static inline void      sireg4_write(uintptr_t v) { CSRW(CSR_SIREG4, v); }
static inline uintptr_t sireg5_read(void) { return CSRR(CSR_SIREG5); }
static inline void      sireg5_write(uintptr_t v) { CSRW(CSR_SIREG5, v); }
static inline uintptr_t sireg6_read(void) { return CSRR(CSR_SIREG6); }
static inline void      sireg6_write(uintptr_t v) { CSRW(CSR_SIREG6, v); }

static inline uintptr_t vsiselect_read(void) { return CSRR(CSR_VSISELECT); }
static inline void      vsiselect_write(uintptr_t v) { CSRW(CSR_VSISELECT, v); }
static inline uintptr_t vsireg_read(void) { return CSRR(CSR_VSIREG); }
static inline void      vsireg_write(uintptr_t v) { CSRW(CSR_VSIREG, v); }
static inline uintptr_t vsireg2_read(void) { return CSRR(CSR_VSIREG2); }
static inline void      vsireg2_write(uintptr_t v) { CSRW(CSR_VSIREG2, v); }
static inline uintptr_t vsireg3_read(void) { return CSRR(CSR_VSIREG3); }
static inline void      vsireg3_write(uintptr_t v) { CSRW(CSR_VSIREG3, v); }
static inline uintptr_t vsireg4_read(void) { return CSRR(CSR_VSIREG4); }
static inline void      vsireg4_write(uintptr_t v) { CSRW(CSR_VSIREG4, v); }
static inline uintptr_t vsireg5_read(void) { return CSRR(CSR_VSIREG5); }
static inline void      vsireg5_write(uintptr_t v) { CSRW(CSR_VSIREG5, v); }
static inline uintptr_t vsireg6_read(void) { return CSRR(CSR_VSIREG6); }
static inline void      vsireg6_write(uintptr_t v) { CSRW(CSR_VSIREG6, v); }

#endif /* COMMON_CSR_OPS_H */
