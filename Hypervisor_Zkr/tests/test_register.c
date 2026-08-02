/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Zkr Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 17.
 *
 * These tests were extracted from Zkr_test_plan.md Group 7 (VS/VU-mode)
 * and the HS-mode cases from Group 6, renumbered to ZKR-HYP-01~12.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 17.1 (HS-mode):    ZKR-HYP-01~02, 13   HS-mode SSEED access control
 *   Group 17.2 (VS/VU-mode): ZKR-HYP-03~11, 14~18 VS/VU virtual-inst vs illegal
 *   Group 17.3 (priority):   ZKR-HYP-12          read-only exception priority (VU-mode)
 */

#include "test_helpers.h"

/* --- Group 17.1: HS-mode access control (SSEED) --- */
#include "test_hcross_zkr_hs.c"

/* --- Group 17.2: VS/VU-mode access control (SSEED + H ext) --- */
#include "test_hcross_zkr_vsvu.c"

/* --- Group 17.3: exception priority and combined scenarios --- */
#include "test_hcross_zkr_prio.c"
