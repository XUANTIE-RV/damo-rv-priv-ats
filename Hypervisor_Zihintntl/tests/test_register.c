/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Zihintntl Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_Zi_test_plan.md Group 2.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 2.1 (HS/VS/VU normal): NTL-HYP-01~03
 *   Group 2.2 (HLV/HSV/HLVX):    NTL-HYP-04
 *   Group 2.3 (NTL + CMO):       NTL-HYP-05a/05b
 *   Group 2.4 (G-stage fault):   NTL-HYP-06
 */

#include "test_helpers.h"

/* --- Group 2.1: NTL execution in HS/VS/VU modes --- */
#include "test_hypntl_exec.c"

/* --- Group 2.2: NTL applied to HLV/HSV/HLVX --- */
#include "test_hypntl_hlv.c"

/* --- Group 2.3: NTL + CMO virtual-instruction report --- */
#include "test_hypntl_cmo.c"

/* --- Group 2.4: NTL + load G-stage guest-page-fault --- */
#include "test_hypntl_gstage.c"
