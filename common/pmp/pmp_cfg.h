/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMP_CFG_H
#define PMP_CFG_H

/* PMP permission/mode bit constants (PMP_R, PMP_W, PMP_X, PMP_A_*,
 * PMP_L, etc.) are defined in sm_defs.h, included via encoding.h. */
#include "encoding.h"

/* ===================================================================
 * PMP Configuration Field Extraction / Construction Helpers
 *
 * PMP entry config register layout:
 *   [7]   L   - Lock
 *   [6:5] 00  - Reserved (WARL, reads as 0)
 *   [4:3] A   - Address matching mode
 *   [2]   X   - Execute permission
 *   [1]   W   - Write permission
 *   [0]   R   - Read permission
 * =================================================================== */

/* Field extraction macros */
#define PMP_CFG_R(cfg)      ((cfg) & 0x1)
#define PMP_CFG_W(cfg)      (((cfg) >> 1) & 0x1)
#define PMP_CFG_X(cfg)      (((cfg) >> 2) & 0x1)
#define PMP_CFG_A(cfg)      (((cfg) >> 3) & 0x3)
#define PMP_CFG_L(cfg)      (((cfg) >> 7) & 0x1)

/* Field construction helpers */
#define PMP_CFG_A_SET(mode) (((mode) & 0x3) << 3)

/* Maximum PMP entries per spec */
#define PMP_MAX_ENTRIES     64

/* ===================================================================
 * PMP Entry Configuration Structure
 *
 * Users fill this structure and pass it to pmp_set_entry().
 * The framework handles the encoding details internally.
 * =================================================================== */
typedef struct {
    uint8_t   cfg;      /* pmpcfg value: L|00|A|X|W|R */
    uintptr_t addr;     /* Physical address (raw, not shifted) */
    uint64_t  size;     /* Region size in bytes (NAPOT mode only) */
} pmp_entry_t;

/* ===================================================================
 * Core API
 * =================================================================== */

/**
 * pmp_set_entry - Configure a single PMP entry
 *
 * @index: PMP entry index (0-63)
 * @entry: Configuration to apply
 *
 * Internally handles:
 *   - Address encoding (>> 2 for pmpaddr)
 *   - NAPOT size encoding
 *   - pmpcfg byte packing (4 entries per CSR on RV32, 8 on RV64)
 */
void pmp_set_entry(unsigned int index, const pmp_entry_t *entry);

/**
 * pmp_get_entry - Read current configuration of a PMP entry
 *
 * @index: PMP entry index (0-63)
 * @entry: Output structure (addr is decoded back to physical address)
 */
void pmp_get_entry(unsigned int index, pmp_entry_t *entry);

/**
 * pmp_clear_entry - Disable a single PMP entry (set A=OFF)
 */
void pmp_clear_entry(unsigned int index);

/**
 * pmp_clear_all - Disable all PMP entries
 */
void pmp_clear_all(void);

/**
 * pmp_set_entries - Configure multiple consecutive PMP entries
 *
 * @entries:     Array of configurations
 * @count:       Number of entries
 * @start_index: First PMP entry index
 */
void pmp_set_entries(const pmp_entry_t *entries, unsigned int count,
                     unsigned int start_index);

/* ===================================================================
 * Address Encoding Helpers
 * =================================================================== */

/**
 * pmp_encode_napot - Encode (base, size) for NAPOT mode pmpaddr
 *
 * @base: Region base address (must be naturally aligned to size)
 * @size: Region size (must be power of 2, >= 8 bytes)
 * @return: Value to write to pmpaddr register
 */
uintptr_t pmp_encode_napot(uintptr_t base, uint64_t size);

/**
 * pmp_encode_tor - Encode address for TOR mode pmpaddr
 *
 * @addr: Top of range address
 * @return: Value to write to pmpaddr register
 */
uintptr_t pmp_encode_tor(uintptr_t addr);

/**
 * pmp_encode_na4 - Encode address for NA4 mode pmpaddr
 *
 * @addr: 4-byte aligned address
 * @return: Value to write to pmpaddr register
 */
