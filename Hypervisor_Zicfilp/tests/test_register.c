/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Zicfilp Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_CFI_test_plan.md Part A.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution
 * by main().
 *
 * Execution order:
 *   Group A1 (HCFI-LP-01~10)  - henvcfg.LPE enable control
 *   Group A2 (HCFI-LP-11~24)  - vsstatus.SPELP save/restore
 *   Group A3 (HCFI-LP-25~33)  - VS-mode Landing Pad functionality
 *   Group A4 (HCFI-LP-34~41)  - software-check exception delegation
 *   Group A5 (HCFI-LP-42~44)  - async event ELP interaction
 */

#include "test_helpers.h"

/* --- Group A1: henvcfg.LPE enable control (HCFI-LP-01~10) --- */
#include "test_hyp_zicfilp_envcfg.c"

/* --- Group A2: vsstatus.SPELP save/restore (HCFI-LP-11~24) --- */
#include "test_hyp_zicfilp_spelp.c"

/* --- Group A3: VS-mode Landing Pad functionality (HCFI-LP-25~33) --- */
#include "test_hyp_zicfilp_lpad.c"

/* --- Group A4: software-check exception delegation (HCFI-LP-34~41) --- */
#include "test_hyp_zicfilp_deleg.c"

/* --- Group A5: async event ELP interaction (HCFI-LP-42~44) --- */
#include "test_hyp_zicfilp_async.c"
