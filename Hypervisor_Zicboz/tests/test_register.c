/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Zicboz Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_CMO_test_plan.md.
 *
 * Execution order:
 *   Group 3 (HCBZE)    - henvcfg.CBZE control
 *   Groups 4,5,6       - htinst / G-stage / stval for cbo.zero
 */

#include "test_helpers.h"

/* --- Group 3: henvcfg CBZE control (HCBZE-01~07) --- */
#include "test_hcbze.c"

/* --- Groups 4,5,6: htinst / G-stage / stval (HCXINST-04,09,11;
 *     HCGSTAGE-04,06,07,10; HVINST-04) --- */
#include "test_hcboz_cross.c"
