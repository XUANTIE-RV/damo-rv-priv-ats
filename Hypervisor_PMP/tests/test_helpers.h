/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYPERVISOR_PMP_TEST_HELPERS_H
#define HYPERVISOR_PMP_TEST_HELPERS_H

/* ===================================================================
 * Forward header for Hypervisor x PMP cross tests.
 *
 * Provides:
 *   - test framework + VM/Hypervisor common definitions
 *   - two-stage translation helpers (VS + G stage)
 *   - PMP configuration API (common/pmp/pmp_cfg.h)
 *   - suite-local PMP deny/restore windows and runtime gates
 * =================================================================== */

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_fence.h"
#include "hyp/hyp_ldst.h"
#include "hyp/hyp_trap.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/test_vs_helpers.h"
#include "pmp/pmp_cfg.h"

/* Linker-provided test-region symbols (see kernel.ld). */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t test_exec_page[];
extern uint8_t test_exec_target[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

/* Suite translation modes: Sv39 (VS-stage) + Sv39x4 (G-stage). */
#define HPMP_VS_MODE   SATP_MODE_SV39
#define HPMP_G_MODE    HGATP_MODE_SV39X4

/* Check if H extension is present (misa.H). */
#define HAS_H_EXT() ({ \
    uintptr_t _misa; \
    asm volatile("csrr %0, misa" : "=r"(_misa) :: "memory"); \
    (_misa & (1UL << ('H' - 'A'))) != 0; \
})

/* ===================================================================
 * Runtime capability gate
 *
 * The suite needs: H extension, at least 2 PMP entries, granularity
 * fine enough for a 4KB NAPOT region (G <= 10), and mseccfg.MML must
 * not be active (it redefines S/U PMP semantics and is sticky).
 * =================================================================== */
#define REQUIRE_HYP_PMP() do { \
    if (!HAS_H_EXT()) { \
        TEST_SKIP("H extension not available"); \
    } \
    if (pmp_detect_entry_count() < 2) { \
        TEST_SKIP("need at least 2 PMP entries"); \
    } \
    if (pmp_detect_granularity() > 10) { \
        TEST_SKIP("PMP granularity too coarse for 4KB NAPOT"); \
    } \
    if (smepmp_is_supported() && (mseccfg_read() & MSECCFG_MML)) { \
        TEST_SKIP("mseccfg.MML active: S/U PMP semantics redefined"); \
    } \
    REQUIRE_VSATP_MODE(HPMP_VS_MODE); \
    REQUIRE_HGATP_MODE(HPMP_G_MODE); \
} while (0)

/* ===================================================================
 * PMP deny-window helpers
 *
 * Strategy (mirrors Sv39x4_Sv39 Group 19 / TS-PMP-01):
 *   - PMP entries are statically prioritized, lowest index wins.
 *   - hyp_reset_state() installs entry 0 = NAPOT(all-space, RWX).
 *   - We override entry 0 with a rule matching the victim 4KB page
 *     only, and add entry 1 as fall-through NAPOT(all-space, RWX).
 *   - All entries stay unlocked (L=0), so M-mode is never affected
 *     and hyp_reset_state() can always restore the baseline.
 *
 * Every PMP change is followed by SFENCE.VMA (x0,x0) + HFENCE.GVMA
 * (x0,x0): translation caches may cache PMP attributes for the final
 * translated SPA (norm:pmp_sfence_required, hypervisor.adoc).
 * =================================================================== */
typedef struct {
    pmp_entry_t e0;
    pmp_entry_t e1;
} hpmp_save_t;

static inline void hpmp_sync_fences(void) {
    vm_sfence_vma(0, 0);
    hfence_gvma(0, 0);
}

/* Deny @pa's 4KB page completely (no R/W/X). */
static void hpmp_deny_page(uintptr_t pa, hpmp_save_t *save) {
    pmp_get_entry(0, &save->e0);
    pmp_get_entry(1, &save->e1);

    pmp_entry_t deny = PMP_ENTRY_NAPOT(pa & ~0xfffUL, 0x1000UL, 0);
    pmp_set_entry(0, &deny);
    pmp_entry_t allow = PMP_ENTRY_NAPOT(0, (uintptr_t)1UL << 54, PMP_RWX);
    pmp_set_entry(1, &allow);
    hpmp_sync_fences();
}

/* Grant execute-only on @pa's 4KB page (X=1, R=0, W=0). */
static void hpmp_xonly_page(uintptr_t pa, hpmp_save_t *save) {
    pmp_get_entry(0, &save->e0);
    pmp_get_entry(1, &save->e1);

    pmp_entry_t xonly = PMP_ENTRY_NAPOT(pa & ~0xfffUL, 0x1000UL, PMP_X);
    pmp_set_entry(0, &xonly);
    pmp_entry_t allow = PMP_ENTRY_NAPOT(0, (uintptr_t)1UL << 54, PMP_RWX);
    pmp_set_entry(1, &allow);
    hpmp_sync_fences();
}

static void hpmp_restore(const hpmp_save_t *save) {
    pmp_set_entry(0, &save->e0);
    pmp_set_entry(1, &save->e1);
    hpmp_sync_fences();
}

/* ===================================================================
 * G-stage page-table walk helper (Sv39x4 only)
 *
 * Returns the physical address of the second-level (mid) G-stage
 * page-table page reached through the root entry for @gpa, i.e. the
 * page the walker reads when translating a 4KB leaf inside that 1GB
 * window. Returns 0 if the root slot is not a valid pointer.
 * =================================================================== */
static uintptr_t hpmp_g_mid_pt_page(const two_stage_ctx_t *ctx,
                                    uintptr_t gpa) {
    uintptr_t *root = ctx->g_ctx.root_pt;
    uintptr_t idx = (gpa >> 30) & 0x7FFUL;   /* Sv39x4: VPN[2] 11 bits */
    uintptr_t pte = root[idx];
    if ((pte & PTE_V) == 0 || PTE_IS_LEAF(pte)) {
        return 0;
    }
    return (pte >> 10) << 12;
}

#endif /* HYPERVISOR_PMP_TEST_HELPERS_H */
