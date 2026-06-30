/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for the Hypervisor × Sscsrind test suite.
 *
 * All test files are #included into test_register.c, so static
 * functions and globals defined here are visible across the whole
 * compilation unit.
 */

#ifndef HYPERVISOR_SSCSRIND_TEST_HELPERS_H
#define HYPERVISOR_SSCSRIND_TEST_HELPERS_H

#include "test_framework.h"

#ifdef ENABLE_HYP
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#endif

/* ===================================================================
 * S-mode CSR access helpers (siselect/sireg*)
 * =================================================================== */

static inline uintptr_t siselect_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x150" : "=r"(v) :: "memory");
    return v;
}

static inline void siselect_write(uintptr_t v)
{
    asm volatile("csrw 0x150, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x151" : "=r"(v) :: "memory");
    return v;
}

static inline void sireg_write(uintptr_t v)
{
    asm volatile("csrw 0x151, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg2_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x152" : "=r"(v) :: "memory");
    return v;
}

static inline void sireg2_write(uintptr_t v)
{
    asm volatile("csrw 0x152, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg3_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x153" : "=r"(v) :: "memory");
    return v;
}

static inline void sireg3_write(uintptr_t v)
{
    asm volatile("csrw 0x153, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg4_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x155" : "=r"(v) :: "memory");
    return v;
}

static inline void sireg4_write(uintptr_t v)
{
    asm volatile("csrw 0x155, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg5_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x156" : "=r"(v) :: "memory");
    return v;
}

static inline void sireg5_write(uintptr_t v)
{
    asm volatile("csrw 0x156, %0" :: "r"(v) : "memory");
}

static inline uintptr_t sireg6_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x157" : "=r"(v) :: "memory");
    return v;
}

