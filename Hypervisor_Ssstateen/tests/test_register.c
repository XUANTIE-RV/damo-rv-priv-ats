/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Ssstateen Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 8.
 *
 * These tests were extracted from Ssstateen/tests/ Groups 7-12 (H-mode tests).
 * Each test file is #included here so TEST_REGISTER macros place function
 * pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 8.1 (HEXIST):  HCROSS-SSSTA-01~06  hstateen CSR existence
 *   Group 8.2 (HB63):    HCROSS-SSSTA-07~15  bit 63 controls sstateen
 *   Group 8.3 (VSPROP):  HCROSS-SSSTA-16~20  VS-mode ROZ propagation
 *   Group 8.4 (HFUNC):   HCROSS-SSSTA-21~38  functional bit controls
 *   Group 8.5 (HRO):     HCROSS-SSSTA-39~45  read-only constraints
 *   Group 8.6 (HENC):    HCROSS-SSSTA-46~50  encoding consistency
 */

#include "test_helpers.h"

/* --- Group 8.1: hstateen CSR existence and accessibility --- */
#include "test_hcross_ssstateen_hexist.c"

/* --- Group 8.2: hstateen bit 63 controls sstateen access --- */
#include "test_hcross_ssstateen_hb63.c"

/* --- Group 8.3: hstateen VS-mode ROZ propagation --- */
#include "test_hcross_ssstateen_vsprop.c"

/* --- Group 8.4: hstateen0 functional bit controls --- */
#include "test_hcross_ssstateen_hfunc.c"

/* --- Group 8.5: hstateen read-only constraints --- */
#include "test_hcross_ssstateen_hro.c"

/* --- Group 8.6: hstateen encoding / mstateen consistency --- */
#include "test_hcross_ssstateen_henc.c"
