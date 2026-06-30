/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Sv39 VM Test Registration
 *
 * All test cases are organized by Group, matching vm_test_plan.md.
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1  (MAP)     - Basic page table mapping verification
 *   Group 2  (SIGN)    - Virtual address sign extension
 *   Group 3  (VALID)   - PTE validity check
 *   Group 4  (RWX)     - PTE permission bits
 *   Group 5  (UPRIV)   - U-bit and privilege access control
 *   Group 6  (SUM)     - SUM bit control
 *   Group 7  (MXR)     - MXR bit control
 *   Group 8  (ALIGN)   - Superpage alignment verification
 *   Group 9  (WALK)    - Multi-level page table walk
 *   Group 10 (AD)      - A/D bit management
 *   Group 11 (SATP)    - satp CSR control
 *   Group 12 (SFENCE)  - SFENCE.VMA instruction
 *   Group 13 (RSVD)    - PTE reserved bits check
 *   Group 14 (NLPTE)   - Non-leaf PTE reserved fields
 */

#include "test_helpers.h"

/* --- Group 1+2: Basic Mapping + Sign Extension (MAP-01~03, SIGN-03) --- */
#include "test_mapping.c"

/* --- Group 3: PTE Validity Check (VALID-01~05) --- */
#include "test_pte_valid.c"

/* --- Group 4: PTE Permission Bits RWX (RWX-01~05) --- */
#include "test_pte_perms.c"

/* --- Group 5+6: U-bit + SUM Control (UPRIV-03~07, SUM-01~05) --- */
#include "test_upriv.c"

/* --- Group 7: MXR Bit Control (MXR-01~05) --- */
#include "test_mxr.c"

/* --- Group 8+9: Superpage Alignment + Walk (ALIGN-01~02, WALK-01/04/05) --- */
#include "test_superpage.c"

/* --- Group 10: A/D Bit Management (AD-01~05) --- */
#include "test_ad_bits.c"

/* --- Group 11: satp CSR Control (SATP-01/02/05/07/09) --- */
#include "test_satp.c"

/* --- Group 12+13+14: SFENCE.VMA + Reserved Bits + Non-leaf PTE --- */
#include "test_sfence_rsvd.c"
