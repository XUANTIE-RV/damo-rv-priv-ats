/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Sstc Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 9.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution
 * by main().
 *
 * Execution order:
 *   9.1: HCROSS-SSTC-01/02 - henvcfg.STCE field control
 *   9.2: HCROSS-SSTC-03/04/05 - VS-mode access control
 *   9.3-9.5: HCROSS-SSTC-06~15 - vstimecmp / VSTIP / VS-mode timer
 */

#include "test_helpers.h"

/* 9.1: henvcfg.STCE field control (HCROSS-SSTC-01/02) */
#include "test_hcross_sstc_stce.c"

/* 9.2: VS-mode access control (HCROSS-SSTC-03/04/05) */
#include "test_hcross_sstc_acc.c"

/* 9.3-9.5: vstimecmp / VSTIP / VS-mode timer (HCROSS-SSTC-06~15) */
#include "test_hcross_sstc_vs.c"
