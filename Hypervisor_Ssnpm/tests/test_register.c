/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Ssnpm Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_Zpm_test_plan.md.
 *
 * Execution order:
 *   Group 1 (HZPM-CAP)  - henvcfg.PMM / hstatus.HUPMM CSR control
 *   Group 2 (HZPM-VS)   - VS-mode pointer masking
 *   Group 3 (HZPM-VU)   - VU-mode pointer masking
 *   Group 4 (HZPM-HLV)  - HLV/HSV pointer-mask selection
 *   Group 5 (HZPM-2STG) - two-stage translation x PM
 *   Group 8 (HZPM-TRAP) - virtualized trap x PM
 */

#include "test_helpers.h"

/* --- Group 1: capability + CSR control (HZPM-CAP-01~10) --- */
#include "test_cap.c"

/* --- Group 2: VS-mode PM (HZPM-VS-01~10) --- */
#include "test_vs.c"

/* --- Group 3: VU-mode PM (HZPM-VU-01~07) --- */
#include "test_vu.c"

/* --- Group 4: HLV/HSV PM selection (HZPM-HLV-01~10) --- */
#include "test_hlv.c"

/* --- Group 5: two-stage x PM (HZPM-2STG-01~05) --- */
#include "test_2stg.c"

/* --- Group 8: trap x PM (HZPM-TRAP-01~06) --- */
#include "test_trap.c"
