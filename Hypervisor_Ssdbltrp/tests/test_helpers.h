/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYPERVISOR_SSDBLTRP_TEST_HELPERS_H
#define HYPERVISOR_SSDBLTRP_TEST_HELPERS_H

/*
 * test_helpers.h - Common helpers for the Hypervisor x Ssdbltrp cross test suite.
 *
 * All test files are #included into test_register.c, so static functions
 * and globals defined here are visible across the whole compilation unit.
 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"

/* ===================================================================
 * Ssdbltrp field masks
 *
 * vsstatus / hstatus / henvcfg / menvcfg accessors (read/write and
 * set_bits/clear_bits) come from common/hyp/hyp_csr.h.
 * =================================================================== */

/* Bit definitions */
#define MENVCFG_DTE          (1ULL << 59)
#define HENVCFG_DTE          (1ULL << 59)

#define VSSTATUS_SDT         (1ULL << 24)
#define VSSTATUS_SIE         (1ULL << 1)
#define VSSTATUS_SPIE        (1ULL << 5)
#define VSSTATUS_SPP         (1ULL << 8)

#define SSTATUS_SDT          (1ULL << 24)
#define SSTATUS_SIE          (1ULL << 1)
#define SSTATUS_SPIE         (1ULL << 5)
#define SSTATUS_SPP          (1ULL << 8)

/* ===================================================================
 * vsstatus CSR access helpers
 *
 * vsstatus_read()/vsstatus_write() from common/hyp/hyp_csr.h
 * (reachable via hyp/hyp_test.h -> hyp/hyp_csr.h).
 * =================================================================== */

/* ===================================================================
 * hstatus CSR access helpers
 * =================================================================== */

/* ===================================================================
 * henvcfg CSR access helpers
 * =================================================================== */

/* ===================================================================
 * menvcfg CSR access helpers
 * =================================================================== */

/* ===================================================================
 * sstatus CSR access helpers (for SDT)
 * =================================================================== */

static inline uintptr_t sstatus_read_csr(void)
{
    uintptr_t v;
    asm volatile("csrr %0, sstatus" : "=r"(v) :: "memory");
    return v;
}

static inline void sstatus_set(uintptr_t bits)
{
    asm volatile("csrs sstatus, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * Ssdbltrp extension detection
 * =================================================================== */

/*
 * Check if Ssdbltrp is implemented by testing if henvcfg.DTE is writable
 * when menvcfg.DTE=1, AND vsstatus.SDT is writable when henvcfg.DTE=1.
 *
 * IMPORTANT: vsstatus is a hypervisor CSR that can only be accessed from
 * HS-mode or M-mode, not from VS-mode. We must be in M-mode when calling this.
 */
static inline bool check_ssdbltrp_extension(void)
{
    if (!H_AVAILABLE)
        return false;

    /* Ensure we're in M-mode */
    goto_priv(PRIV_M);

    /* Clear MDT first - required for safe mstatus access on Ssdbltrp platforms */
    clear_mdt();

    /* Save original state */
    uintptr_t menvcfg_orig = menvcfg_read();
    uintptr_t henvcfg_orig = henvcfg_read();
    uintptr_t vsstatus_orig = vsstatus_read();

    /* Enable DTE at M-level */
    menvcfg_set_bits(MENVCFG_DTE);
    uintptr_t menvcfg_val = menvcfg_read();
    if ((menvcfg_val & MENVCFG_DTE) == 0) {
        /* menvcfg.DTE is read-only zero - Ssdbltrp not implemented at M-level
         * This is expected on QEMU which doesn't support Ssdbltrp */
        menvcfg_write(menvcfg_orig);
        return false;
    }

    /* Try to set henvcfg.DTE */
    henvcfg_set_bits(HENVCFG_DTE);
    uintptr_t henvcfg_val = henvcfg_read();
    if ((henvcfg_val & HENVCFG_DTE) == 0) {
        /* henvcfg.DTE is read-only zero - Ssdbltrp not implemented at H-level */
        henvcfg_write(henvcfg_orig);
        menvcfg_write(menvcfg_orig);
        return false;
    }

    /* Try to set vsstatus.SDT - this must be done from M-mode or HS-mode */
    vsstatus_set_bits(VSSTATUS_SDT);
    bool sdt_writable = (vsstatus_read() & VSSTATUS_SDT) != 0;

    /* Restore - IMPORTANT: clear vsstatus.SDT to prevent double-traps
     * when entering VS-mode in subsequent tests */
    vsstatus_clear_bits(VSSTATUS_SDT);
    vsstatus_write(vsstatus_orig & ~VSSTATUS_SDT);
    henvcfg_write(henvcfg_orig);
    menvcfg_write(menvcfg_orig);

    return sdt_writable;
}

/* ===================================================================
 * Ssdbltrp-specific TEST_END / TEST_SKIP
 *
 * Clear MDT before framework's TEST_END/TEST_SKIP to prevent
 * double-trap when reset_state() writes CSRs (mtvec, stvec, etc).
 * =================================================================== */

#define SSDBLTRP_HYP_TEST_END() do { \
    clear_mdt(); \
    goto_priv(PRIV_M); \
    hyp_reset_state(); \
    return _test_end_record(); \
} while (0)

/* ===================================================================
 * VS/VU-mode trampoline functions for run_in_vs_mode/run_in_vu_mode
 *
 * IMPORTANT: In VS-mode, VS-mode software accesses sstatus (not vsstatus).
 * The hardware automatically maps sstatus reads/writes to vsstatus when
 * virtualization is enabled (V=1). So trampolines use sstatus accessors.
 * =================================================================== */

/* VS-mode: set sstatus.SDT (mapped to vsstatus.SDT by hardware) */
static uintptr_t _vs_set_vsstatus_sdt(uintptr_t arg)
{
    (void)arg;
    sstatus_set(SSTATUS_SDT);
    return (sstatus_read_csr() & SSTATUS_SDT) != 0;
}

/* VS-mode: trigger ecall to trap to VS-mode handler */
static uintptr_t _vs_ecall(uintptr_t arg)
{
    (void)arg;
    asm volatile("ecall");
    return 0;
}

/* VU-mode: trigger ecall to trap to VS-mode handler */
static uintptr_t _vu_ecall(uintptr_t arg)
{
    (void)arg;
    asm volatile("ecall");
    return 0;
}

#endif /* HYPERVISOR_SSDBLTRP_TEST_HELPERS_H */
