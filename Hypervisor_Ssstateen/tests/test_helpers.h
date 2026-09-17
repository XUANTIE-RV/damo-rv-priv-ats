/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Ssstateen test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * Extracted from Ssstateen/tests/test_helpers.h, keeping only the
 * H-mode (Groups 7-12) relevant helpers.
 */

#ifndef HYPERVISOR_SSSTATEEN_TEST_HELPERS_H
#define HYPERVISOR_SSSTATEEN_TEST_HELPERS_H

#include "test_framework.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_trap.h"

/* ===================================================================
 * Probe helper: check if a bit of hstateen0 is writable.
 * =================================================================== */

static inline bool hstateen0_bit_writable(uintptr_t bit)
{
    uintptr_t saved = hstateen_read(0);
    hstateen_set_bits(0, bit);
    uintptr_t rb = hstateen_read(0);
    hstateen_write(0, saved);
    return (rb & bit) != 0;
}

static inline bool mstateen0_bit_writable(uintptr_t bit)
{
    uintptr_t saved = mstateen_read(0);
    mstateen_set_bits(0, bit);
    uintptr_t rb = mstateen_read(0);
    mstateen_write(0, saved);
    return (rb & bit) != 0;
}

/* ===================================================================
 * GEILEN detection via hgeie writability.
 *
 * Per norm:geilen: "if GEILEN is nonzero, bits GEILEN:1 shall be
 * writable in hgeie, and all other bit positions shall be read-only
 * zeros in both hgeip and hgeie". This is the SPEC-reliable way to
 * detect guest interrupt files.
 *
 * Note: hstatus.VGEIN write/readback is NOT a reliable probe. VGEIN
 * is WLRL; when GEILEN=0 the SPEC only says VGEIN "may be read-only
 * zero", so an implementation may legally retain a written non-zero
 * VGEIN while no guest interrupt file exists.
 * =================================================================== */

static inline bool geilen_nonzero(void)
{
    uintptr_t saved;
    asm volatile("csrr %0, " CSR_STR(CSR_HGEIE) : "=r"(saved));
    asm volatile("csrw " CSR_STR(CSR_HGEIE) ", %0" :: "r"(~0UL));
    uintptr_t rb;
    asm volatile("csrr %0, " CSR_STR(CSR_HGEIE) : "=r"(rb));
    asm volatile("csrw " CSR_STR(CSR_HGEIE) ", %0" :: "r"(saved));
    return (rb & ~1UL) != 0;
}

/* ===================================================================
 * VS-mode callback functions (invoked via run_in_vs_mode)
 * =================================================================== */

static uintptr_t _vs_read_sstateen0(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(CSR_SSTATEEN0) : "=r"(val));
    return val;
}

static uintptr_t _vs_write_sstateen0(uintptr_t arg)
{
    asm volatile("csrw " CSR_STR(CSR_SSTATEEN0) ", %0" :: "r"(arg));
    return 0;
}

static uintptr_t _vs_read_sstateen(uintptr_t idx)
{
    uintptr_t val;
    switch ((int)idx)
    {
    case 0: asm volatile("csrr %0, " CSR_STR(CSR_SSTATEEN0) : "=r"(val)); break;
    case 1: asm volatile("csrr %0, " CSR_STR(CSR_SSTATEEN1) : "=r"(val)); break;
    case 2: asm volatile("csrr %0, " CSR_STR(CSR_SSTATEEN2) : "=r"(val)); break;
    case 3: asm volatile("csrr %0, " CSR_STR(CSR_SSTATEEN3) : "=r"(val)); break;
    default: val = 0; break;
    }
    return val;
}

static uintptr_t _vs_read_senvcfg(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(CSR_SENVCFG) : "=r"(val));
    return val;
}

static uintptr_t _vs_write_senvcfg(uintptr_t arg)
{
    asm volatile("csrw " CSR_STR(CSR_SENVCFG) ", %0" :: "r"(arg));
    return 0;
}

/* siselect (CSR 0x150) */
static uintptr_t _vs_read_siselect(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(CSR_SISELECT) : "=r"(val));
    return val;
}

/* sireg (CSR 0x151) */
static uintptr_t _vs_read_sireg(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(CSR_SIREG) : "=r"(val));
    return val;
}

/* stopei (CSR 0x15C) */
static uintptr_t _vs_read_stopei(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(CSR_STOPEI) : "=r"(val));
    return val;
}

/* scontext (CSR 0x5A8) */
static uintptr_t _vs_read_scontext(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(CSR_SCONTEXT) : "=r"(val));
    return val;
}

static uintptr_t _vs_write_scontext(uintptr_t arg)
{
    asm volatile("csrw " CSR_STR(CSR_SCONTEXT) ", %0" :: "r"(arg));
    return 0;
}

#if __riscv_xlen == 32
/* sieh (CSR 0x154) - RV32 Ssaia: upper 32 bits of sie */
static uintptr_t _vs_read_sieh(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(CSR_SIPH) : "=r"(val));
    return val;
}

/* siph (CSR 0x155) - RV32 Ssaia: upper 32 bits of sip */
static uintptr_t _vs_read_siph(uintptr_t arg)
{
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(CSR_SIREG4) : "=r"(val));
    return val;
}
#endif /* __riscv_xlen == 32 */

#endif /* HYPERVISOR_SSSTATEEN_TEST_HELPERS_H */
