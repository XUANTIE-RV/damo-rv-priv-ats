/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Sstvala Extension Test Registration
 *
 * All test cases are organized by Group, matching docs/sstvala_test_plan.md.
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1 (PF)   - Page-Fault stval == faulting VA        (7 cases)
 *   Group 2 (AF)   - Access-Fault stval == faulting VA      (4 cases)
 *   Group 3 (MA)   - Misaligned stval == faulting VA        (3 cases)
 *   Group 4 (BKP)  - Breakpoint stval == faulting addr      (3 cases)
 *   Group 5 (ILL)  - Illegal Instruction stval == encoding  (5 cases)
 *   Group 6 (VI)   - Virtual Instruction stval == encoding  (3 cases)
 */

#include "test_helpers.h"

/* --- Group 1: Page-Fault (TVAL-LPF-01~03, TVAL-SPF-01~02, TVAL-IPF-01~02) --- */
#include "test_pagefault.c"

/* --- Group 2: Access-Fault (TVAL-LAF-01~02, TVAL-SAF-01, TVAL-IAF-01) --- */
#include "test_accessfault.c"

/* --- Group 3: Misaligned (TVAL-LMA-01, TVAL-SMA-01, TVAL-IMA-01) --- */
#include "test_misaligned.c"

/* --- Group 4: Breakpoint (TVAL-BKP-01~03) --- */
#include "test_breakpoint.c"

/* --- Group 5: Illegal Instruction (TVAL-ILL-01~05) --- */
#include "test_illegal.c"

/* --- Group 6: Virtual Instruction (TVAL-VI-01~03, optional H ext) --- */
#include "test_virtual_inst.c"