uintptr_t pmp_encode_na4(uintptr_t addr);

/* ===================================================================
 * Hardware Capability Detection
 * =================================================================== */

/**
 * pmp_detect_granularity - Detect PMP granularity G value
 *
 * @return: G value. Minimum region size = 2^(G+2) bytes.
 *          G=0 means 4-byte granularity (NA4 supported).
 */
unsigned int pmp_detect_granularity(void);

/**
 * pmp_detect_entry_count - Detect number of implemented PMP entries
 *
 * @return: 0, 16, or 64
 */
unsigned int pmp_detect_entry_count(void);

/* ===================================================================
 * mseccfg Access API (Smepmp Extension)
 * =================================================================== */

/**
 * mseccfg_read - Read the full mseccfg register
 *
 * On RV32, combines mseccfg and mseccfgh into a 64-bit value.
 */
uint64_t mseccfg_read(void);

/**
 * mseccfg_write - Write the full mseccfg register
 *
 * On RV32, splits into mseccfg (low 32) and mseccfgh (high 32).
 */
void mseccfg_write(uint64_t val);

/**
 * mseccfg_set - Set specific bits in mseccfg (OR operation)
 */
void mseccfg_set(uint64_t bits);

/**
 * mseccfg_clear - Clear specific bits in mseccfg (AND-NOT operation)
 *
 * Note: MML and MMWP are sticky bits - once set, they cannot be
 * cleared until PMP reset. Attempting to clear them will be ignored
 * by hardware.
 */
void mseccfg_clear(uint64_t bits);

/**
 * smepmp_is_supported - Detect if Smepmp extension is available
 *
 * Attempts to write and read back mseccfg.RLB to detect presence.
 * Returns true if Smepmp is supported.
 */
bool smepmp_is_supported(void);

/* ===================================================================
 * Convenience Construction Macros
 *
 * Usage:
 *   pmp_entry_t e = PMP_ENTRY_NAPOT(0x80000000, 0x1000, PMP_RWX);
 *   pmp_set_entry(0, &e);
 * =================================================================== */

#define PMP_ENTRY_NAPOT(base, sz, rwx_flags) \
    (pmp_entry_t){ .cfg = PMP_A_NAPOT | (rwx_flags), \
                   .addr = (base), .size = (sz) }

/* NAPOT entry covering the entire physical address space.
 * RV64: 2^56 bytes (full Sv39/Sv48/Sv57 physical address width).
 * RV32: 2^32 bytes (full 4 GiB address space). */
#if __riscv_xlen == 64
#define PMP_ENTRY_FULL(rwx_flags) \
    PMP_ENTRY_NAPOT(0x0, (1ULL << 56), (rwx_flags))
#else
#define PMP_ENTRY_FULL(rwx_flags) \
    PMP_ENTRY_NAPOT(0x0, 0x100000000ULL, (rwx_flags))
#endif

#define PMP_ENTRY_TOR(top_addr, rwx_flags) \
    (pmp_entry_t){ .cfg = PMP_A_TOR | (rwx_flags), \
                   .addr = (top_addr), .size = 0 }

#define PMP_ENTRY_NA4(address, rwx_flags) \
    (pmp_entry_t){ .cfg = PMP_A_NA4 | (rwx_flags), \
                   .addr = (address), .size = 4 }

#define PMP_ENTRY_OFF() \
    (pmp_entry_t){ .cfg = PMP_A_OFF, .addr = 0, .size = 0 }

/* Locked variants */
#define PMP_ENTRY_NAPOT_LOCKED(base, sz, rwx_flags) \
    (pmp_entry_t){ .cfg = PMP_L | PMP_A_NAPOT | (rwx_flags), \
                   .addr = (base), .size = (sz) }

#define PMP_ENTRY_TOR_LOCKED(top_addr, rwx_flags) \
    (pmp_entry_t){ .cfg = PMP_L | PMP_A_TOR | (rwx_flags), \
                   .addr = (top_addr), .size = 0 }

#endif /* PMP_CFG_H */
