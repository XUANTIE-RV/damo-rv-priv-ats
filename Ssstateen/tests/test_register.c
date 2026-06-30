/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Ssstateen Extension Compliance Test Registration
 *
 * Each test file is #included so TEST_REGISTER places the function
 * pointers into the .test_table section for auto-execution by main().
 *
 * Execution order matches DOCS/testplan/ssstateen_test_plan.md:
 *   Group 1   EXIST    - sstateen CSR existence and accessibility
 *   Group 2   UCTL     - sstateen U-mode/VU-mode access control
 *   Group 3   FUNC     - sstateen0 functional bits
 *   Group 4   ALLOC    - sstateen bit allocation and read-only constraints
 *   Group 5   EXC      - sstateen exception behavior
 *   Group 6   INIT     - sstateen initialization requirements
 *
 * H-mode tests (Groups 7-12) have been extracted to Hypervisor_Ssstateen/.
 */

#include "test_helpers.h"

/* S-mode tests (Groups 1-6) */
/* Group 1  */ #include "test_exist.c"
/* Group 2  */ #include "test_uctl.c"
/* Group 3  */ #include "test_func.c"
/* Group 4  */ #include "test_alloc.c"
/* Group 5  */ #include "test_exc.c"
/* Group 6  */ #include "test_init.c"

/*
 * H-mode tests (Groups 7-12) have been extracted to the
 * Hypervisor_Ssstateen/ subproject and renumbered to HCROSS-SSSTA-01~50.
 * See DOCS/testplan/Hypervisor_cross_test_plan.md Group 8.
 */
