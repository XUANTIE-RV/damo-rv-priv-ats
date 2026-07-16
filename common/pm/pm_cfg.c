/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pm_cfg.h"
#include "encoding.h"
#include "uart.h"

/* Trap API from trap.c */
extern void trap_expect_begin(void);
extern void trap_expect_end(void);
extern bool trap_was_triggered(void);

/* ===================================================================
 * U-mode PM control (via senvcfg.PMM, Ssnpm)
 * =================================================================== */

void pm_set_umode(unsigned pmm) {
    uintptr_t val = CSRR(CSR_SENVCFG);
    val &= ~SENVCFG_PMM_MASK;
    val |= ((uintptr_t)(pmm & 0x3) << SENVCFG_PMM_OFF);
    CSRW(CSR_SENVCFG, val);
}

unsigned pm_get_umode(void) {
    uintptr_t val = CSRR(CSR_SENVCFG);
    return (unsigned)((val >> SENVCFG_PMM_OFF) & 0x3);
}

/* ===================================================================
 * S-mode PM control (via menvcfg.PMM, Smnpm)
 * =================================================================== */

void pm_set_smode(unsigned pmm) {
    uintptr_t val = CSRR(CSR_MENVCFG);
    val &= ~MENVCFG_PMM_MASK;
    val |= ((uintptr_t)(pmm & 0x3) << MENVCFG_PMM_OFF);
    CSRW(CSR_MENVCFG, val);
}

unsigned pm_get_smode(void) {
    uintptr_t val = CSRR(CSR_MENVCFG);
    return (unsigned)((val >> MENVCFG_PMM_OFF) & 0x3);
}

/* ===================================================================
 * M-mode PM control (via mseccfg.PMM, Smmpm)
 * =================================================================== */

void pm_set_mmode(unsigned pmm) {
    uintptr_t val = CSRR(CSR_MSECCFG);
    val &= ~MSECCFG_PMM_MASK;
    val |= ((uintptr_t)(pmm & 0x3) << MSECCFG_PMM_OFF);
    CSRW(CSR_MSECCFG, val);
}

unsigned pm_get_mmode(void) {
    uintptr_t val = CSRR(CSR_MSECCFG);
    return (unsigned)((val >> MSECCFG_PMM_OFF) & 0x3);
}

/* ===================================================================
 * Extension detection
 *
 * Strategy: probe PMM field by trying PMLEN7 (PMM=0b10) first,
 * then PMLEN16 (PMM=0b11).  Either acceptance proves the PMM
 * field is writable and the extension is implemented.  This
 * ordering handles WARL implementations that may accept only
 * one PMM encoding (e.g., PMLEN7 but not PMLEN16).
 *
 * Uses trap_expect to handle the case where the CSR itself does
 * not exist (would cause illegal instruction exception).
 * =================================================================== */

bool detect_ssnpm(void) {
    trap_expect_begin();
    unsigned saved = pm_get_umode();
    if (trap_was_triggered()) {
        trap_expect_end();
        return false;
    }

    /*
     * Probe PMM field by trying PMLEN7 (PMM=0b10) first, then PMLEN16
     * (PMM=0b11).  A WARL implementation may accept only one of these
     * encodings; either acceptance proves the extension is present.
     */
    pm_set_umode(PMM_PMLEN7);
    bool pmlen7 = (pm_get_umode() == PMM_PMLEN7);

    pm_set_umode(PMM_PMLEN16);
    bool pmlen16 = (pm_get_umode() == PMM_PMLEN16);

    pm_set_umode(saved);
    trap_expect_end();

    return pmlen7 || pmlen16;
}

bool detect_smnpm(void) {
    trap_expect_begin();
    unsigned saved = pm_get_smode();
    if (trap_was_triggered()) {
        trap_expect_end();
        return false;
    }

    /*
     * Probe PMM field by trying PMLEN7 (PMM=0b10) first, then PMLEN16
     * (PMM=0b11).  A WARL implementation may accept only one of these
     * encodings; either acceptance proves the extension is present.
     */
    pm_set_smode(PMM_PMLEN7);
    bool pmlen7 = (pm_get_smode() == PMM_PMLEN7);

    pm_set_smode(PMM_PMLEN16);
    bool pmlen16 = (pm_get_smode() == PMM_PMLEN16);

    pm_set_smode(saved);
    trap_expect_end();

    return pmlen7 || pmlen16;
}

