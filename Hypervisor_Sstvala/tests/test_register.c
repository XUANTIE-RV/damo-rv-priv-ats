/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor × Sstvala Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 2.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution
 * by main().
 *
 * Execution order:
 *   HCROSS-SSTVALA-01: Instruction guest-page-fault stval precision
 *   HCROSS-SSTVALA-02: Store guest-page-fault stval precision
 *   HCROSS-SSTVALA-03: AMO guest-page-fault stval precision
 *   HCROSS-SSTVALA-04: Instruction guest-page-fault vstval (delegated)
 *   HCROSS-SSTVALA-05: Store guest-page-fault vstval (delegated)
 *   HCROSS-SSTVALA-06: Virtual-instruction stval (read hstatus)
 *   HCROSS-SSTVALA-07: Virtual-instruction stval (write hgatp)
 *   HCROSS-SSTVALA-08: Virtual-instruction stval (read hideleg)
 */

#include "test_helpers.h"

/* Group 2: Hypervisor × Sstvala */
#include "test_hcross_sstvala.c"
