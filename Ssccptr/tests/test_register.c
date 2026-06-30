/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Ssccptr Extension Test Registration
 *
 * All test cases are organized by Group, matching docs/ssccptr_test_plan.md.
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1 (BASIC)     - Basic page walk verification (Sv39, 4 KiB)
 *   Group 2 (SUPER)     - Superpage page walk (Sv39, 2M/1G)
 *   Group 3 (LEVEL)     - Per-level PTE read verification + PMP control
 *   Group 4 (SV48)      - Sv48 page table walk
 *   Group 5 (SV57)      - Sv57 page table walk
 *   Group 6 (TLB)       - Repeated page walk after TLB flush
 *   Group 7 (DYN)       - Dynamic PTE modification
 *   Group 8 (PMP)       - PMP blocks page walk (control group)
 */

#include "test_helpers.h"

/* --- Group 1: Basic page walk (SSCCPTR-BASIC-01~06) --- */
#include "test_basic.c"

/* --- Group 2: Superpage page walk (SSCCPTR-SUPER-01~06) --- */
#include "test_superpage.c"

/* --- Group 3: Per-level verification + PMP control (SSCCPTR-LEVEL-01~06) --- */
#include "test_level.c"

/* --- Group 4: Sv48 page walk (SSCCPTR-SV48-01~06) --- */
#include "test_sv48.c"

/* --- Group 5: Sv57 page walk (SSCCPTR-SV57-01~07) --- */
#include "test_sv57.c"

/* --- Group 6: TLB flush + repeated walk (SSCCPTR-TLB-01~04) --- */
#include "test_tlb.c"

/* --- Group 7: Dynamic PTE modification (SSCCPTR-DYN-01~04) --- */
#include "test_dynamic.c"

/* --- Group 8: PMP blocks page walk (SSCCPTR-PMP-01~06) --- */
#include "test_pmp_block.c"
