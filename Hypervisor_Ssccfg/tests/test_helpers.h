/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor x Ssccfg test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 *
 * See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 11.
 */

#ifndef HYPERVISOR_SSCCFG_TEST_HELPERS_H
#define HYPERVISOR_SSCCFG_TEST_HELPERS_H

#include "test_framework.h"

#ifdef ENABLE_HYP
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#endif

/* ===================================================================
 * Field / bit definitions
 * =================================================================== */

/* menvcfg.CDE bit (Counter Delegation Enable, bit 12) */
#define MENVCFG_CDE         (1ULL << 12)

/* LCOFI interrupt bit (bit 13 of mip/mie/hvip/hvien/vsie/vsip) */
#define LCOFI_BIT           (1ULL << 13)

/* mstateen0/hstateen0 CSRIND bit (bit 60) - STATEEN0_CSRIND is
 * provided by common/encoding.h. */

/* siselect range reserved for delegated counters */
#define SISELECT_DELEG_BASE 0x40
#define SISELECT_DELEG_END  0x5F

/* ===================================================================
 * CSR access helpers (inline asm, independent of csr_accessors.c)
 * =================================================================== */

static inline uintptr_t siselect_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_SISELECT) : "=r"(v) :: "memory");
    return v;
}

static inline void siselect_write(uintptr_t v)
{
    asm volatile("csrw " CSR_STR(CSR_SISELECT) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_SIREG) : "=r"(v) :: "memory");
    return v;
}

static inline uintptr_t vsiselect_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_VSISELECT) : "=r"(v) :: "memory");
    return v;
}

static inline void vsiselect_write(uintptr_t v)
{
    asm volatile("csrw " CSR_STR(CSR_VSISELECT) ", %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_VSIREG) : "=r"(v) :: "memory");
    return v;
}

/* scountovf (0xDA0, Sscofpmf) - read-only */
static inline uintptr_t scountovf_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_SCOUNTOVF) : "=r"(v) :: "memory");
    return v;
}

/* scountinhibit (0x120, Ssccfg) */
static inline uintptr_t scountinhibit_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_SCOUNTINHIBIT) : "=r"(v) :: "memory");
    return v;
}

static inline void scountinhibit_write(uintptr_t v)
{
    asm volatile("csrw " CSR_STR(CSR_SCOUNTINHIBIT) ", %0" :: "r"(v) : "memory");
}

/* hvip (0x645) is provided by common/hyp/hyp_csr.h (hvip_read /
 * hvip_write). hvien (0x648) has no framework wrapper yet. */
#ifndef CSR_HVIEN
#define CSR_HVIEN 0x648
#endif

static inline uintptr_t hvien_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_HVIEN) : "=r"(v) :: "memory");
    return v;
}

static inline void hvien_write(uintptr_t v)
{
    asm volatile("csrw " CSR_STR(CSR_HVIEN) ", %0" :: "r"(v) : "memory");
}

/* vsie (0x204) / vsip (0x244) are provided by common/hyp/hyp_csr.h
 * (vsie_read / vsie_write / vsip_read). */

/* mstateen0 (0x30C) / hstateen0 (0x60C) */
static inline uintptr_t mstateen0_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_MSTATEEN0) : "=r"(v) :: "memory");
    return v;
}

static inline void mstateen0_set(uintptr_t bits)
{
    asm volatile("csrs " CSR_STR(CSR_MSTATEEN0) ", %0" :: "r"(bits) : "memory");
}

static inline uintptr_t hstateen0_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, " CSR_STR(CSR_HSTATEEN0) : "=r"(v) :: "memory");
    return v;
}

static inline void hstateen0_write(uintptr_t v)
{
    asm volatile("csrw " CSR_STR(CSR_HSTATEEN0) ", %0" :: "r"(v) : "memory");
}

static inline void hstateen0_set(uintptr_t bits)
{
    asm volatile("csrs " CSR_STR(CSR_HSTATEEN0) ", %0" :: "r"(bits) : "memory");
}

