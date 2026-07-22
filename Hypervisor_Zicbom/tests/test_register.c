/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Zicbom Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_CMO_test_plan.md.
 *
 * Execution order:
 *   Group 1 (HCBIE)    - henvcfg.CBIE control
 *   Group 2 (HCBCFE)   - henvcfg.CBCFE control
 *   Group 4 (HCXINST)  - htinst standard transformation
 *   Group 5 (HCGSTAGE) - G-stage address translation faults
 *   Group 6 (HVINST)   - virtual-instruction stval
 */

#include "test_helpers.h"

/* --- Group 1: henvcfg CBIE control (HCBIE-01~13) --- */
#include "test_hcbie.c"

/* --- Group 2: henvcfg CBCFE control (HCBCFE-01~10) --- */
#include "test_hcbcfe.c"

/* --- Group 4: htinst standard transformation (HCXINST-01~03,07,08,10) --- */
#include "test_hcxinst.c"

/* --- Group 5: G-stage faults (HCGSTAGE-01~03,05,08,09,12) --- */
#include "test_hcgstage.c"

/* --- Group 6: virtual-instruction stval (HVINST-01~03,05) --- */
#include "test_hvinst.c"
