/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Ssqosid Extension Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Ssqosid_test_plan.md.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1 (SRMCFG-01~07)  - srmcfg basic read/write and field behavior
 *   Group 2 (SRMCFG-08~10)  - Cross-privilege mode applicability
 *   Group 3 (SRMCFG-11~18)  - Smstateen gating access control
 *   Group 4 (SRMCFG-25~26)  - U-mode access control
 *
 * Note: Group 15 (SRMCFG-19~24, Virtualization mode V=1) has been
 * moved to Hypervisor_Ssqosid/ (Hypervisor_cross_test_plan.md Group 15).
 */

#include "test_helpers.h"

/* --- Group 1: srmcfg basic read/write and field behavior --- */
#include "test_basic.c"

/* --- Group 2: Cross-privilege mode applicability --- */
#include "test_cross_priv.c"

/* --- Group 3: Smstateen gating access control --- */
#include "test_smstateen.c"

/* --- Group 4: U-mode access control --- */
#include "test_umode.c"
