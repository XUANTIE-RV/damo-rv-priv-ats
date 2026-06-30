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
 * Ssdbltrp CSR address constants and field masks
 * =================================================================== */

#define CSR_VSSTATUS_ADDR    0x200
#define CSR_HSTATUS_ADDR     0x600
#define CSR_HENVCFG_ADDR     0x60A
#define CSR_MENVCFG_ADDR     0x30A
#define CSR_MTVAL2_ADDR      0x34B

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

#define MSTATUS_SDT          (1ULL << 24)

#define HSTATUS_SPV          (1ULL << 7)
#define HSTATUS_SPVP         (1ULL << 8)

#define CAUSE_DOUBLE_TRAP    16
#define CAUSE_ECALL_FROM_VU  8
#define CAUSE_ECALL_FROM_VS  10

/* ===================================================================
 * vsstatus CSR access helpers
 * =================================================================== */

static inline uintptr_t vsstatus_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x200" : "=r"(v) :: "memory");
    return v;
}

static inline void vsstatus_write(uintptr_t v)
{
    asm volatile("csrw 0x200, %0" :: "r"(v) : "memory");
}

static inline void vsstatus_set(uintptr_t bits)
{
    asm volatile("csrs 0x200, %0" :: "r"(bits) : "memory");
}

static inline void vsstatus_clear(uintptr_t bits)
{
    asm volatile("csrc 0x200, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * hstatus CSR access helpers
 * =================================================================== */

static inline uintptr_t hstatus_read_csr(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x600" : "=r"(v) :: "memory");
    return v;
}

static inline void hstatus_write_csr(uintptr_t v)
{
    asm volatile("csrw 0x600, %0" :: "r"(v) : "memory");
}

static inline void hstatus_set(uintptr_t bits)
{
    asm volatile("csrs 0x600, %0" :: "r"(bits) : "memory");
}

static inline void hstatus_clear(uintptr_t bits)
{
    asm volatile("csrc 0x600, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * henvcfg CSR access helpers
 * =================================================================== */

static inline uintptr_t henvcfg_read_csr(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x60A" : "=r"(v) :: "memory");
    return v;
}

static inline void henvcfg_write_csr(uintptr_t v)
{
    asm volatile("csrw 0x60A, %0" :: "r"(v) : "memory");
}

static inline void henvcfg_set(uintptr_t bits)
{
    asm volatile("csrs 0x60A, %0" :: "r"(bits) : "memory");
}

static inline void henvcfg_clear(uintptr_t bits)
{
    asm volatile("csrc 0x60A, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * menvcfg CSR access helpers
 * =================================================================== */

static inline uintptr_t menvcfg_read_csr(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x30A" : "=r"(v) :: "memory");
    return v;
}

static inline void menvcfg_write_csr(uintptr_t v)
{
    asm volatile("csrw 0x30A, %0" :: "r"(v) : "memory");
}

static inline void menvcfg_set(uintptr_t bits)
{
    asm volatile("csrs 0x30A, %0" :: "r"(bits) : "memory");
}

static inline void menvcfg_clear(uintptr_t bits)
{
    asm volatile("csrc 0x30A, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * mtval2 CSR access helpers
 * =================================================================== */

static inline uintptr_t mtval2_read(void)
{
    uintptr_t v;
    asm volatile("csrr %0, 0x34B" : "=r"(v) :: "memory");
    return v;
}

static inline void mtval2_write(uintptr_t v)
{
    asm volatile("csrw 0x34B, %0" :: "r"(v) : "memory");
}

/* ===================================================================
 * mstatus CSR access helpers (for SDT)
 * =================================================================== */

static inline uintptr_t mstatus_read_csr(void)
{
    uintptr_t v;
    asm volatile("csrr %0, mstatus" : "=r"(v) :: "memory");
    return v;
}

static inline void mstatus_write_csr(uintptr_t v)
{
    asm volatile("csrw mstatus, %0" :: "r"(v) : "memory");
}

static inline void mstatus_set(uintptr_t bits)
{
    asm volatile("csrs mstatus, %0" :: "r"(bits) : "memory");
}

static inline void mstatus_clear(uintptr_t bits)
{
    asm volatile("csrc mstatus, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * sstatus CSR access helpers (for SDT)
 * =================================================================== */

static inline uintptr_t sstatus_read_csr(void)
{
    uintptr_t v;
    asm volatile("csrr %0, sstatus" : "=r"(v) :: "memory");
    return v;
}

static inline void sstatus_write_csr(uintptr_t v)
{
    asm volatile("csrw sstatus, %0" :: "r"(v) : "memory");
}

static inline void sstatus_set(uintptr_t bits)
{
    asm volatile("csrs sstatus, %0" :: "r"(bits) : "memory");
}

static inline void sstatus_clear(uintptr_t bits)
{
    asm volatile("csrc sstatus, %0" :: "r"(bits) : "memory");
}

/* ===================================================================
 * H extension detection
 * =================================================================== */
#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

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
    if (!HAS_H_EXT())
        return false;

    /* Ensure we're in M-mode */
    goto_priv(PRIV_M);

    /* Clear MDT first - required for safe mstatus access on Ssdbltrp platforms */
    clear_mdt();

    /* Save original state */
    uintptr_t menvcfg_orig = menvcfg_read_csr();
    uintptr_t henvcfg_orig = henvcfg_read_csr();
    uintptr_t vsstatus_orig = vsstatus_read();

    /* Enable DTE at M-level */
    menvcfg_set(MENVCFG_DTE);
    uintptr_t menvcfg_val = menvcfg_read_csr();
    if ((menvcfg_val & MENVCFG_DTE) == 0) {
        /* menvcfg.DTE is read-only zero - Ssdbltrp not implemented at M-level
         * This is expected on QEMU which doesn't support Ssdbltrp */
        menvcfg_write_csr(menvcfg_orig);
        return false;
    }

    /* Try to set henvcfg.DTE */
    henvcfg_set(HENVCFG_DTE);
    uintptr_t henvcfg_val = henvcfg_read_csr();
    if ((henvcfg_val & HENVCFG_DTE) == 0) {
        /* henvcfg.DTE is read-only zero - Ssdbltrp not implemented at H-level */
        henvcfg_write_csr(henvcfg_orig);
        menvcfg_write_csr(menvcfg_orig);
        return false;
    }

    /* Try to set vsstatus.SDT - this must be done from M-mode or HS-mode */
    vsstatus_set(VSSTATUS_SDT);
    bool sdt_writable = (vsstatus_read() & VSSTATUS_SDT) != 0;

    /* Restore - IMPORTANT: clear vsstatus.SDT to prevent double-traps
     * when entering VS-mode in subsequent tests */
    vsstatus_clear(VSSTATUS_SDT);
    vsstatus_write(vsstatus_orig & ~VSSTATUS_SDT);
    henvcfg_write_csr(henvcfg_orig);
    menvcfg_write_csr(menvcfg_orig);

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

#define SSDBLTRP_HYP_TEST_SKIP(reason) do { \
    clear_mdt(); \
    test_results.skipped++; \
    if (test_results.skipped_count < MAX_SKIPPED_TESTS) { \
        test_results.skipped_names[test_results.skipped_count] = \
            test_results.current_test_name; \
        test_results.skipped_reasons[test_results.skipped_count] = (reason); \
        test_results.skipped_count++; \
    } \
    printf("[SKIP] %s: %s\n\n", test_results.current_test_name, (reason)); \
    goto_priv(PRIV_M); \
    hyp_reset_state(); \
    return true; \
} while (0)

/* ===================================================================
 * VS/VU-mode trampoline functions for run_in_vs_mode/run_in_vu_mode
 *
 * IMPORTANT: In VS-mode, VS-mode software accesses sstatus (not vsstatus).
 * The hardware automatically maps sstatus reads/writes to vsstatus when
 * virtualization is enabled (V=1). So trampolines use sstatus accessors.
 * =================================================================== */

/* VS-mode: read sstatus.SDT (mapped to vsstatus.SDT by hardware) */
static uintptr_t _vs_read_vsstatus_sdt(uintptr_t arg)
{
    (void)arg;
    return (sstatus_read_csr() & SSTATUS_SDT) != 0;
}

/* VS-mode: set sstatus.SDT (mapped to vsstatus.SDT by hardware) */
static uintptr_t _vs_set_vsstatus_sdt(uintptr_t arg)
{
    (void)arg;
    sstatus_set(SSTATUS_SDT);
    return (sstatus_read_csr() & SSTATUS_SDT) != 0;
}

/* VS-mode: clear sstatus.SDT (mapped to vsstatus.SDT by hardware) */
static uintptr_t _vs_clear_vsstatus_sdt(uintptr_t arg)
{
    (void)arg;
    sstatus_clear(SSTATUS_SDT);
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

/* VS-mode: set sstatus.SDT and trigger ecall (for double-trap test) */
static uintptr_t _vs_set_sdt_and_ecall(uintptr_t arg)
{
    (void)arg;
    sstatus_set(SSTATUS_SDT);
    asm volatile("ecall");
    return 0;
}

/* VU-mode: set sstatus.SDT and trigger ecall (for double-trap test) */
static uintptr_t _vu_set_sdt_and_ecall(uintptr_t arg)
{
    (void)arg;
    sstatus_set(SSTATUS_SDT);
    asm volatile("ecall");
    return 0;
}

#endif /* HYPERVISOR_SSDBLTRP_TEST_HELPERS_H */
