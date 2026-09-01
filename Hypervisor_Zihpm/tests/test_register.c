/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Zihpm Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_Zi_test_plan.md Group 7.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 7.1 (unimplemented hpmcounter when V=1): HZHPM-01~04
 *   Group 7.2 (VU three-layer gating chain):       HZHPM-05
 */

#include "test_helpers.h"

/* --- Group 7.1: unimplemented hpmcounter behavior when V=1 --- */
#include "test_hzhpM_unimpl.c"

/* --- Group 7.2: hpmcounter VU three-layer gating chain --- */
#include "test_hzhpM_chain.c"
