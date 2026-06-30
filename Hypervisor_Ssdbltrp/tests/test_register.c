/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Ssdbltrp Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 12.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution
 * by main().
 *
 * Execution order:
 *   12.1: HCROSS-SSDBLTRP-01~06 - henvcfg.DTE control
 *   12.2: HCROSS-SSDBLTRP-07~12 - vsstatus.SDT in VS-mode
 *   12.3: HCROSS-SSDBLTRP-13~16 - SRET clears vsstatus.SDT
 *   12.4: HCROSS-SSDBLTRP-17~18 - menvcfg.DTE control
 *   12.5: HCROSS-SSDBLTRP-19~24 - MRET/SRET/MNRET cross-mode clear
 */

#include "test_helpers.h"

/* 12.1: henvcfg.DTE control (HCROSS-SSDBLTRP-01~06) */
#include "test_hcross_ssdbltrp_henvcfg.c"

/* 12.2: vsstatus.SDT in VS-mode (HCROSS-SSDBLTRP-07~12) */
#include "test_hcross_ssdbltrp_vsstatus.c"

/* 12.3: SRET clears vsstatus.SDT (HCROSS-SSDBLTRP-13~16) */
#include "test_hcross_ssdbltrp_sret.c"

/* 12.4: menvcfg.DTE control (HCROSS-SSDBLTRP-17~18) */
#include "test_hcross_ssdbltrp_menvcfg.c"

/* 12.5: MRET/SRET/MNRET cross-mode clear (HCROSS-SSDBLTRP-19~24) */
#include "test_hcross_ssdbltrp_xret.c"
