/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Zkr Entropy Source Extension Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Zkr_test_plan.md.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1 (SEED)    - seed CSR basic format and address
 *   Group 2 (OPST)    - OPST state machine and entropy field
 *   Group 3 (ROACC)   - Read-only access exception
 *   Group 4 (MACC)    - M-mode access control
 *   Group 5 (UACC)    - U-mode access control (USEED)
 *   Group 6 (SACC)    - S-mode access control (SSEED)
 *   Group 7 (MSECFG)  - mseccfg SSEED/USEED field properties
 *   Group 8 (SEC)     - Security strength requirements
 *   Group 9 (MISC)    - Comprehensive scenarios and edge cases
 */

#include "test_helpers.h"

/* --- Group 1: seed CSR basic format (SEED-01~04) --- */
#include "test_seed_basic.c"

/* --- Group 2: OPST state and entropy (OPST-01~09) --- */
#include "test_opst.c"

/* --- Group 3: Read-only access exception (ROACC-01~09) --- */
#include "test_ro_access.c"

/* --- Group 4: M-mode access control (MACC-01~03) --- */
#include "test_mmode.c"

/* --- Group 5: U-mode access control (UACC-01~07) --- */
#include "test_umode.c"

/* --- Group 6: S-mode access control (SACC-01~09) --- */
#include "test_smode.c"

/* --- Group 7: mseccfg field properties (MSECFG-01~06) --- */
#include "test_mseccfg.c"

/* --- Group 8: Security strength (SEC-01~02) --- */
#include "test_security.c"

/* --- Group 9: Comprehensive scenarios (MISC-01~09) --- */
#include "test_misc.c"
