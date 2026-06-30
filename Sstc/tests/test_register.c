/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Sstc Extension Test Registration
 *
 * All test cases are organized by Group, matching docs/sstc_test_plan.md.
 * Each test file is #included here so TEST_REGISTER macros place function
 * pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1 (STCE)       - menvcfg STCE field read/write
 *   Group 2 (RW)         - stimecmp CSR read/write
 *   Group 3 (ACC)        - Access control (S-mode only)
 *   Group 4 (TMR)        - Timer interrupt generation
 *   Group 5 (STIP)       - STIP read-only behavior
 *
 * Note: Group 6 (vstimecmp/VS-mode) and Hypervisor-dependent tests from
 * Groups 1 and 3 have been migrated to Hypervisor_Sstc/ project.
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 9.
 */

#include "test_helpers.h"

/* --- Group 1: menvcfg STCE field control (SSTC-STCE-01,02,05) --- */
#include "test_stce.c"

/* --- Group 2: stimecmp CSR read/write (SSTC-RW-01~06) --- */
#include "test_rw.c"

/* --- Group 3: Access control (SSTC-ACC-01~07) --- */
#include "test_access.c"

/* --- Group 4: Timer interrupt generation (SSTC-TMR-01~10) --- */
#include "test_timer.c"

/* --- Group 5: STIP read-only behavior (SSTC-STIP-01~05) --- */
#include "test_stip.c"