static inline void sireg6_write(uintptr_t v)
{
    asm volatile("csrw 0x157, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * VS-mode CSR access helpers (vsiselect/vsireg*)
 * Accessed from HS-mode or M-mode via CSR addresses 0x250-0x257
 * =================================================================== */

static inline uintptr_t vsiselect_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x250" : "=r"(v) :: "memory");
    return v;
}

static inline void vsiselect_write(uintptr_t v)
{
    asm volatile("csrw 0x250, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x251" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg_write(uintptr_t v)
{
    asm volatile("csrw 0x251, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg2_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x252" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg2_write(uintptr_t v)
{
    asm volatile("csrw 0x252, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg3_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x253" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg3_write(uintptr_t v)
{
    asm volatile("csrw 0x253, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg4_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x255" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg4_write(uintptr_t v)
{
    asm volatile("csrw 0x255, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg5_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x256" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg5_write(uintptr_t v)
{
    asm volatile("csrw 0x256, %0" :: "r"(v) : "memory");
}

static inline uintptr_t vsireg6_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x257" : "=r"(v) :: "memory");
    return v;
}

static inline void vsireg6_write(uintptr_t v)
{
    asm volatile("csrw 0x257, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * M-mode CSR access helpers (miselect/mireg* for setup)
 * =================================================================== */

static inline uintptr_t miselect_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x350" : "=r"(v) :: "memory");
    return v;
}

static inline void miselect_write(uintptr_t v)
{
    asm volatile("csrw 0x350, %0" :: "r"(v) : "memory");
}

static inline uintptr_t mireg_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x351" : "=r"(v) :: "memory");
    return v;
}

static inline void mireg_write(uintptr_t v)
{
    asm volatile("csrw 0x351, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * State-enable CSR helpers (mstateen0/hstateen0)
 * =================================================================== */

static inline uintptr_t mstateen0_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x30C" : "=r"(v) :: "memory");
    return v;
}

static inline void mstateen0_write(uintptr_t v)
{
    asm volatile("csrw 0x30C, %0" :: "r"(v) : "memory");
}

static inline void mstateen0_set(uintptr_t bits)
{
    asm volatile("csrs 0x30C, %0" :: "r"(bits) : "memory");
}

static inline void mstateen0_clear(uintptr_t bits)
{
    asm volatile("csrc 0x30C, %0" :: "r"(bits) : "memory");
}

static inline uintptr_t hstateen0_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x60C" : "=r"(v) :: "memory");
    return v;
}

static inline void hstateen0_write(uintptr_t v)
{
    asm volatile("csrw 0x60C, %0" :: "r"(v) : "memory");
}

static inline void hstateen0_set(uintptr_t bits)
{
    asm volatile("csrs 0x60C, %0" :: "r"(bits) : "memory");
}

static inline void hstateen0_clear(uintptr_t bits)
{
    asm volatile("csrc 0x60C, %0" :: "r"(bits) : "memory");
}

/* mstateen0 CSRIND bit (bit 60) */
#define MSTATEEN0_CSRIND    (1ULL << 60)

/* hstateen0 CSRIND bit (bit 60) */
#define HSTATEEN0_CSRIND    (1ULL << 60)

/*
 * Safely read vsiselect from current privilege (any mode).
 * Returns true if read succeeded, false if trapped.
 * On success, *val holds the read value.
 */
static inline bool vsiselect_read_safe(uintptr_t *val)
{
    trap_expect_begin();
    *val = vsiselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/*
 * Safely write vsiselect from current privilege (any mode).
 * Returns true if write succeeded, false if trapped.
 */
static inline bool vsiselect_write_safe(uintptr_t val)
{
    trap_expect_begin();
    vsiselect_write(val);
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/*
 * Safely write vsireg from current privilege (any mode).
 * Returns true if write succeeded, false if trapped.
 */
static inline bool vsireg_write_safe(uintptr_t val)
{
    trap_expect_begin();
    vsireg_write(val);
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

/* ===================================================================
 * CSR accessibility check from current privilege level
 * =================================================================== */

/*
 * Verify vsiselect is accessible from the current privilege level.
 * Must be called AFTER goto_priv(PRIV_S) and AFTER platform_has_sscsrind().
 * Returns false (and records a FAIL) if vsiselect is not accessible.
 *
 * Usage: if (!vsiselect_accessible()) { goto_priv(PRIV_M); TEST_SKIP(...); }
 */
static inline bool vsiselect_accessible(void)
{
    trap_expect_begin();
    vsiselect_read();
    bool ok = !trap_was_triggered();
    trap_expect_end();
    return ok;
}

/* ===================================================================
 * Platform detection
 * =================================================================== */

/* Check if H extension is present */
#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

/* Check if Sscsrind is implemented (siselect + vsiselect accessible) */
static inline bool platform_has_sscsrind(void)
{
    /* Check S-level siselect */
    trap_expect_begin();
    siselect_read();
    bool trapped = trap_was_triggered();
    trap_expect_end();
    if (trapped)
        return false;

#ifdef ENABLE_HYP
    /*
     * Check VS-level vsiselect (CSR 0x250).  Some platforms implement
     * siselect but not the VS-level counterpart (e.g., QEMU max CPU may
     * expose siselect via Ssaia but leave vsiselect unimplemented).
     * Access from M-mode is always valid if the CSR exists.
     */
    trap_expect_begin();
    vsiselect_read();
    trapped = trap_was_triggered();
    trap_expect_end();
    if (trapped)
        return false;
#endif

    return true;
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

/* ===================================================================
 * VS-mode trampoline functions (for run_in_vs_mode)
 * =================================================================== */

/* VS-mode: read vsiselect directly */
static uintptr_t _vs_read_vsiselect(uintptr_t arg)
{
    (void)arg;
    return vsiselect_read();
}

/* VS-mode: write vsiselect directly */
static uintptr_t _vs_write_vsiselect(uintptr_t arg)
{
    vsiselect_write(arg);
    return 0;
}

/* VS-mode: read vsireg directly */
static uintptr_t _vs_read_vsireg(uintptr_t arg)
{
    (void)arg;
    return vsireg_read();
}

/* VS-mode: write vsireg directly */
static uintptr_t _vs_write_vsireg(uintptr_t arg)
{
    vsireg_write(arg);
    return 0;
}

/* VS-mode: read siselect (remapped to vsiselect) */
static uintptr_t _vs_read_siselect(uintptr_t arg)
{
    (void)arg;
    return siselect_read();
}

/* VS-mode: write siselect (remapped to vsiselect) */
static uintptr_t _vs_write_siselect(uintptr_t arg)
{
    siselect_write(arg);
    return 0;
}

/* VS-mode: read sireg (remapped to vsireg) */
static uintptr_t _vs_read_sireg(uintptr_t arg)
{
    (void)arg;
    return sireg_read();
}

/* VS-mode: write sireg (remapped to vsireg) */
static uintptr_t _vs_write_sireg(uintptr_t arg)
{
    sireg_write(arg);
    return 0;
}

/* VS-mode: read vsireg2~vsireg6 directly */
static uintptr_t _vs_read_vsireg2(uintptr_t arg)
{
    (void)arg;
    return vsireg2_read();
}

static uintptr_t _vs_read_vsireg3(uintptr_t arg)
{
    (void)arg;
    return vsireg3_read();
}

static uintptr_t _vs_read_vsireg4(uintptr_t arg)
{
    (void)arg;
    return vsireg4_read();
}

static uintptr_t _vs_read_vsireg5(uintptr_t arg)
{
    (void)arg;
    return vsireg5_read();
}

static uintptr_t _vs_read_vsireg6(uintptr_t arg)
{
    (void)arg;
    return vsireg6_read();
}

/* ===================================================================
 * VU-mode trampoline functions (for run_in_vu_mode)
 * =================================================================== */

/* VU-mode: read vsiselect directly (should trap) */
static uintptr_t _vu_read_vsiselect(uintptr_t arg)
{
    (void)arg;
    return vsiselect_read();
}

/* VU-mode: read vsireg directly (should trap) */
static uintptr_t _vu_read_vsireg(uintptr_t arg)
{
    (void)arg;
    return vsireg_read();
}

/* VU-mode: read siselect (should trap) */
static uintptr_t _vu_read_siselect(uintptr_t arg)
{
    (void)arg;
    return siselect_read();
}

/* VU-mode: write siselect (should trap) */
static uintptr_t _vu_write_siselect(uintptr_t arg)
{
    siselect_write(arg);
    return 0;
}

/* VU-mode: read sireg (should trap) */
static uintptr_t _vu_read_sireg(uintptr_t arg)
{
    (void)arg;
    return sireg_read();
}

/* ===================================================================
 * Test macros for VS/VU-mode trap verification
 * =================================================================== */

/* Execute fn in VS-mode and verify it traps with expected_cause */
#define TEST_VS_MODE_TRAP(desc, fn, arg, expected_cause) do { \
    trap_expect_begin(); \
    run_in_vs_mode(fn, arg); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ(desc, trap_get_cause(), (uintptr_t)expected_cause); \
    } else { \
        TEST_ASSERT(desc " (should trap)", false); \
    } \
    trap_expect_end(); \
} while (0)

/* Execute fn in VU-mode and verify it traps with expected_cause */
#define TEST_VU_MODE_TRAP(desc, fn, arg, expected_cause) do { \
    trap_expect_begin(); \
    run_in_vu_mode(fn, arg); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ(desc, trap_get_cause(), (uintptr_t)expected_cause); \
    } else { \
        TEST_ASSERT(desc " (should trap)", false); \
    } \
    trap_expect_end(); \
} while (0)

#endif /* HYPERVISOR_SSCSRIND_TEST_HELPERS_H */
