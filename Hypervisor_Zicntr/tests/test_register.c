/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Zicntr Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_Zi_test_plan.md Group 6.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 6.1 (rdtime/htimedelta):      HZCNT-01~06
 *   Group 6.2 (scounteren -> VU-mode):  HZCNT-07~09
 */

#include "test_helpers.h"

/* --- Group 6.1: rdtime instruction-level htimedelta semantics --- */
#include "test_hzcnt_rdtime.c"

/* --- Group 6.2: scounteren continued control of VU-mode --- */
#include "test_hzcnt_scounteren.c"