static inline void hstateen0_clear(uintptr_t bits)
{
    asm volatile("csrc " CSR_STR(CSR_HSTATEEN0) ", %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * Platform detection
 * =================================================================== */

/* Check if H extension is present (misa.H) */
#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

/*
 * Check whether menvcfg.CDE can actually be set to 1.
 * The virtualization clauses under test (norm:ssccfg_virtual_scountovf_vs_vu,
 * norm:ssccfg_illegal_scountinhibit_vs_vu) only apply with CDE=1, so tests
 * must skip when CDE is read-only zero.
 */
static inline bool cde_settable(void)
{
    uintptr_t orig = menvcfg_read();
    trap_expect_begin();
    menvcfg_write(orig | MENVCFG_CDE);
    bool trapped = trap_was_triggered();
    uintptr_t rb = trapped ? 0 : menvcfg_read();
    trap_expect_end();
    menvcfg_write(orig);
    return !trapped && ((rb & MENVCFG_CDE) != 0);
}

/* Check if Sscofpmf is implemented (scountovf readable from M-mode) */
static inline bool platform_has_sscofpmf(void)
{
    trap_expect_begin();
    scountovf_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/* Check if Smaia/Ssaia is implemented (hvien accessible) */
static inline bool platform_has_hvien(void)
{
    trap_expect_begin();
    hvien_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/* Check if Smstateen is implemented (mstateen0 accessible) */
static inline bool platform_has_smstateen(void)
{
    trap_expect_begin();
    mstateen0_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/*
 * Lift mstateen0[60]/hstateen0[60] (CSRIND) gating when Smstateen is
 * implemented, so that the rules under test (not the state-enable
 * gates) are what actually get exercised. No-op without Smstateen.
 */
static inline void stateen_allow_csrind(void)
{
    if (platform_has_smstateen()) {
        mstateen0_set(STATEEN0_CSRIND);
        hstateen0_set(STATEEN0_CSRIND);
    }
}

/* ===================================================================
 * VS/VU-mode trampoline functions (for run_in_vs_mode / run_in_vu_mode)
 * =================================================================== */

#ifdef ENABLE_HYP

/* VS-mode: read scountovf (should trap with virtual-inst when CDE=1) */
static uintptr_t _vs_read_scountovf(uintptr_t arg)
{
    (void)arg;
    return scountovf_read();
}

/* VU-mode: read scountovf (should trap with virtual-inst when CDE=1) */
static uintptr_t _vu_read_scountovf(uintptr_t arg)
{
    (void)arg;
    return scountovf_read();
}

/* VS-mode: read scountinhibit (should trap with virtual-inst when CDE=1) */
static uintptr_t _vs_read_scountinhibit(uintptr_t arg)
{
    (void)arg;
    return scountinhibit_read();
}

/* VU-mode: read scountinhibit (should trap with virtual-inst when CDE=1) */
static uintptr_t _vu_read_scountinhibit(uintptr_t arg)
{
    (void)arg;
    return scountinhibit_read();
}

/* VS-mode: directly access vsiselect (should trap with virtual-inst) */
static uintptr_t _vs_read_vsiselect_direct(uintptr_t arg)
{
    (void)arg;
    return vsiselect_read();
}

/* VS-mode: directly access vsireg (should trap with virtual-inst) */
static uintptr_t _vs_read_vsireg_direct(uintptr_t arg)
{
    (void)arg;
    return vsireg_read();
}

/* VU-mode: directly access vsiselect (should trap with virtual-inst) */
static uintptr_t _vu_read_vsiselect_direct(uintptr_t arg)
{
    (void)arg;
    return vsiselect_read();
}

/* VU-mode: directly access vsireg (should trap with virtual-inst) */
static uintptr_t _vu_read_vsireg_direct(uintptr_t arg)
{
    (void)arg;
    return vsireg_read();
}

/* VU-mode: access siselect (should trap with virtual-inst) */
static uintptr_t _vu_read_siselect(uintptr_t arg)
{
    (void)arg;
    return siselect_read();
}

/* VU-mode: access sireg (should trap with virtual-inst) */
static uintptr_t _vu_read_sireg(uintptr_t arg)
{
    (void)arg;
    return sireg_read();
}

/* VS-mode: read sireg (remapped to vsireg) */
static uintptr_t _vs_read_sireg(uintptr_t arg)
{
    (void)arg;
    return sireg_read();
}

/* VS-mode: set siselect to delegated range, then read sireg
 * (really vsireg) - trap type depends on menvcfg.CDE. */
static uintptr_t _vs_read_sireg_deleg(uintptr_t arg)
{
    siselect_write(arg);
    return sireg_read();
}

/* VS-mode: write siselect (remapped to vsiselect), write-only so that
 * a state-enable gate trap is attributed to a single access. */
static uintptr_t _vs_write_siselect(uintptr_t arg)
{
    siselect_write(arg);
    return 0;
}

/* VS-mode: write siselect with a non-delegated value (remapped to
 * vsiselect), then read it back for verification. */
static uintptr_t _vs_write_read_siselect(uintptr_t arg)
{
    siselect_write(arg);
    return siselect_read();
}

/* Execute fn in VS-mode and verify it traps with expected_cause.
 * The trap is delivered after the trampoline ecall round-trip returns
 * to M-mode, so the expect window must stay armed across
 * run_in_vs_mode() and only be disarmed after inspection. */
#define TEST_VS_MODE_TRAP(desc, fn, arg, expected_cause) do { \
    trap_expect_begin(); \
    run_in_vs_mode(fn, arg); \
    bool _vs_trapped = trap_was_triggered(); \
    uintptr_t _vs_cause = _vs_trapped ? trap_get_cause() : 0; \
    trap_expect_end(); \
    TEST_ASSERT(desc " (should trap)", _vs_trapped); \
    if (_vs_trapped) { \
        TEST_ASSERT_EQ(desc, _vs_cause, (uintptr_t)(expected_cause)); \
    } \
} while (0)

/* Execute fn in VU-mode and verify it traps with expected_cause */
#define TEST_VU_MODE_TRAP(desc, fn, arg, expected_cause) do { \
    trap_expect_begin(); \
    run_in_vu_mode(fn, arg); \
    bool _vu_trapped = trap_was_triggered(); \
    uintptr_t _vu_cause = _vu_trapped ? trap_get_cause() : 0; \
    trap_expect_end(); \
    TEST_ASSERT(desc " (should trap)", _vu_trapped); \
    if (_vu_trapped) { \
        TEST_ASSERT_EQ(desc, _vu_cause, (uintptr_t)(expected_cause)); \
    } \
} while (0)

/* Same as TEST_VU_MODE_TRAP but accepts either cause (2 or 22).
 * Used for VU-mode access to S-level CSRs, where the Ssccfg clause
 * ("V=1" wording implies virtual-instruction) conflicts with the base
 * CSR access rule (U-mode accessing an S-level CSR raises
 * illegal-instruction). Consistent with SRMCFG-21 handling in
 * Hypervisor_Ss_test_plan.md Group 9. */
#define TEST_VU_MODE_TRAP_ILLEGAL_OR_VIRTUAL(desc, fn, arg) do { \
    trap_expect_begin(); \
    run_in_vu_mode(fn, arg); \
    bool _vu_trapped = trap_was_triggered(); \
    uintptr_t _vu_cause = _vu_trapped ? trap_get_cause() : 0; \
    trap_expect_end(); \
    TEST_ASSERT(desc " (should trap)", _vu_trapped); \
    if (_vu_trapped) { \
        TEST_ASSERT(desc " (cause 2 or 22)", \
                    _vu_cause == CAUSE_ILLEGAL_INST || \
                    _vu_cause == CAUSE_VIRTUAL_INSTRUCTION); \
    } \
} while (0)

#endif /* ENABLE_HYP */

#endif /* HYPERVISOR_SSCCFG_TEST_HELPERS_H */
