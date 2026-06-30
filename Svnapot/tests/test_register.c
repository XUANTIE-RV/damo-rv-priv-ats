/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Svnapot Extension Test Registration
 *
 * All test cases are organized by Group, matching svnapot_test_plan.md.
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1  (NAPOT64) - 64 KiB NAPOT page basic mapping
 *   Group 2  (PPN)     - PPN substitution semantics
 *   Group 3  (RSVD)    - Reserved encoding exceptions
 *   Group 4  (PERM)    - NAPOT PTE permission control
 *   Group 5  (NZERO)   - N=0 behavior verification
 *   Group 6  (SFENCE)  - SFENCE.VMA and NAPOT interaction
 *   Group 7  (MODE)    - Sv mode compatibility
 *   Group 8  (AD)      - A/D bits and NAPOT pages
 *   Group 9  (TLB)     - NAPOT PTE and TLB cache behavior
 *   Group 11 (BITS50)  - PTE bits 5-0 consistency and G bit
 *   Group 12 (RSW)     - RSW bits verification
 *   Group 13 (ALIGN)   - NAPOT region boundary alignment
 */

#include "test_helpers.h"

/* --- Group 1: 64 KiB NAPOT Page Basic Mapping (NAPOT64-01~06) --- */
#include "test_napot_basic.c"

/* --- Group 2: PPN Substitution Semantics (PPN-01~05) --- */
#include "test_ppn_subst.c"

/* --- Group 3: Reserved Encoding Exceptions (RSVD-01~09) --- */
#include "test_reserved.c"

/* --- Group 4: NAPOT PTE Permission Control (PERM-01~07) --- */
#include "test_permissions.c"

/* --- Group 5: N=0 Behavior Verification (NZERO-01~03) --- */
#include "test_n_zero.c"

/* --- Group 6: SFENCE.VMA and NAPOT Interaction (SFENCE-01~05) --- */
#include "test_sfence.c"

/* --- Group 7: Sv Mode Compatibility (MODE-01~05) --- */
#include "test_sv_modes.c"

/* --- Group 8: A/D Bits and NAPOT Pages (AD-01~04) --- */
#include "test_ad_bits.c"

/* --- Group 9: NAPOT PTE and TLB Cache Behavior (TLB-01~03) --- */
#include "test_tlb.c"

/* --- Group 11: PTE bits 5-0 Consistency and G Bit (BITS50-03) --- */
#include "test_bits50.c"

/* --- Group 12: RSW Bits Verification (RSW-01~02) --- */
#include "test_rsw.c"

/* --- Group 13: NAPOT Region Boundary Alignment (ALIGN-01~02) --- */
#include "test_alignment.c"
