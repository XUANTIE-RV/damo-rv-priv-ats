/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Ssqosid Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 15.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 15 (SRMCFG-19~24) - Virtualization mode (V=1) access exceptions
 */

#include "test_helpers.h"

/* --- Group 15: Virtualization mode (V=1) access exceptions --- */
#include "test_virt.c"
