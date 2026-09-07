/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Zawrs Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_Za_test_plan.md Group 6 (HZWRS-01~12).
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 6.1 (normal execution): HZWRS-01~03  HS/VS/VU wrs normal
 *   Group 6.2 (VTW gating):       HZWRS-04~07  VTW virtual-instruction
 *   Group 6.3 (TW priority):      HZWRS-08~12  TW over VTW, scope
 */

#include "test_helpers.h"

/* --- Group 6.1: HS/VS/VU-mode wrs normal execution (VTW=0) --- */
#include "test_hzwrs_exec.c"

/* --- Group 6.2: hstatus.VTW gating (VS/VU-mode wrs.nto) --- */
#include "test_hzwrs_vtw.c"

/* --- Group 6.3: TW priority and instruction scope (VS/VU-mode) --- */
#include "test_hzwrs_tw.c"
