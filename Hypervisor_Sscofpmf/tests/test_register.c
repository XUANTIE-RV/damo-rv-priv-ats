/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Sscofpmf Test Registration File
 *
 * All test cases are organized by Group, matching
 * Hypervisor_Ss_test_plan.md Group 10 (Hypervisor x Sscofpmf).
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 10.1 (HCROSS-SSCOFPMF-01 ~ 03) - VS-mode scountovf dual gating
 *   Group 10.2 (HCROSS-SSCOFPMF-04 ~ 06) - VSINH/VUINH counting inhibition
 */

#include "sscofpmf_helpers.h"

/* --- Group 10.1: VS-mode scountovf dual gating (migrated) --- */
#include "test_scountovf_vs.c"

/* --- Group 10.2: VSINH/VUINH counting inhibition (new) --- */
#include "test_mode_filter_vs.c"
