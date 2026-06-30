/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Svinval Extension Test Registration
 *
 * All test cases are organized by Group, matching svinval_test_plan.md.
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 7  (ENC)     - Instruction encoding verification
 *   Group 1  (SINVAL)  - SINVAL.VMA basic functionality
 *   Group 2  (ORDER)   - Ordering semantics verification
 *   Group 3  (BATCH)   - Batch invalidation operations
 *   Group 4  (PARAM)   - rs1/rs2 parameter combinations
 *   Group 5  (PRIV)    - SINVAL.VMA privilege level exceptions
 *   Group 6  (FENCE)   - SFENCE.W.INVAL / SFENCE.INVAL.IR privilege behavior
 */

#include "test_helpers.h"

/* --- Group 7: Instruction Encoding Verification (ENC-01~03) --- */
#include "test_encoding.c"

/* --- Group 1: SINVAL.VMA Basic Functionality (SINVAL-01~07) --- */
#include "test_sinval_basic.c"

/* --- Group 2: Ordering Semantics Verification (ORDER-01~06) --- */
#include "test_ordering.c"

/* --- Group 3: Batch Invalidation Operations (BATCH-01~04) --- */
#include "test_batch.c"

/* --- Group 4: rs1/rs2 Parameter Combinations (PARAM-01~05) --- */
#include "test_params.c"

/* --- Group 5: SINVAL.VMA Privilege Level Exceptions (PRIV-01~04) --- */
#include "test_sinval_priv.c"

/* --- Group 6: SFENCE.W.INVAL / SFENCE.INVAL.IR Privilege Behavior (FENCE-01~08) --- */
#include "test_fence_priv.c"
