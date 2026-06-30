/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Svpbmt Extension Test Registration
 *
 * All test cases are organized by Group, matching svpbmt_test_plan.md.
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1  (PBMT)    - PBMT basic encoding verification
 *   Group 2  (RSVD)    - PBMT reserved value exceptions
 *   Group 3  (NLPTE)   - Non-leaf PTE PBMT check
 *   Group 4  (PERM)    - PBMT and page permission interaction
 *   Group 5  (PGSZ)    - PBMT and different page sizes
 *   Group 6  (ORDER)   - Memory ordering semantics
 *   Group 7  (ALIAS)   - Aliasing and coherence
 *   Group 8  (SFENCE)  - SFENCE.VMA and PBMT
 *   Group 9  (MODE)    - Multi-mode compatibility
 */

#include "test_helpers.h"

/* --- Group 1: PBMT Basic Encoding Verification (PBMT-01~06) --- */
#include "test_pbmt_basic.c"

/* --- Group 2: PBMT Reserved Value Exceptions (RSVD-01~04) --- */
#include "test_pbmt_reserved.c"

/* --- Group 3: Non-leaf PTE PBMT Check (NLPTE-01~05) --- */
#include "test_nonleaf_pbmt.c"

/* --- Group 4: PBMT and Page Permission Interaction (PERM-01~06, ALIGN-01~03) --- */
#include "test_pbmt_perms.c"

/* --- Group 5: PBMT and Different Page Sizes (PGSZ-01~06) --- */
#include "test_pbmt_pagesz.c"

/* --- Group 6: Memory Ordering Semantics (ORDER-01~06) --- */
#include "test_pbmt_ordering.c"

/* --- Group 7: Aliasing and Coherence (ALIAS-01~06) --- */
#include "test_pbmt_alias.c"

/* --- Group 8: SFENCE.VMA and PBMT (SFENCE-01~04) --- */
#include "test_pbmt_sfence.c"

/* --- Group 9: Multi-mode Compatibility (MODE-01~03) --- */
#include "test_pbmt_modes.c"
