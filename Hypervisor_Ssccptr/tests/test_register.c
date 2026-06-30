/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor × Ssccptr Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 3.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution
 * by main().
 *
 * Execution order:
 *   Group 3 (HCROSS-SSCCPTR) - Hypervisor × Ssccptr cross tests
 */

#include "test_helpers.h"

/* --- Group 3: Hypervisor × Ssccptr (HCROSS-SSCCPTR-01~04) --- */
#include "test_hcross_ssccptr.c"
