/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Smcntrpmf Test Registration File
 *
 * All test cases are organized by Group, matching Smcntrpmf_test_plan.md.
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1 (CSR)  - mcyclecfg/minstretcfg CSR access and field constraints
 *   Group 2 (CYC)  - Cycle counter privilege mode filtering
 *   Group 3 (INS)  - Instret counter privilege mode filtering
 *   Group 4 (TR)   - Mode transition and counting boundary behavior
 *   Group 6 (INH)  - Interaction with mcountinhibit
 *   Group 7 (CTR)  - Interaction with mcounteren/scounteren
 */

#include "smcntrpmf_helpers.h"

/* --- Group 1: CSR Access & Field Constraints (PMF-CSR-01 ~ CSR-13) --- */
#include "test_csr_rw.c"

/* --- Group 2: Cycle Counter Filtering (PMF-CYC-01 ~ CYC-11) --- */
#include "test_cycle_filter.c"

/* --- Group 3: Instret Counter Filtering (PMF-INS-01 ~ INS-14) --- */
#include "test_instret_filter.c"

/* --- Group 4: Mode Transition Behavior (PMF-TR-01 ~ TR-10) --- */
#include "test_transition.c"

/* --- Group 6: mcountinhibit Interaction (PMF-INH-01 ~ INH-04) --- */
#include "test_mcountinhibit.c"

/* --- Group 7: counteren Interaction (PMF-CTR-01 ~ CTR-05) --- */
#include "test_counteren.c"
