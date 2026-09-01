/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x V-vector Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_Zi_test_plan.md Group 5.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 5.1 (vsstatus.vs field and gating): HVEC-01~08
 *   Group 5.2 (vector FP gating):             HVEC-09~12
 *   Group 5.3 (conditional misa.v writable):  HVEC-13
 */

#include "test_helpers.h"

/* --- Group 5.1: vsstatus.vs field and vector instruction/CSR gating --- */
#include "test_hvec_vs.c"

/* --- Group 5.2: vector floating-point gating (vsstatus.fs) --- */
#include "test_hvec_fp.c"

/* --- Group 5.3: conditional case (misa.v writable) --- */
#include "test_hvec_cond.c"