bool detect_smmpm(void) {
    trap_expect_begin();
    unsigned saved = pm_get_mmode();
    if (trap_was_triggered()) {
        trap_expect_end();
        return false;
    }

    /*
     * Probe PMM field by trying PMLEN7 (PMM=0b10) first, then PMLEN16
     * (PMM=0b11).  Either acceptance proves the extension is present.
     *
     * We prefer PMLEN7 first because mseccfg.PMM is sticky when
     * ext_smepmp is enabled (new |= old): once 0b11 is written, the
     * field can never return to 0b10 (3|2=3), poisoning later PMLEN7
     * tests.  Writing 0b10 first is safe because 0|2=2 and we can
     * still upgrade to 0b11 later.  The PMLEN16 fallback is needed
     * for implementations that only support PMLEN=16.
     */
    pm_set_mmode(PMM_PMLEN7);
    bool pmlen7 = (pm_get_mmode() == PMM_PMLEN7);

    if (!pmlen7) {
        pm_set_mmode(PMM_PMLEN16);
    }
    bool pmlen16 = (pm_get_mmode() == PMM_PMLEN16);

    pm_set_mmode(saved);
    trap_expect_end();

    return pmlen7 || pmlen16;
}

/* ===================================================================
 * Supported PMLEN detection
 *
 * Tests which PMLEN values (7 and/or 16) are supported by trying
 * to write each PMM encoding and reading back.
 *
 * IMPORTANT: mseccfg.PMM is sticky when ext_smepmp is enabled
 * (new_val = new_val | old_val). This means:
 *   - Once PMM=3 is written, PMM=2 can never be achieved (3|2=3)
 *   - Once any non-zero PMM is written, it cannot be cleared (N|0=N)
 *
 * For mseccfg (M-mode), we ONLY test PMLEN7 (PMM=0b10) during
 * detection to avoid permanently poisoning the PMM field. PMLEN16
 * support is inferred: if the PMM field is writable at all, PMLEN16
 * (PMM=0b11) is always supported (it's the OR of all bits).
 *
 * For menvcfg/senvcfg (S-mode/U-mode), PMM is NOT sticky, so both
 * values can be tested safely.
 *
 * Returns a bitmask:
 *   bit 0 = PMLEN=7  supported (PMM=0b10)
 *   bit 1 = PMLEN=16 supported (PMM=0b11)
 * =================================================================== */

unsigned pm_detect_supported_pmlen(bool (*detect_fn)(void),
                                   void (*set_fn)(unsigned),
                                   unsigned (*get_fn)(void)) {
    unsigned supported = 0;

    if (!detect_fn())
        return 0;

    /* Save current PMM */
    unsigned saved = get_fn();

    /*
     * Test PMLEN=7 (PMM=0b10) FIRST.
     * For mseccfg with sticky bits, this is safe because 0|2=2.
     */
    set_fn(PMM_PMLEN7);
    if (get_fn() == PMM_PMLEN7)
        supported |= (1U << 0);

    /*
     * Test PMLEN=16 (PMM=0b11).
     * WARNING: For mseccfg with sticky bits, once we write 3 here,
     * PMM can never go back to 2 (3|2=3). Only do this if the caller
     * can tolerate the side effect, or if PMM is not sticky.
     *
     * We test PMLEN16 by writing it; if PMM is sticky (already at 2),
     * writing 3 will succeed (2|3=3). If not sticky, it also works.
     */
    set_fn(PMM_PMLEN16);
    if (get_fn() == PMM_PMLEN16)
        supported |= (1U << 1);

    /* Try to restore (may fail for mseccfg due to sticky bits) */
    set_fn(saved);

    return supported;
}

/*
 * Non-destructive PMLEN detection for M-mode (mseccfg).
 * Only tests PMLEN7 to avoid poisoning the sticky PMM field.
 * Infers PMLEN16 support from PMLEN7: if PMLEN7 works, PMLEN16
 * also works (since 2|3=3 is always achievable from state 2).
 */
unsigned pm_detect_supported_pmlen_mmode(void) {
    if (!detect_smmpm())
        return 0;

    unsigned saved = pm_get_mmode();
    unsigned supported = 0;

    /* Only test PMLEN7 (PMM=0b10) to avoid sticky pollution */
    pm_set_mmode(PMM_PMLEN7);
    if (pm_get_mmode() == PMM_PMLEN7) {
        supported |= (1U << 0);  /* PMLEN7 supported */
        supported |= (1U << 1);  /* PMLEN16 also supported (superset) */
    } else {
        /* PMLEN7 failed, try PMLEN16 directly */
        pm_set_mmode(PMM_PMLEN16);
        if (pm_get_mmode() == PMM_PMLEN16)
            supported |= (1U << 1);
    }

    /* Restore (best effort, may not clear due to sticky) */
    pm_set_mmode(saved);

    return supported;
}
